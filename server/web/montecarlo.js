// Monte Carlo tab. Display only: every number comes from /api/v1/mc/*.
// Uses $, api, color, showError and scheduleRefresh from app.js (loaded after this file).
"use strict";

let mcProduct = "vanilla";
let mcChart = null;
let trChart = null;

// Illustrative AUD/USD for Hull's treasurer (26.13 gives no numbers).
const TREASURER_INPUTS = { spot: 0.65, strike: 0.65, days: 365, rate: 4, vol: 10 };
const TREASURER_YEARS = { 5: "Bad (1 in 20)", 25: "Below par", 50: "Typical", 75: "Above par", 95: "Good (1 in 20)" };

const MC_METHODS = [
  { key: "plain", name: "Plain", colorVar: "--call" },
  { key: "antithetic", name: "Antithetic", colorVar: "--put" },
  { key: "control_variate", name: "Control variate", colorVar: "--gain" },
];

// More decimals for small prices, such as a currency option's premium per unit.
let mcDigits = 4;
const fmtPrice = (x) => x.toFixed(mcDigits);
const digitsFor = (reference) => (Math.abs(reference) < 1 ? 6 : 4);
const millions = (x) => `$${(x / 1e6).toFixed(2)}M`;
const signedMillions = (x) => `${x < 0 ? "−" : "+"}${millions(Math.abs(x))}`;

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
      + `(exact, the control): ${fmtPrice(d.geometric_price)}`,
    exact: false,
  },
};

function mcParams() {
  return {
    type: $("mc-type").value,
    paths: parseInt($("mc-paths").value, 10),
    seed: parseInt($("mc-seed").value, 10),
    fixings: parseInt($("mc-fixings").value, 10),
    yield: parseFloat($("mc-yield").value) / 100,
  };
}
const speedupText = (s) => (s === null ? "no error left" : `≈ ${s < 10 ? s.toFixed(1) : Math.round(s).toLocaleString("en-US")}× plain paths`);

function showMcPrice(data, product) {
  const reference = product.reference(data);
  mcDigits = digitsFor(reference);
  $("mc-ref-label").textContent = product.referenceName;
  $("mc-ref").textContent = fmtPrice(reference);
  $("mc-ref-hint").textContent = product.referenceHint(data);
  for (const m of MC_METHODS) {
    const e = data.estimates[m.key];
    $(`mc-${m.key}`).textContent = fmtPrice(e.price);
    $(`mc-${m.key}-se`).textContent = `± ${fmtPrice(e.std_error)} · 95%: ${fmtPrice(e.ci_low)} – ${fmtPrice(e.ci_high)}`;
    $(`mc-${m.key}-speedup`).textContent = m.key === "plain"
      ? "the baseline: 1 path = 1 path"
      : `worth ${speedupText(e.speedup)}`;
    $(`mc-${m.key}`).classList.toggle("loss",
      product.exact && !(e.ci_low <= reference && reference <= e.ci_high));
  }
}

function showConvergence(data, product) {
  const cell = (e) => `${fmtPrice(e.price)} <span class="hint">± ${fmtPrice(e.std_error)}</span>`;
  const gain = (e) => (e.speedup === null ? "–" : `${e.speedup.toFixed(1)}×`);
  $("mc-table").querySelector("tbody").innerHTML = data.rows.map((r) => `
    <tr>
      <td>${r.paths.toLocaleString("en-US")}</td>
      <td>${cell(r.estimates.plain)}</td>
      <td>${cell(r.estimates.antithetic)}</td>
      <td>${cell(r.estimates.control_variate)}</td>
      <td>${gain(r.estimates.antithetic)} / ${gain(r.estimates.control_variate)}</td>
    </tr>`).join("")
    + `<tr><td colspan="5" class="hint">${product.referenceName}: ${fmtPrice(product.reference(data))} · `
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
          label: (item) => `${item.dataset.label}: ${fmtPrice(item.parsed.y)}`,
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
  const option = {
    ...m, type: p.type, seed: p.seed, ...(key === "asian" ? { fixings: p.fixings, yield: p.yield } : {}),
  };
  if (key === "asian") refreshTreasurer(m, p);
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

function showTreasurer(d) {
  $("tr-fair").textContent = d.fair_premium.toFixed(5);
  $("tr-fair-hint").textContent = "USD per AUD: what the put is worth, by simulation";
  $("tr-cost").textContent = millions(d.premium_cost);
  $("tr-cost-hint").textContent = `${d.premium} USD per AUD, plus a year's interest at r`;
  $("tr-floor").textContent = millions(d.floor);
  $("tr-floor-hint").textContent = "notional × K − premium paid. Worst simulated year: "
    + `${millions(d.worst.hedged)} hedged, ${millions(d.worst.unhedged)} unhedged`;
  $("tr-pnl").textContent = signedMillions(d.mean.put_pnl);
  $("tr-pnl").classList.toggle("loss", d.mean.put_pnl < 0);
  $("tr-pnl").classList.toggle("gain", d.mean.put_pnl > 0);
  $("tr-pnl-hint").textContent = `the put paid out in ${Math.round(d.probability_put_pays * 100)}% `
    + "of years; at the fair premium it breaks even on average";

  const pnlCell = (x) => `<td class="${x < 0 ? "loss" : "gain"}">${signedMillions(x)}</td>`;
  $("tr-table").querySelector("tbody").innerHTML = d.scenarios.map((r) => `
    <tr>
      <td>${TREASURER_YEARS[r.percentile]} <span class="hint">${r.percentile}th pct</span></td>
      <td>${r.average_rate.toFixed(4)}</td>
      <td>${millions(r.unhedged)}</td>
      <td>${millions(r.put_payoff)}</td>
      ${pnlCell(r.put_pnl)}
      <td>${millions(r.hedged)}</td>
    </tr>`).join("")
    + `<tr><td colspan="6" class="hint">${d.paths.toLocaleString("en-US")} simulated years · `
    + `seed ${d.seed} · rows are percentiles of the average rate</td></tr>`;
  drawTreasurerChart(d.histogram);
}

function drawTreasurerChart(h) {
  const ink = color("--muted");
  const labels = h.hedged.map((_, i) => ((h.edges[i] + h.edges[i + 1]) / 2 / 1e6).toFixed(1));
  const bars = (label, counts, colorVar) => ({
    label, data: counts, backgroundColor: `${color(colorVar)}b3`, borderWidth: 0,
    barPercentage: 1, categoryPercentage: 0.9,
  });
  if (trChart) trChart.destroy();
  trChart = new Chart($("tr-chart"), {
    type: "bar",
    data: {
      labels,
      datasets: [bars("Unhedged", h.unhedged, "--put"), bars("Hedged with the put", h.hedged, "--gain")],
    },
    options: {
      animation: false,
      maintainAspectRatio: false,
      plugins: {
        legend: { labels: { color: ink } },
        tooltip: { callbacks: { title: (items) => `≈ $${items[0].label}M` } },
      },
      scales: {
        x: {
          title: { display: true, text: "USD received over the year ($ million)", color: ink },
          ticks: { color: ink, maxRotation: 0, autoSkip: true, maxTicksLimit: 10 },
          grid: { display: false },
        },
        y: {
          title: { display: true, text: "Simulated years", color: ink },
          ticks: { color: ink },
          grid: { color: color("--grid") },
        },
      },
    },
  });
}

async function refreshTreasurer(m, p) {
  try {
    const data = await api("/api/v1/mc/asian/hedge", {
      ...m,
      fixings: p.fixings,
      yield: p.yield,
      seed: p.seed,
      notional: parseFloat($("tr-notional").value) * 1e6,
      premium: parseFloat($("tr-premium").value),
      paths: parseInt($("tr-paths").value, 10),
    });
    if (mcProduct !== "asian") return;
    showTreasurer(data);
    showError($("tr-error"), "");
  } catch (e) {
    for (const id of ["tr-fair", "tr-cost", "tr-floor", "tr-pnl"]) $(id).textContent = "–";
    showError($("tr-error"), e.message);
  }
}

function loadTreasurer() {
  for (const [id, value] of Object.entries(TREASURER_INPUTS)) $(id).value = value;
  $("mc-type").value = "put";
  $("mc-fixings").value = 12;
  $("mc-yield").value = 3.5;
  $("tr-notional").value = 100;
  $("tr-premium").value = 0.016;
  $("inputs-hint").textContent = "Illustrative AUD/USD for Hull's treasurer (26.13). Shared by every tab.";
  scheduleRefresh();
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
  for (const id of ["mc-paths", "mc-seed", "mc-fixings", "mc-yield", "tr-notional", "tr-premium", "tr-paths"]) {
    $(id).addEventListener("input", scheduleRefresh);
  }
  $("tr-load").addEventListener("click", loadTreasurer);
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
