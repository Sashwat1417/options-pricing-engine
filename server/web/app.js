// Display only: reads the form, calls the C++ API, draws the results. No pricing here.
"use strict";

const $ = (id) => document.getElementById(id);
const fields = ["spot", "strike", "days", "rate", "vol"].map($);
const VIEWS = ["pricing", "greeks", "iv"];
let view = "pricing";
let axis = "spot";
Chart.defaults.font.family = getComputedStyle(document.body).fontFamily;
const charts = {};

// Chart per view. A new Greek becomes one more entry and its own chart or selector.
const CHARTS = {
  pricing: {
    canvas: "price-chart",
    yLabel: "Option price",
    call: "call",
    put: "put",
    suffix: "",
    payoffs: true,
    yRange: {},
  },
  greeks: {
    canvas: "greek-chart",
    yLabel: "Delta Δ (∂price/∂S)",
    call: "call_delta",
    put: "put_delta",
    suffix: " Δ",
    payoffs: false,
    yRange: { min: -1, max: 1 },
  },
};

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
  if (axis === "spot") return { from: Math.max(m.strike * 0.4, 0.01), to: m.strike * 1.6 };
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
  const spec = CHARTS[name];
  const xy = (ys) => sweep.x.map((x, i) => ({ x: toAxisUnits(x), y: ys[i] }));
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
          label: (item) => `${item.dataset.label}: ${item.parsed.y.toFixed(4)}`,
        },
      },
    },
    scales: {
      x: {
        type: "linear",
        // Expiry reads left to right as time passing: far from expiry -> expiry (0 days).
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
        // Highlight zero on the Greeks chart: calls sit above it, puts below.
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

const BLANK = { "call-price": "–", "put-price": "–", "call-delta": "–", "put-delta": "–" };

async function refresh() {
  const m = marketInputs();
  if (view === "iv") {
    showError($("error"), "");
    return refreshIv(m);
  }
  try {
    const [callPrice, putPrice, callGreeks, putGreeks] = await Promise.all([
      api("/api/v1/price", { ...m, type: "call" }),
      api("/api/v1/price", { ...m, type: "put" }),
      api("/api/v1/greeks", { ...m, type: "call" }),
      api("/api/v1/greeks", { ...m, type: "put" }),
    ]);
    setReadouts({
      "call-price": callPrice.price.toFixed(4),
      "put-price": putPrice.price.toFixed(4),
      "call-delta": callGreeks.delta.toFixed(4),
      "put-delta": putGreeks.delta.toFixed(4),
    });
    showError($("error"), "");

    // Chart.js can't size a hidden canvas, so only the visible view's chart is drawn.
    const sweep = await api("/api/v1/sweep", { ...m, vary: axis, ...sweepRange(m), steps: 121 });
    const current = { spot: m.spot, vol: m.vol, expiry: m.expiry }[axis];
    const now = view === "pricing"
      ? { call: callPrice.price, put: putPrice.price }
      : { call: callGreeks.delta, put: putGreeks.delta };
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

fields.forEach((f) => f.addEventListener("input", scheduleRefresh));
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

document.querySelectorAll(".tabs button").forEach((btn) => {
  btn.addEventListener("click", () => {
    try { history.replaceState(null, "", `#${btn.dataset.view}`); } catch (e) {}
    showView(btn.dataset.view);
  });
});
matchMedia("(prefers-color-scheme: dark)").addEventListener("change", refresh);

// Deep link to a tab: /#pricing, /#greeks, /#iv.
showView(location.hash.slice(1));
