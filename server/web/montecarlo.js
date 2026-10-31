// Monte Carlo tab. Display only: every number comes from /api/v1/mc/*.
// Uses $, api, color, showError and scheduleRefresh from app.js (loaded after this file).
"use strict";

let mcProduct = "vanilla";
let mcChart = null;

const MC_METHODS = [
  { key: "plain", name: "Plain", colorVar: "--call" },
  { key: "antithetic", name: "Antithetic", colorVar: "--put" },
  { key: "control_variate", name: "Control variate", colorVar: "--gain" },
];

function mcParams() {
  return {
    type: $("mc-type").value,
    paths: parseInt($("mc-paths").value, 10),
    seed: parseInt($("mc-seed").value, 10),
  };
}

const fmt4 = (x) => x.toFixed(4);
const speedupText = (s) => (s === null ? "no error left" : `≈ ${s < 10 ? s.toFixed(1) : Math.round(s).toLocaleString("en-US")}× plain paths`);

function showMcPrice(data) {
  $("mc-bs").textContent = fmt4(data.bs_price);
  for (const m of MC_METHODS) {
    const e = data.estimates[m.key];
    $(`mc-${m.key}`).textContent = fmt4(e.price);
    $(`mc-${m.key}-se`).textContent = `± ${fmt4(e.std_error)} · 95%: ${fmt4(e.ci_low)} – ${fmt4(e.ci_high)}`;
    $(`mc-${m.key}-speedup`).textContent = m.key === "plain"
      ? "the baseline: 1 path = 1 path"
      : `worth ${speedupText(e.speedup)}`;
    $(`mc-${m.key}`).classList.toggle("loss", !(e.ci_low <= data.bs_price && data.bs_price <= e.ci_high));
  }
}

function showConvergence(data) {
  const cell = (e) => `${fmt4(e.price)} <span class="hint">± ${fmt4(e.std_error)}</span>`;
  const gain = (e) => (e.speedup === null ? "–" : `${e.speedup.toFixed(1)}×`);
  $("mc-table").querySelector("tbody").innerHTML = data.rows.map((r) => `
    <tr>
      <td>${r.paths.toLocaleString("en-US")}</td>
      <td>${cell(r.estimates.plain)}</td>
      <td>${cell(r.estimates.antithetic)}</td>
      <td>${cell(r.estimates.control_variate)}</td>
      <td>${gain(r.estimates.antithetic)} / ${gain(r.estimates.control_variate)}</td>
    </tr>`).join("")
    + `<tr><td colspan="5" class="hint">Black-Scholes price: ${fmt4(data.bs_price)} · `
    + `seed ${data.seed} · each method uses the same number of simulated prices</td></tr>`;
  drawConvergenceChart(data);
}

function drawConvergenceChart(data) {
  const ink = color("--muted");
  const xs = data.rows.map((r) => r.paths);
  const series = (key, field) => data.rows.map((r) => ({ x: r.paths, y: r.estimates[key][field] }));
  const datasets = [];
  for (const m of MC_METHODS) {
    const c = color(m.colorVar);
    datasets.push(
      { label: `${m.name} upper`, group: m.key, band: true, data: series(m.key, "ci_high"),
        borderWidth: 0, pointRadius: 0, fill: false },
      { label: `${m.name} 95%`, group: m.key, band: true, data: series(m.key, "ci_low"),
        borderWidth: 0, pointRadius: 0, fill: "-1", backgroundColor: `${c}2e` },
      { label: m.name, group: m.key, data: series(m.key, "price"), borderColor: c,
        backgroundColor: c, borderWidth: 2, pointRadius: 2.5, pointStyle: "circle" },
    );
  }
  datasets.push({
    label: "Black-Scholes", group: "bs",
    data: [{ x: xs[0], y: data.bs_price }, { x: xs[xs.length - 1], y: data.bs_price }],
    borderColor: color("--payoff"), backgroundColor: color("--payoff"), borderWidth: 1.5,
    borderDash: [6, 4], pointRadius: 0, pointStyle: "line",
  });

  const options = {
    animation: false,
    maintainAspectRatio: false,
    interaction: { mode: "nearest", axis: "x", intersect: false },
    plugins: {
      legend: {
        labels: { color: ink, usePointStyle: true, filter: (item, chartData) => !chartData.datasets[item.datasetIndex].band },
        onClick: (event, item, legend) => {
          const chart = legend.chart;
          const group = chart.data.datasets[item.datasetIndex].group;
          const show = !chart.isDatasetVisible(item.datasetIndex);
          chart.data.datasets.forEach((d, i) => {
            if (d.group === group) chart.setDatasetVisibility(i, show);
          });
          chart.update();
        },
      },
      tooltip: {
        filter: (item) => !item.dataset.band,
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
    for (const id of ["mc-bs", ...MC_METHODS.map((x) => `mc-${x.key}`)]) $(id).textContent = "–";
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
