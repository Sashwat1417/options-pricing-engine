// Display only: reads the form, calls the C++ API, draws the results. No pricing here.
"use strict";

const $ = (id) => document.getElementById(id);
const fields = ["spot", "strike", "days", "rate", "vol"].map($);
const VIEWS = ["pricing", "greeks", "iv", "hedging"];
let view = "pricing";
let axis = "spot";
let greek = "delta";
Chart.defaults.font.family = getComputedStyle(document.body).fontFamily;
const charts = {};

// One entry per Greek sub-tab. A new Greek is one more entry plus a sub-tab button.
// `scale` converts the API's units for display (theta: per year -> per day).
const GREEKS = {
  delta: {
    name: "Delta Δ",
    symbol: "Δ",
    key: "delta",
    scale: 1,
    digits: 4,
    yLabel: "Delta Δ (∂price/∂S)",
    yRange: { min: -1, max: 1 },
    hints: { call: "shares to hold per call sold", put: "shares to short per put sold" },
    note: "Δ = ∂price/∂S, the slope of the price curve. Δ<sub>put</sub> = Δ<sub>call</sub> − 1.",
  },
  theta: {
    name: "Theta Θ",
    symbol: "Θ",
    key: "theta",
    scale: 1 / 365,
    digits: 5,
    yLabel: "Theta Θ (price change per day)",
    yRange: {},
    hints: {
      call: "price change per day as time passes",
      put: "price change per day as time passes",
    },
    note: "Θ = ∂price/∂t, shown per calendar day (Hull quotes it per year: × 365). "
      + "Negative means the option loses value each day; time passing is certain, so it is a "
      + "known cost, not a risk to hedge.",
  },
  gamma: {
    name: "Gamma Γ",
    symbol: "Γ",
    key: "gamma",
    scale: 1,
    digits: 5,
    yLabel: "Gamma Γ (∂Δ/∂S)",
    yRange: { min: 0 },
    hints: {
      call: "change in Δ per ₹1 move in S",
      put: "change in Δ per ₹1 move in S (same as the call)",
    },
    note: "Γ = ∂Δ/∂S = ∂²price/∂S², how fast delta changes, i.e. how much the price curve bends. "
      + "Largest at the money and near expiry, where a delta hedge goes stale fastest. Shares have "
      + "Γ = 0, so only options can hedge it.",
  },
  vega: {
    name: "Vega ν",
    symbol: "ν",
    key: "vega",
    scale: 1 / 100,
    digits: 4,
    yLabel: "Vega ν (price change per 1% vol)",
    yRange: { min: 0 },
    hints: {
      call: "price change if implied vol rises 1%",
      put: "price change if implied vol rises 1% (same as the call)",
    },
    note: "ν = ∂price/∂σ, shown per 1 percentage point of volatility (the API returns it per "
      + "unit of σ: × 100). Largest at the money and for long-dated options. Shares have "
      + "ν = 0, so only options can hedge it.",
  },
  rho: {
    name: "Rho ρ",
    symbol: "ρ",
    key: "rho",
    scale: 1 / 100,
    digits: 4,
    yLabel: "Rho ρ (price change per 1% rate)",
    yRange: {},
    hints: {
      call: "price change if the risk-free rate rises 1%",
      put: "price change if the risk-free rate rises 1%",
    },
    note: "ρ = ∂price/∂r, shown per 1 percentage point of interest rate (the API returns it per "
      + "unit of r: × 100). Calls gain when rates rise (the strike you pay later is worth less "
      + "today); puts lose. Small for short-dated options, since it grows with T.",
  },
};

// The chart for each view, built from the active Greek on the Greeks tab.
function chartSpec(name) {
  if (name === "pricing") {
    return { canvas: "price-chart", yLabel: "Option price", call: "call", put: "put", scale: 1,
             suffix: "", payoffs: true, yRange: {} };
  }
  const g = GREEKS[greek];
  return { canvas: "greek-chart", yLabel: g.yLabel, call: `call_${g.key}`, put: `put_${g.key}`,
           scale: g.scale, suffix: ` ${g.symbol}`, payoffs: false, yRange: g.yRange };
}

// Form units -> API units: days -> years, % -> decimal.
function marketInputs() {
  return {
    spot: parseFloat($("spot").value),
    strike: parseFloat($("strike").value),
    expiry: parseFloat($("days").value) / 365,
    rate: parseFloat($("rate").value) / 100,
    vol: parseFloat($("vol").value) / 100,
  };
}

async function api(path, body) {
  const res = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  const data = await res.json();
  if (!res.ok) throw new Error(data.error || `HTTP ${res.status}`);
  return data;
}

function showError(el, message) {
  el.textContent = message || "";
  el.hidden = !message;
}

function sweepRange(m) {
  if (axis === "spot") return { from: m.strike * 0.01, to: m.strike * 3 };
  if (axis === "vol") return { from: 0.01, to: 1.0 };
  return { from: 1 / 365, to: 2.0 };
}

function color(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}

function axisLabel() {
  return { spot: "Spot price S", vol: "Volatility σ", expiry: "Days to expiry (time passes →)" }[axis];
}

// API sends years; the expiry axis shows days, matching the input box.
function toAxisUnits(x) {
  return axis === "expiry" ? x * 365 : x;
}

function line(label, data, colorName, dashed) {
  return {
    label,
    data,
    borderColor: color(colorName),
    backgroundColor: color(colorName),
    borderWidth: dashed ? 1.25 : 2,
    borderDash: dashed ? [5, 4] : [],
    pointRadius: 0,
    pointStyle: "line",
    tension: 0,
  };
}

// current: the x of today's inputs; now: {call, put} values of this chart's series there.
function drawChart(name, sweep, current, now) {
  const spec = chartSpec(name);
  const xy = (ys) => sweep.x.map((x, i) => ({ x: toAxisUnits(x), y: ys[i] * spec.scale }));
  const datasets = [
    line(`Call${spec.suffix}`, xy(sweep[spec.call]), "--call", false),
    line(`Put${spec.suffix}`, xy(sweep[spec.put]), "--put", false),
  ];
  if (spec.payoffs && axis === "spot") {
    datasets.push(line("Call payoff at expiry", xy(sweep.call_payoff), "--payoff", true));
    datasets.push(line("Put payoff at expiry", xy(sweep.put_payoff), "--payoff", true));
  }
  datasets.push({
    type: "scatter",
    label: "Current",
    data: [{ x: toAxisUnits(current), y: now.call }, { x: toAxisUnits(current), y: now.put }],
    backgroundColor: [color("--call"), color("--put")],
    borderColor: color("--surface"),
    borderWidth: 2,
    pointRadius: 6,
  });

  const ink = color("--muted");
  const grid = color("--grid");
  const percent = axis === "vol";
  const options = {
    animation: false,
    maintainAspectRatio: false,
    interaction: { mode: "nearest", axis: "x", intersect: false },
    plugins: {
      legend: {
        labels: { color: ink, usePointStyle: true, filter: (item) => item.text !== "Current" },
      },
      tooltip: {
        callbacks: {
          title: (items) => {
            const x = items[0].parsed.x;
            if (percent) return `Volatility σ: ${(x * 100).toFixed(1)}%`;
            if (axis === "expiry") return `Days to expiry: ${x.toFixed(0)}`;
            return `Spot price S: ${x.toFixed(2)}`;
          },
          label: (item) => `${item.dataset.label}: ${item.parsed.y.toFixed(spec.payoffs ? 4 : 5)}`,
        },
      },
    },
    scales: {
      x: {
        type: "linear",
        reverse: axis === "expiry",
        min: toAxisUnits(sweep.x[0]),
        max: toAxisUnits(sweep.x[sweep.x.length - 1]),
        title: { display: true, text: axisLabel(), color: ink },
        ticks: { color: ink, callback: (v) => (percent ? `${Math.round(v * 100)}%` : v) },
        grid: { display: false },
      },
      y: {
        title: { display: true, text: spec.yLabel, color: ink },
        ticks: { color: ink },
        grid: { color: (ctx) => (ctx.tick.value === 0 && !spec.payoffs ? ink : grid) },
        beginAtZero: true,
        ...spec.yRange,
      },
    },
  };

  if (charts[name]) charts[name].destroy();
  charts[name] = new Chart($(spec.canvas), { type: "line", data: { datasets }, options });
}

async function refreshIv(m) {
  try {
    const { implied_vol } = await api("/api/v1/iv", {
      ...m,
      type: $("iv-type").value,
      price: parseFloat($("iv-price").value),
    });
    $("iv-result").textContent = `${(implied_vol * 100).toFixed(2)}%`;
    showError($("iv-error"), "");
  } catch (e) {
    $("iv-result").textContent = "–";
    showError($("iv-error"), e.message);
  }
}

function setReadouts(text) {
  for (const [id, value] of Object.entries(text)) $(id).textContent = value;
}

const BLANK = { "call-price": "–", "put-price": "–", "call-greek": "–", "put-greek": "–" };

function showGreekLabels() {
  const g = GREEKS[greek];
  const lower = g.name.split(" ")[0].toLowerCase();
  const symbol = `<span class="symbol">${g.symbol}</span>`;
  $("call-greek-label").innerHTML = `Call ${lower} ${symbol}`;
  $("put-greek-label").innerHTML = `Put ${lower} ${symbol}`;
  $("call-greek-hint").textContent = g.hints.call;
  $("put-greek-hint").textContent = g.hints.put;
  $("greek-chart-title").textContent = `${g.name} as one input varies`;
  $("greek-note").innerHTML = g.note;
}

async function refresh() {
  const m = marketInputs();
  if (view === "iv") {
    showError($("error"), "");
    return refreshIv(m);
  }
  if (view === "hedging") {
    showError($("error"), "");
    return refreshHedge(m);
  }
  try {
    const [callPrice, putPrice, callGreeks, putGreeks] = await Promise.all([
      api("/api/v1/price", { ...m, type: "call" }),
      api("/api/v1/price", { ...m, type: "put" }),
      api("/api/v1/greeks", { ...m, type: "call" }),
      api("/api/v1/greeks", { ...m, type: "put" }),
    ]);
    const g = GREEKS[greek];
    setReadouts({
      "call-price": callPrice.price.toFixed(4),
      "put-price": putPrice.price.toFixed(4),
      "call-greek": (callGreeks[g.key] * g.scale).toFixed(g.digits),
      "put-greek": (putGreeks[g.key] * g.scale).toFixed(g.digits),
    });
    showError($("error"), "");

    const sweep = await api("/api/v1/sweep", { ...m, vary: axis, ...sweepRange(m), steps: 301 });
    const current = { spot: m.spot, vol: m.vol, expiry: m.expiry }[axis];
    const now = view === "pricing"
      ? { call: callPrice.price, put: putPrice.price }
      : { call: callGreeks[g.key] * g.scale, put: putGreeks[g.key] * g.scale };
    drawChart(view, sweep, current, now);
  } catch (e) {
    setReadouts(BLANK);
    showError($("error"), e.message);
  }
}

let timer = null;
function scheduleRefresh() {
  clearTimeout(timer);
  timer = setTimeout(refresh, 150);
}

function showView(name) {
  view = VIEWS.includes(name) ? name : "pricing";
  document.querySelectorAll(".tabs button").forEach((b) =>
    b.setAttribute("aria-selected", String(b.dataset.view === view)));
  for (const v of VIEWS) $(`view-${v}`).hidden = v !== view;
  refresh();
}

// Editing the option leaves Hull's path (it only fits Hull's inputs) for a simulated one.
fields.forEach((f) => f.addEventListener("input", () => {
  hedgeMode = "simulated";
  scheduleRefresh();
}));
initHedging();
$("iv-type").addEventListener("change", () => refreshIv(marketInputs()));
$("iv-price").addEventListener("input", () => refreshIv(marketInputs()));

// The Spot / Volatility / Expiry choice is shared by both charts.
document.querySelectorAll(".segmented button").forEach((btn) => {
  btn.addEventListener("click", () => {
    axis = btn.dataset.axis;
    document.querySelectorAll(".segmented button").forEach((b) =>
      b.setAttribute("aria-selected", String(b.dataset.axis === axis)));
    refresh();
  });
});

document.querySelectorAll(".subtabs button").forEach((btn) => {
  btn.addEventListener("click", () => {
    greek = btn.dataset.greek;
    document.querySelectorAll(".subtabs button").forEach((b) =>
      b.setAttribute("aria-selected", String(b === btn)));
    try { history.replaceState(null, "", `#greeks-${greek}`); } catch (e) {}
    showGreekLabels();
    refresh();
  });
});

document.querySelectorAll(".tabs button").forEach((btn) => {
  btn.addEventListener("click", () => {
    try { history.replaceState(null, "", `#${btn.dataset.view}`); } catch (e) {}
    showView(btn.dataset.view);
  });
});
matchMedia("(prefers-color-scheme: dark)").addEventListener("change", refresh);

// Deep link to a tab or Greek: /#pricing, /#greeks, /#greeks-theta, /#iv, /#hedging.
const [linkedView, linkedGreek] = location.hash.slice(1).split("-");
if (linkedGreek in GREEKS) {
  greek = linkedGreek;
  document.querySelectorAll(".subtabs button").forEach((b) =>
    b.setAttribute("aria-selected", String(b.dataset.greek === greek)));
}
showGreekLabels();
showView(linkedView);
