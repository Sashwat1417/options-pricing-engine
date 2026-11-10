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

const fmt4 = (x) => x.toFixed(4);

// `exact`: the reference is the true price, so an interval that misses it is flagged.
const MC_PRODUCTS = {
  vanilla: {
    endpoint: "/api/v1/mc",
    reference: (d) => d.bs_price,
    referenceName: "Black-Scholes",
    referenceHint: () => "exact formula (Layer 1): the target",
    exact: true,
  },
  asian: {
    endpoint: "/api/v1/mc/asian",
    reference: (d) => d.lognormal_price,
    referenceName: "Hull approximation",
    referenceHint: (d) => `lognormal fit (26.3–26.4), not exact · geometric average option `
      + `(exact, the control): ${fmt4(d.geometric_price)}`,
    exact: false,
  },
};

function mcParams() {
  return {
    type: $("mc-type").value,
    paths: parseInt($("mc-paths").value, 10),
    seed: parseInt($("mc-seed").value, 10),
    fixings: parseInt($("mc-fixings").value, 10),
  };
}
const speedupText = (s) => (s === null ? "no error left" : `≈ ${s < 10 ? s.toFixed(1) : Math.round(s).toLocaleString("en-US")}× plain paths`);

function showMcPrice(data, product) {
  const reference = product.reference(data);
  $("mc-ref-label").textContent = product.referenceName;
  $("mc-ref").textContent = fmt4(reference);
  $("mc-ref-hint").textContent = product.referenceHint(data);
  for (const m of MC_METHODS) {
    const e = data.estimates[m.key];
    $(`mc-${m.key}`).textContent = fmt4(e.price);
    $(`mc-${m.key}-se`).textContent = `± ${fmt4(e.std_error)} · 95%: ${fmt4(e.ci_low)} – ${fmt4(e.ci_high)}`;
    $(`mc-${m.key}-speedup`).textContent = m.key === "plain"
      ? "the baseline: 1 path = 1 path"
      : `worth ${speedupText(e.speedup)}`;
    $(`mc-${m.key}`).classList.toggle("loss",
      product.exact && !(e.ci_low <= reference && reference <= e.ci_high));
  }
}

function showConvergence(data, product) {
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
    + `<tr><td colspan="5" class="hint">${product.referenceName}: ${fmt4(product.reference(data))} · `
    + `seed ${data.seed}${data.fixings ? ` · ${data.fixings} fixings` : ""} · `
    + `each method uses the same number of simulated paths</td></tr>`;
  drawConvergenceChart(data, product);
}

function drawConvergenceChart(data, product) {
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
  const reference = product.reference(data);
  datasets.push({
    label: product.referenceName, group: "reference",
    data: [{ x: xs[0], y: reference }, { x: xs[xs.length - 1], y: reference }],
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
  const key = mcProduct;
  const product = MC_PRODUCTS[key];
  const option = { ...m, type: p.type, seed: p.seed, ...(key === "asian" ? { fixings: p.fixings } : {}) };
  try {
    const [price, convergence] = await Promise.all([
      api(`${product.endpoint}/price`, { ...option, paths: p.paths }),
      api(`${product.endpoint}/convergence`, option),
    ]);
    if (key !== mcProduct) return;
    showMcPrice(price, product);
    showConvergence(convergence, product);
    showError($("mc-error"), "");
  } catch (e) {
    if (key !== mcProduct) return;
    for (const id of ["mc-ref", ...MC_METHODS.map((x) => `mc-${x.key}`)]) $(id).textContent = "–";
    showError($("mc-error"), e.message);
  }
}

function selectMcProduct(name) {
  if (!(name in MC_PRODUCTS)) return;
  mcProduct = name;
  document.querySelectorAll("#mc-products button").forEach((b) =>
    b.setAttribute("aria-selected", String(b.dataset.product === name)));
  document.querySelectorAll("#view-mc [data-for]").forEach((el) => {
    el.hidden = el.dataset.for !== name;
  });
}

// Called by app.js once its helpers exist.
function initMonteCarlo() {
  $("mc-type").addEventListener("change", scheduleRefresh);
  for (const id of ["mc-paths", "mc-seed", "mc-fixings"]) $(id).addEventListener("input", scheduleRefresh);
  $("mc-new").addEventListener("click", () => {
    $("mc-seed").value = Math.floor(Math.random() * 1e9);
    scheduleRefresh();
  });
  document.querySelectorAll("#mc-products button").forEach((btn) => {
    btn.addEventListener("click", () => {
      selectMcProduct(btn.dataset.product);
      try { history.replaceState(null, "", `#mc-${mcProduct}`); } catch (e) {}
      scheduleRefresh();
    });
  });
}
