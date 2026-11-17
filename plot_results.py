"""Plot the running average of transmissions per packet for each network.

Reads results/running_average.csv written by the simulator and saves
results/running_average.png: one panel per network, the running average
(blue line) and the theoretical ETX from the existing code (dashed line).

Usage (after installing matplotlib in .venv, see README):
    .venv/bin/python plot_results.py [--out results] [--logx]
"""
import argparse
import csv
import os
import sys

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    if os.name == "nt":
        steps = ("python -m venv .venv\n"
                 "  .venv\\Scripts\\pip install matplotlib\n"
                 "  .venv\\Scripts\\python plot_results.py")
    else:
        steps = ("python3 -m venv .venv\n"
                 "  .venv/bin/pip install matplotlib\n"
                 "  .venv/bin/python plot_results.py")
    sys.exit("matplotlib is not installed for this Python. From the project folder run:\n  "
             + steps)

SERIES = "#2a78d6"     # running average
REFERENCE = "#52514e"  # theoretical ETX line and its label
INK = "#0b0b0b"
SURFACE = "#fcfcfb"

TITLES = {
    1: "Network 1: A to B and C",
    2: "Network 2: A to D and E",
    3: "Network 3: A to E and F",
    4: "Network 4: A to E, F and G",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", default="results", help="folder with the CSV files")
    parser.add_argument("--logx", action="store_true", help="log scale for packets")
    args = parser.parse_args()

    path = os.path.join(args.out, "running_average.csv")
    if not os.path.exists(path):
        sys.exit(f"{path} not found. Run the simulator first.")

    data = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            net = int(row["network"])
            entry = data.setdefault(net, {"packet": [], "avg": [], "etx": float(row["theoretical_etx"])})
            entry["packet"].append(int(row["packet"]))
            entry["avg"].append(float(row["running_average"]))

    fig, axes = plt.subplots(2, 2, figsize=(11, 7.5), facecolor=SURFACE)
    for ax, net in zip(axes.flat, sorted(data)):
        d = data[net]
        ax.set_facecolor(SURFACE)
        ax.plot(d["packet"], d["avg"], color=SERIES, linewidth=2,
                label="simulated running average")
        ax.axhline(d["etx"], color=REFERENCE, linewidth=1.5, linestyle="--",
                   label="theoretical ETX")
        final = d["avg"][-1]
        ax.set_title(f"{TITLES.get(net, f'Network {net}')}\n"
                     f"theoretical ETX {d['etx']:.4f}, simulated {final:.4f} "
                     f"after {d['packet'][-1]:,} packets",
                     fontsize=10, color=INK, loc="left")
        ax.set_xlabel("packets simulated", color=INK)
        ax.set_ylabel("average transmissions per packet", color=INK)
        if args.logx:
            ax.set_xscale("log")
        # Zoom the y axis around the ETX so the convergence is visible.
        tail = d["avg"][min(len(d["avg"]) - 1, 20):]
        spread = max(abs(v - d["etx"]) for v in tail) if tail else 0.5
        spread = max(spread * 1.2, 0.05)
        ax.set_ylim(d["etx"] - spread, d["etx"] + spread)
        ax.grid(True, color="#e4e3df", linewidth=0.8)
        for side in ("top", "right"):
            ax.spines[side].set_visible(False)
        for side in ("left", "bottom"):
            ax.spines[side].set_color("#b5b4ae")
        ax.tick_params(colors=REFERENCE, labelsize=9)

    handles, labels = axes.flat[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=2, frameon=False)
    fig.tight_layout(rect=(0, 0, 1, 0.95))
    out = os.path.join(args.out, "running_average.png")
    fig.savefig(out, dpi=150, facecolor=SURFACE)
    print(f"Saved {out}")


if __name__ == "__main__":
    main()
