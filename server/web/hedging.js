// Hedging tab. Display only: every number comes from /api/v1/hedge/*.
// Uses $, api, color, showError and marketInputs from app.js (loaded after this file).
"use strict";

// Hull Tables 19.2 and 19.3: his weekly stock prices, as data from the book.
const HULL_PATHS = {
  hull192: {
    label: "Hull Table 19.2 path (ends in the money)",
    prices: [49.00, 48.12, 47.37, 50.25, 51.75, 53.12, 53.00, 51.87, 51.38, 53.00, 49.88,
             48.50, 49.88, 50.37, 52.13, 51.88, 52.87, 54.87, 54.62, 55.87, 57.25],
  },
  hull193: {
    label: "Hull Table 19.3 path (ends out of the money)",
    prices: [49.00, 49.75, 52.00, 50.00, 48.38, 48.25, 48.75, 49.63, 48.25, 48.25, 51.12,
             51.50, 49.88, 49.88, 48.75, 47.50, 48.00, 46.25, 48.13, 46.63, 48.12],
  },
};

// Hull 19.1's option: S=49, K=50, r=5%, σ=20%, T=20 weeks (0.3846 years = 140.379 days).
const HULL_INPUTS = { spot: 49, strike: 50, days: 140.379, rate: 5, vol: 20 };

let hedgeMode = "simulated";  // or a key of HULL_PATHS
const hedgeCharts = {};

function hedgeParams() {
  return {
    quantity: parseFloat($("h-quantity").value),
    premium: parseFloat($("h-premium").value),
    drift: parseFloat($("h-drift").value) / 100,
    realVol: parseFloat($("h-real-vol").value) / 100,
    costRate: parseFloat($("h-cost-rate").value) / 100,
    rebalances: parseInt($("h-rebalances").value, 10),
    seed: parseInt($("h-seed").value, 10),
  };
}

const usd = (x) => `$${Math.round(x).toLocaleString("en-US")}`;
const signedUsd = (x) => `${x < 0 ? "−" : "+"}${usd(Math.abs(x))}`;
// Hull's style: thousands with one decimal, negatives in parentheses.
const thousands = (x) => {
  const v = (Math.abs(x) / 1000).toLocaleString("en-US", { minimumFractionDigits: 1, maximumFractionDigits: 1 });
  return x < -0.05 ? `(${v})` : v;
};
const shares = (x) => {
  const v = Math.round(Math.abs(x)).toLocaleString("en-US");
  return x < -0.5 ? `(${v})` : v;
};

function weeks(timeInYears) {
  const w = timeInYears * 52;
  return Math.abs(w - Math.round(w)) < 0.01 ? String(Math.round(w)) : w.toFixed(2);
}

function showHedgeResult(data, m) {
  $("h-cost").textContent = usd(data.pv_hedge_cost);
  $("h-cost-note").textContent = data.total_trading_cost > 0
    ? `incl. ${usd(data.total_trading_cost)} of trading costs`
    : "what hedging cost the bank on this path";
  $("h-bs").textContent = usd(data.bs_value);
  $("h-fair").textContent = `fair value: ${(data.bs_value / hedgeParams().quantity).toFixed(2)}`;
  $("h-premium-out").textContent = usd(data.premium_received);
  $("h-pnl").textContent = signedUsd(data.pnl);
  $("h-pnl").classList.toggle("gain", data.pnl >= 0);
  $("h-pnl").classList.toggle("loss", data.pnl < 0);
  const last = data.steps[data.steps.length - 1];
  $("h-outcome").textContent = data.exercised
    ? `Exercised: S_T = ${last.spot.toFixed(2)} > K = ${m.strike}`
    : `Expired worthless: S_T = ${last.spot.toFixed(2)} ≤ K = ${m.strike}`;

  $("hedge-table").querySelector("tbody").innerHTML = data.steps.map((s) => `
    <tr>
      <td>${weeks(s.time)}</td><td>${s.spot.toFixed(2)}</td><td>${s.delta.toFixed(3)}</td>
      <td>${shares(s.shares_bought)}</td><td>${thousands(s.purchase_cost)}</td>
      <td>${thousands(s.trading_cost)}</td>
      <td>${thousands(s.cumulative_cost)}</td>
      <td>${s === last ? "" : thousands(s.interest)}</td>
    </tr>`).join("");

  drawHedgeChart(data, m);
}

function drawHedgeChart(data, m) {
  const ink = color("--muted");
  const grid = color("--grid");
  const xs = data.steps.map((s) => s.time * 52);
  const datasets = [
    {
      label: "Stock price",
      data: data.steps.map((s, i) => ({ x: xs[i], y: s.spot })),
      borderColor: color("--call"),
      backgroundColor: color("--call"),
      borderWidth: 2,
      pointRadius: data.steps.length <= 60 ? 3 : 0,
      pointStyle: "circle",
      yAxisID: "y",
    },
    {
      label: "Strike K",
      data: [{ x: xs[0], y: m.strike }, { x: xs[xs.length - 1], y: m.strike }],
      borderColor: color("--payoff"),
      backgroundColor: color("--payoff"),
      borderWidth: 1.25,
      borderDash: [5, 4],
      pointRadius: 0,
      pointStyle: "line",
      yAxisID: "y",
    },
    {
      label: "Shares held",
      data: data.steps.map((s, i) => ({ x: xs[i], y: s.shares_held })),
      borderColor: color("--put"),
      backgroundColor: color("--put"),
      borderWidth: 2,
      stepped: "before",
      pointRadius: 0,
      pointStyle: "line",
      yAxisID: "y1",
    },
  ];
  const options = {
    animation: false,
    maintainAspectRatio: false,
    interaction: { mode: "index", intersect: false },
    plugins: {
      legend: { labels: { color: ink, usePointStyle: true } },
      tooltip: {
        filter: (item) => item.dataset.label !== "Strike K",
        callbacks: {
          title: (items) => `Week ${weeks(items[0].parsed.x / 52)}`,
          label: (item) => item.dataset.label === "Shares held"
            ? `Shares held: ${Math.round(item.parsed.y).toLocaleString("en-US")}`
            : `Stock price: ${item.parsed.y.toFixed(2)}`,
        },
      },
    },
    scales: {
      x: {
        type: "linear",
        min: xs[0],
        max: xs[xs.length - 1],
        title: { display: true, text: "Week (time passes →)", color: ink },
        ticks: { color: ink },
        grid: { display: false },
      },
      y: {
        position: "left",
        title: { display: true, text: "Stock price", color: ink },
        ticks: { color: ink },
        grid: { color: grid },
      },
      y1: {
        position: "right",
        min: 0,
        suggestedMax: hedgeParams().quantity,
        title: { display: true, text: "Shares held", color: ink },
        ticks: { color: ink, callback: (v) => v.toLocaleString("en-US") },
        grid: { display: false },
      },
    },
  };
  if (hedgeCharts.path) hedgeCharts.path.destroy();
  hedgeCharts.path = new Chart($("hedge-chart"), { type: "line", data: { datasets }, options });
}

async function refreshHedge(m) {
  const p = hedgeParams();
  const body = { ...m, quantity: p.quantity, premium: p.premium, cost_rate: p.costRate };
  if (hedgeMode === "simulated") {
    Object.assign(body, { drift: p.drift, real_vol: p.realVol, rebalances: p.rebalances,
                          seed: p.seed });
    $("h-source").textContent = `Simulated path · seed ${p.seed} · real drift μ = `
      + `${(p.drift * 100).toFixed(1)}% · realised vol ${(p.realVol * 100).toFixed(1)}% `
      + `(hedged at implied ${(m.vol * 100).toFixed(1)}%) · ${p.rebalances} rebalances`;
  } else {
    body.prices = HULL_PATHS[hedgeMode].prices;
    $("h-source").textContent = `${HULL_PATHS[hedgeMode].label} · 20 weekly rebalances`;
  }
  try {
    showHedgeResult(await api("/api/v1/hedge/path", body), m);
    showError($("h-error"), "");
  } catch (e) {
    showError($("h-error"), e.message);
  }
}

async function runFrequencyStudy() {
  const m = marketInputs();
  const p = hedgeParams();
  const button = $("f-run");
  button.disabled = true;
  button.textContent = "Running…";
  try {
    const data = await api("/api/v1/hedge/frequency", {
      ...m, quantity: p.quantity, drift: p.drift, real_vol: p.realVol, seed: p.seed,
      cost_rate: p.costRate, paths: parseInt($("f-paths").value, 10),
    });
    const totalWeeks = m.expiry * 52;
    const label = (n) => `${(totalWeeks / n).toFixed(2).replace(/\.?0+$/, "")} wk`;
    $("freq-table").querySelector("tbody").innerHTML = data.rows.map((r) => `
      <tr>
        <td>${label(r.rebalances)} <span class="hint">(${r.rebalances} rebalances)</span></td>
        <td>${usd(r.mean_pv_cost)}</td><td>${usd(r.sd_pv_cost)}</td>
        <td>${r.sd_over_bs.toFixed(2)}</td>
      </tr>`).join("")
      + `<tr><td colspan="4" class="hint">Black-Scholes value: ${usd(data.bs_value)} · `
      + `${data.paths.toLocaleString("en-US")} paths per row · seed ${data.seed}</td></tr>`;
    $("freq-chart-box").hidden = false;
    drawFrequencyChart(data, label);
    showError($("f-error"), "");
  } catch (e) {
    showError($("f-error"), e.message);
  } finally {
    button.disabled = false;
    button.textContent = "Run study";
  }
}

function drawFrequencyChart(data, label) {
  const ink = color("--muted");
  const options = {
    animation: false,
    maintainAspectRatio: false,
    plugins: {
      legend: { display: false },
      tooltip: { callbacks: { label: (item) => `Std dev ÷ BS: ${item.parsed.y.toFixed(3)}` } },
    },
    scales: {
      x: { title: { display: true, text: "Rebalance every", color: ink }, ticks: { color: ink },
           grid: { display: false } },
      y: { beginAtZero: true, title: { display: true, text: "Std dev ÷ BS value", color: ink },
           ticks: { color: ink }, grid: { color: color("--grid") } },
    },
  };
  if (hedgeCharts.freq) hedgeCharts.freq.destroy();
  hedgeCharts.freq = new Chart($("freq-chart"), {
    type: "bar",
    data: {
      labels: data.rows.map((r) => label(r.rebalances)),
      datasets: [{ data: data.rows.map((r) => r.sd_over_bs), backgroundColor: color("--call"),
                   borderRadius: 4 }],
    },
    options,
  });
}

async function runVolStudy() {
  const m = marketInputs();
  const p = hedgeParams();
  const button = $("v-run");
  button.disabled = true;
  button.textContent = "Running…";
  try {
    const data = await api("/api/v1/hedge/vol-study", {
      ...m, quantity: p.quantity, premium: p.premium, drift: p.drift, rebalances: p.rebalances,
      seed: p.seed, cost_rate: p.costRate, paths: parseInt($("v-paths").value, 10),
    });
    const pct = (v) => `${(v * 100).toFixed(1)}%`;
    $("v-summary").textContent = data.premium_implied_vol === null
      ? `Sold at implied ${pct(data.implied_vol)}.`
      : `Sold at implied ${pct(data.implied_vol)}; the premium of ${p.premium} per option is the `
        + `Black-Scholes price at ${pct(data.premium_implied_vol)} vol, so the bank breaks even `
        + `when the stock really moves at about that vol.`;
    $("vol-table").querySelector("tbody").innerHTML = data.rows.map((r) => `
      <tr>
        <td>${pct(r.real_vol)}${Math.abs(r.real_vol - data.implied_vol) < 1e-9 ? " <span class=\"hint\">(= implied)</span>" : ""}</td>
        <td>${usd(r.mean_pv_cost)}</td><td>${usd(r.sd_pv_cost)}</td>
        <td class="${r.mean_pnl >= 0 ? "gain" : "loss"}">${signedUsd(r.mean_pnl)}</td>
      </tr>`).join("")
      + `<tr><td colspan="4" class="hint">Premium received: ${usd(data.premium_received)} · `
      + `Black-Scholes value at implied vol: ${usd(data.bs_value)} · `
      + `${data.paths.toLocaleString("en-US")} paths per row · ${data.rebalances} rebalances</td></tr>`;
    $("vol-chart-box").hidden = false;
    drawVolChart(data, pct);
    showError($("v-error"), "");
  } catch (e) {
    showError($("v-error"), e.message);
  } finally {
    button.disabled = false;
    button.textContent = "Run study";
  }
}

function drawVolChart(data, pct) {
  const ink = color("--muted");
  const options = {
    animation: false,
    maintainAspectRatio: false,
    plugins: {
      legend: { display: false },
      tooltip: { callbacks: { label: (item) => `Mean bank P&L: ${signedUsd(item.parsed.y)}` } },
    },
    scales: {
      x: { title: { display: true, text: "Realised vol (stock really moved at)", color: ink },
           ticks: { color: ink }, grid: { display: false } },
      y: { title: { display: true, text: "Mean bank P&L", color: ink },
           ticks: { color: ink, callback: (v) => signedUsd(v) },
           grid: { color: (ctx) => (ctx.tick.value === 0 ? ink : color("--grid")) } },
    },
  };
  if (hedgeCharts.vol) hedgeCharts.vol.destroy();
  hedgeCharts.vol = new Chart($("vol-chart"), {
    type: "bar",
    data: {
      labels: data.rows.map((r) => pct(r.real_vol)),
      datasets: [{
        data: data.rows.map((r) => r.mean_pnl),
        backgroundColor: data.rows.map((r) => color(r.mean_pnl >= 0 ? "--gain" : "--error")),
        borderRadius: 4,
      }],
    },
    options,
  });
}

function loadHull(key) {
  hedgeMode = key;
  for (const [id, value] of Object.entries(HULL_INPUTS)) $(id).value = value;
  $("h-rebalances").value = 20;
  $("h-premium").value = 3;
  $("h-real-vol").value = 20;
  $("h-cost-rate").value = 0;
  refreshHedge(marketInputs());
}

// Called by app.js once its helpers exist.
function initHedging() {
  $("h-new").addEventListener("click", () => {
    hedgeMode = "simulated";
    $("h-seed").value = Math.floor(Math.random() * 1e9);
    refreshHedge(marketInputs());
  });
  $("h-hull192").addEventListener("click", () => loadHull("hull192"));
  $("h-hull193").addEventListener("click", () => loadHull("hull193"));
  for (const id of ["h-drift", "h-real-vol", "h-quantity", "h-rebalances", "h-premium",
                    "h-cost-rate", "h-seed"]) {
    $(id).addEventListener("input", () => {
      hedgeMode = "simulated";
      scheduleRefresh();
    });
  }
  $("f-run").addEventListener("click", runFrequencyStudy);
  $("v-run").addEventListener("click", runVolStudy);
}
