// Monte Carlo tab. Display only: every number comes from /api/v1/mc/*.
// Uses $, api, color, showError and scheduleRefresh from app.js (loaded after this file).
"use strict";

let mcProduct = "vanilla";
let mcChart = null;

function mcParams() {
  return {
    type: $("mc-type").value,
    paths: parseInt($("mc-paths").value, 10),
    seed: parseInt($("mc-seed").value, 10),
  };
}

const fmt4 = (x) => x.toFixed(4);
const signed4 = (x) => `${x < 0 ? "−" : "+"}${Math.abs(x).toFixed(4)}`;
const inSe = (diff, se) => (se > 0 ? `${diff < 0 ? "−" : "+"}${Math.abs(diff / se).toFixed(1)} SE` : "exact");

function showMcPrice(data) {
  const diff = data.price - data.bs_price;
  const inside = data.ci_low <= data.bs_price && data.bs_price <= data.ci_high;
  $("mc-price").textContent = fmt4(data.price);
  $("mc-se").textContent = `± ${fmt4(data.std_error)} std error · 95%: `
    + `${fmt4(data.ci_low)} – ${fmt4(data.ci_high)}`;
  $("mc-bs").textContent = fmt4(data.bs_price);
  $("mc-diff").textContent = signed4(diff);
  $("mc-verdict").textContent = inside
    ? `${inSe(diff, data.std_error)} · Black-Scholes is inside the 95% interval`
    : `${inSe(diff, data.std_error)} · outside the 95% interval (expect this 1 run in 20)`;
  $("mc-diff").classList.toggle("loss", !inside);
}

function showConvergence(data) {
  $("mc-table").querySelector("tbody").innerHTML = data.rows.map((r) => `
    <tr>
      <td>${r.paths.toLocaleString("en-US")}</td><td>${fmt4(r.price)}</td>
      <td>${fmt4(r.std_error)}</td><td>${fmt4(r.ci_low)} – ${fmt4(r.ci_high)}</td>
      <td>${signed4(r.price - data.bs_price)} <span class="hint">(${inSe(r.price - data.bs_price, r.std_error)})</span></td>
    </tr>`).join("")
    + `<tr><td colspan="5" class="hint">Black-Scholes price: ${fmt4(data.bs_price)} · `
    + `seed ${data.seed} · each row is a separate run from the same seed</td></tr>`;
  drawConvergenceChart(data);
}

function drawConvergenceChart(data) {
  const ink = color("--muted");
  const call = color("--call");
  const xs = data.rows.map((r) => r.paths);
  const series = (key) => data.rows.map((r) => ({ x: r.paths, y: r[key] }));
  const datasets = [
    { label: "upper", data: series("ci_high"), borderWidth: 0, pointRadius: 0, fill: false },
    { label: "95% interval", data: series("ci_low"), borderWidth: 0, pointRadius: 0,
      fill: "-1", backgroundColor: `${call}33`, pointStyle: "rect" },
    { label: "Monte Carlo estimate", data: series("price"), borderColor: call,
      backgroundColor: call, borderWidth: 2, pointRadius: 3, pointStyle: "circle" },
    { label: "Black-Scholes", data: [{ x: xs[0], y: data.bs_price }, { x: xs[xs.length - 1], y: data.bs_price }],
      borderColor: color("--put"), backgroundColor: color("--put"), borderWidth: 1.5,
      borderDash: [6, 4], pointRadius: 0, pointStyle: "line" },
  ];
  const options = {
    animation: false,
    maintainAspectRatio: false,
    interaction: { mode: "nearest", axis: "x", intersect: false },
    plugins: {
      legend: { labels: { color: ink, usePointStyle: true, filter: (item) => item.text !== "upper" } },
      tooltip: {
        filter: (item) => item.dataset.label !== "upper" && item.dataset.label !== "95% interval",
        callbacks: {
          title: (items) => `${items[0].parsed.x.toLocaleString("en-US")} paths`,
          label: (item) => `${item.dataset.label}: ${fmt4(item.parsed.y)}`,
        },
      },
    },
    scales: {
      x: {
        type: "logarithmic",
        min: xs[0],
        max: xs[xs.length - 1],
        title: { display: true, text: "Paths simulated (log scale)", color: ink },
        ticks: {
          color: ink,
          maxRotation: 0,
          callback: (v) => (Number.isInteger(Math.log10(v)) ? v.toLocaleString("en-US") : ""),
        },
        grid: { display: false },
      },
      y: {
        title: { display: true, text: "Option price", color: ink },
        ticks: { color: ink },
        grid: { color: color("--grid") },
      },
    },
  };
  if (mcChart) mcChart.destroy();
  mcChart = new Chart($("mc-chart"), { type: "line", data: { datasets }, options });
}

async function refreshMc(m) {
  const p = mcParams();
  const option = { ...m, type: p.type, seed: p.seed };
  try {
    const [price, convergence] = await Promise.all([
      api("/api/v1/mc/price", { ...option, paths: p.paths }),
      api("/api/v1/mc/convergence", option),
    ]);
    showMcPrice(price);
    showConvergence(convergence);
    showError($("mc-error"), "");
  } catch (e) {
    for (const id of ["mc-price", "mc-bs", "mc-diff"]) $(id).textContent = "–";
    showError($("mc-error"), e.message);
  }
}

// Called by app.js once its helpers exist.
function initMonteCarlo() {
  $("mc-type").addEventListener("change", scheduleRefresh);
  for (const id of ["mc-paths", "mc-seed"]) $(id).addEventListener("input", scheduleRefresh);
  $("mc-new").addEventListener("click", () => {
    $("mc-seed").value = Math.floor(Math.random() * 1e9);
    scheduleRefresh();
  });
  document.querySelectorAll("#mc-products button").forEach((btn) => {
    btn.addEventListener("click", () => {
      mcProduct = btn.dataset.product;
      document.querySelectorAll("#mc-products button").forEach((b) =>
        b.setAttribute("aria-selected", String(b === btn)));
      scheduleRefresh();
    });
  });
}
