// Display only: reads the form, calls the C++ API, draws the results. No pricing here.
"use strict";

const $ = (id) => document.getElementById(id);
const fields = ["spot", "strike", "days", "rate", "vol"].map($);
let axis = "spot";
Chart.defaults.font.family = getComputedStyle(document.body).fontFamily;
let chart = null;

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
  return { spot: "Spot price S", vol: "Volatility σ", expiry: "Time to expiry (years)" }[axis];
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

function drawChart(sweep, current, prices) {
  const xy = (ys) => sweep.x.map((x, i) => ({ x, y: ys[i] }));
  const datasets = [
    line("Call", xy(sweep.call), "--call", false),
    line("Put", xy(sweep.put), "--put", false),
  ];
  if (axis === "spot") {
    datasets.push(line("Call payoff at expiry", xy(sweep.call_payoff), "--payoff", true));
    datasets.push(line("Put payoff at expiry", xy(sweep.put_payoff), "--payoff", true));
  }
  datasets.push({
    type: "scatter",
    label: "Current",
    data: [{ x: current, y: prices.call }, { x: current, y: prices.put }],
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
          title: (items) => `${axisLabel()}: ${percent ? (items[0].parsed.x * 100).toFixed(1) + "%"
                                                        : items[0].parsed.x.toFixed(2)}`,
          label: (item) => `${item.dataset.label}: ${item.parsed.y.toFixed(4)}`,
        },
      },
    },
    scales: {
      x: {
        type: "linear",
        title: { display: true, text: axisLabel(), color: ink },
        ticks: { color: ink, callback: (v) => (percent ? `${Math.round(v * 100)}%` : v) },
        grid: { display: false },
      },
      y: {
        title: { display: true, text: "Option price", color: ink },
        ticks: { color: ink },
        grid: { color: grid },
        beginAtZero: true,
      },
    },
  };

  if (chart) chart.destroy();
  chart = new Chart($("chart"), { type: "line", data: { datasets }, options });
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

async function refresh() {
  const m = marketInputs();
  try {
    const [call, put] = await Promise.all([
      api("/api/v1/price", { ...m, type: "call" }),
      api("/api/v1/price", { ...m, type: "put" }),
    ]);
    $("call-price").textContent = call.price.toFixed(4);
    $("put-price").textContent = put.price.toFixed(4);
    showError($("error"), "");

    const sweep = await api("/api/v1/sweep", { ...m, vary: axis, ...sweepRange(m), steps: 121 });
    const current = { spot: m.spot, vol: m.vol, expiry: m.expiry }[axis];
    drawChart(sweep, current, { call: call.price, put: put.price });
  } catch (e) {
    $("call-price").textContent = "–";
    $("put-price").textContent = "–";
    showError($("error"), e.message);
  }
  refreshIv(m);
}

let timer = null;
function scheduleRefresh() {
  clearTimeout(timer);
  timer = setTimeout(refresh, 150);
}

fields.forEach((f) => f.addEventListener("input", scheduleRefresh));
$("iv-type").addEventListener("change", () => refreshIv(marketInputs()));
$("iv-price").addEventListener("input", () => refreshIv(marketInputs()));
document.querySelectorAll(".segmented button").forEach((btn) => {
  btn.addEventListener("click", () => {
    axis = btn.dataset.axis;
    document.querySelectorAll(".segmented button").forEach((b) =>
      b.setAttribute("aria-selected", String(b === btn)));
    refresh();
  });
});
matchMedia("(prefers-color-scheme: dark)").addEventListener("change", refresh);

refresh();
