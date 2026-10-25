"""Charts for the Black-Scholes pricer. Every number comes from the `ope` CLI; this only draws."""

import argparse
import csv
import io
import subprocess
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.ticker import PercentFormatter  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent

# Hull Ch 15 worked example.
HULL = {"spot": 42, "strike": 40, "expiry": 0.5, "rate": 0.1, "vol": 0.2}
HULL_LABEL = "K = 40, T = 0.5y, r = 10%, σ = 20%  (Hull Ch 15 example)"

THEMES = {
    "light": {
        "surface": "#fcfcfb", "ink": "#0b0b0b", "ink2": "#52514e", "muted": "#898781",
        "grid": "#e1e0d9", "axis": "#c3c2b7",
        "call": "#2a78d6", "put": "#eb6834",
        "ramp": ["#86b6ef", "#3987e5", "#1c5cab", "#0d366b"],
    },
    "dark": {
        "surface": "#1a1a19", "ink": "#f0efec", "ink2": "#c3c2b7", "muted": "#898781",
        "grid": "#2c2c2a", "axis": "#383835",
        "call": "#3987e5", "put": "#d95926",
        "ramp": ["#184f95", "#2a78d6", "#6da7ec", "#b7d3f6"],
    },
}


def sweep(ope, vary, start, stop, steps=241, **overrides):
    params = {**HULL, **overrides}
    args = [str(ope), "sweep", "--vary", vary, "--from", str(start), "--to", str(stop),
            "--steps", str(steps)]
    for key, value in params.items():
        args += [f"--{key}", str(value)]
    out = subprocess.run(args, check=True, capture_output=True, text=True).stdout
    rows = list(csv.DictReader(io.StringIO(out)))
    return {key: [float(row[key]) for row in rows] for key in rows[0]}


def new_figure(t, ncols=1):
    fig, axes = plt.subplots(1, ncols, figsize=(9, 4.4), facecolor=t["surface"], squeeze=False)
    return fig, list(axes[0])


def style(ax, t, xlabel, ylabel=None):
    ax.set_facecolor(t["surface"])
    ax.grid(axis="y", color=t["grid"], linewidth=0.8)
    ax.set_axisbelow(True)
    for side in ("top", "right", "left"):
        ax.spines[side].set_visible(False)
    ax.spines["bottom"].set_color(t["axis"])
    ax.tick_params(colors=t["muted"], length=0, labelsize=9)
    ax.set_xlabel(xlabel, color=t["ink2"], fontsize=10)
    if ylabel:
        ax.set_ylabel(ylabel, color=t["ink2"], fontsize=10)


def titles(fig, t, title, subtitle):
    fig.text(0.06, 0.95, title, color=t["ink"], fontsize=13, fontweight="bold", va="top")
    fig.text(0.06, 0.89, subtitle, color=t["ink2"], fontsize=9.5, va="top")


def legend(ax, t, **kwargs):
    leg = ax.legend(frameon=False, fontsize=9, labelcolor=t["ink2"], **kwargs)
    return leg


def price_vs_spot(ope, t):
    data = sweep(ope, "spot", 25, 60)
    fig, axes = new_figure(t, ncols=2)
    for ax, kind in zip(axes, ("call", "put")):
        style(ax, t, "Spot price S", "Option price" if kind == "call" else None)
        above = [p >= q for p, q in zip(data[kind], data[f"{kind}_payoff"])]
        ax.fill_between(data["spot"], data[f"{kind}_payoff"], data[kind], where=above,
                        interpolate=True, color=t[kind], alpha=0.14, linewidth=0,
                        label="Time value")
        ax.plot(data["spot"], data[f"{kind}_payoff"], color=t["ink2"], linewidth=1.2,
                linestyle=(0, (4, 3)), label="Payoff at expiry")
        ax.plot(data["spot"], data[kind], color=t[kind], linewidth=2, label="Price today")
        ax.set_title(kind.capitalize(), color=t["ink"], fontsize=11, loc="left")
        ax.set_ylim(bottom=0)
        legend(ax, t, loc="upper center" if kind == "call" else "upper right")

    hull = sweep(ope, "spot", 42, 42.000001, steps=2)
    for ax, kind in zip(axes, ("call", "put")):
        y = hull[kind][0]
        ax.plot([42], [y], "o", markersize=8, color=t[kind], markeredgecolor=t["surface"],
                markeredgewidth=2, zorder=5)
        offset, align = ((-12, 10), "right") if kind == "call" else ((10, 12), "left")
        ax.annotate(f"S = 42 → {y:.2f}", (42, y), xytext=offset, textcoords="offset points",
                    color=t["ink"], fontsize=9, ha=align)

    # Deep ITM European put: worth less than its payoff, since K arrives only at expiry.
    axes[1].annotate("Below payoff: a European put\ncan't be exercised early",
                     (27.6, 11.3), xytext=(32.5, 10.8), color=t["ink2"], fontsize=8.5,
                     va="center",
                     arrowprops={"arrowstyle": "-", "color": t["muted"], "linewidth": 0.8})

    titles(fig, t, "Price today vs payoff at expiry", HULL_LABEL)
    fig.subplots_adjust(top=0.78, bottom=0.14, left=0.07, right=0.98, wspace=0.12)
    return fig


def time_decay(ope, t):
    expiries = [(1.0, "1 year"), (0.25, "3 months"), (1 / 12, "1 month"), (1 / 365, "1 day")]
    fig, (ax,) = new_figure(t)
    style(ax, t, "Spot price S", "Call price")

    payoff = sweep(ope, "spot", 25, 60)
    ax.plot(payoff["spot"], payoff["call_payoff"], color=t["ink"], linewidth=1.2,
            linestyle=(0, (4, 3)), label="Payoff at expiry", zorder=4)
    for color, (expiry, label) in zip(reversed(t["ramp"]), expiries):
        data = sweep(ope, "spot", 25, 60, expiry=expiry)
        ax.plot(data["spot"], data["call"], color=color, linewidth=2, label=label)

    ax.set_ylim(bottom=0)
    legend(ax, t, loc="upper left", title="Time to expiry", title_fontsize=9)
    ax.get_legend().get_title().set_color(t["ink2"])
    titles(fig, t, "As expiry approaches, the price bends into the payoff",
           "Call, K = 40, r = 10%, σ = 20%")
    fig.subplots_adjust(top=0.80, bottom=0.14, left=0.07, right=0.98)
    return fig


def price_vs_vol(ope, t):
    data = sweep(ope, "vol", 0.0, 1.0)
    fig, (ax,) = new_figure(t)
    style(ax, t, "Volatility σ", "Option price")
    ax.xaxis.set_major_formatter(PercentFormatter(1.0))

    for kind in ("call", "put"):
        ax.plot(data["vol"], data[kind], color=t[kind], linewidth=2, label=kind.capitalize())
        ax.annotate(kind.capitalize(), (data["vol"][-1], data[kind][-1]), xytext=(6, 0),
                    textcoords="offset points", color=t["ink"], fontsize=9, va="center")

    hull = sweep(ope, "vol", 0.2, 0.200001, steps=2)
    for kind in ("call", "put"):
        y = hull[kind][0]
        ax.plot([0.2], [y], "o", markersize=8, color=t[kind], markeredgecolor=t["surface"],
                markeredgewidth=2, zorder=5)
        ax.annotate(f"{y:.2f}", (0.2, y), xytext=(-8, 8), textcoords="offset points",
                    color=t["ink"], fontsize=9, ha="right")

    ax.set_xlim(0, 1.0)
    ax.set_ylim(bottom=0)
    legend(ax, t, loc="upper left")
    titles(fig, t, "Price always rises with volatility, so implied vol has one answer",
           "S = 42, K = 40, T = 0.5y, r = 10%.  Dots: Hull's σ = 20%")
    fig.subplots_adjust(top=0.80, bottom=0.14, left=0.07, right=0.95)
    return fig


CHARTS = {"price_vs_spot": price_vs_spot, "time_decay": time_decay, "price_vs_vol": price_vs_vol}


def default_ope():
    for preset in ("release", "debug"):
        path = ROOT / "build" / preset / "cli" / "ope"
        if path.exists():
            return path
    raise SystemExit("ope binary not found; build it with: cmake --workflow --preset debug")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ope", type=Path, default=None, help="path to the ope binary")
    parser.add_argument("--out", type=Path, default=ROOT / "docs" / "images")
    args = parser.parse_args()

    ope = args.ope or default_ope()
    args.out.mkdir(parents=True, exist_ok=True)
    for name, chart in CHARTS.items():
        for mode, theme in THEMES.items():
            fig = chart(ope, theme)
            suffix = "" if mode == "light" else "-dark"
            path = args.out / f"{name}{suffix}.png"
            fig.savefig(path, dpi=160, facecolor=theme["surface"])
            plt.close(fig)
            print(path.relative_to(ROOT) if path.is_relative_to(ROOT) else path)


if __name__ == "__main__":
    main()
