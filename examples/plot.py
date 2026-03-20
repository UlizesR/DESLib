from __future__ import annotations

import csv
from pathlib import Path

import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parent
DATA_DIR = ROOT / "data"
PLOT_DIR = ROOT / "plot"


def read_csv(path: Path) -> dict[str, list[float]]:
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        headers = reader.fieldnames or []
        data = {name: [] for name in headers}

        for row in reader:
            for name in headers:
                value = row.get(name, "")
                if value is None or value == "":
                    continue
                data[name].append(float(value))

    return data


def state_columns(data: dict[str, list[float]]) -> list[str]:
    excluded = {"t", "error_norm", "h"}
    return [name for name in data if name not in excluded]


def plot_states(csv_file: Path, data: dict[str, list[float]]) -> None:
    t = data["t"]
    cols = state_columns(data)

    if not cols:
        return

    fig = plt.figure(figsize=(8, 5))
    ax = fig.add_subplot(111)

    for col in cols:
        ax.plot(t, data[col], label=col)

    ax.set_xlabel("t")
    ax.set_ylabel("state")
    ax.set_title(csv_file.stem)
    ax.grid(True)
    ax.legend()
    fig.tight_layout()

    out_path = PLOT_DIR / f"{csv_file.stem}.png"
    fig.savefig(out_path, dpi=200)
    plt.close(fig)
    print(f"Saved {out_path}")


def plot_error(csv_file: Path, data: dict[str, list[float]]) -> None:
    if "error_norm" not in data:
        return

    t = data["t"]
    err = data["error_norm"]

    if not err:
        return

    fig = plt.figure(figsize=(8, 5))
    ax = fig.add_subplot(111)
    ax.semilogy(t, err, label="error_norm")

    ax.set_xlabel("t")
    ax.set_ylabel("error_norm")
    ax.set_title(f"{csv_file.stem} error")
    ax.grid(True)
    ax.legend()
    fig.tight_layout()

    out_path = PLOT_DIR / f"{csv_file.stem}_error.png"
    fig.savefig(out_path, dpi=200)
    plt.close(fig)
    print(f"Saved {out_path}")


def plot_stepsize(csv_file: Path, data: dict[str, list[float]]) -> None:
    if "h" not in data:
        return

    t = data["t"]
    h = data["h"]

    if not h:
        return

    fig = plt.figure(figsize=(8, 5))
    ax = fig.add_subplot(111)
    ax.plot(t, h, label="h")

    ax.set_xlabel("t")
    ax.set_ylabel("step size")
    ax.set_title(f"{csv_file.stem} step size")
    ax.grid(True)
    ax.legend()
    fig.tight_layout()

    out_path = PLOT_DIR / f"{csv_file.stem}_stepsize.png"
    fig.savefig(out_path, dpi=200)
    plt.close(fig)
    print(f"Saved {out_path}")


def plot_3d_if_possible(csv_file: Path, data: dict[str, list[float]]) -> None:
    required = ["y0", "y1", "y2"]
    if not all(name in data for name in required):
        return

    x = data["y0"]
    y = data["y1"]
    z = data["y2"]

    fig = plt.figure(figsize=(8, 6))
    ax = fig.add_subplot(111, projection="3d")
    ax.plot(x, y, z, linewidth=0.6)

    ax.set_xlabel("y0")
    ax.set_ylabel("y1")
    ax.set_zlabel("y2")
    ax.set_title(f"{csv_file.stem} 3D trajectory")
    ax.view_init(elev=25, azim=45)
    fig.tight_layout()

    out_path = PLOT_DIR / f"{csv_file.stem}_3d.png"
    fig.savefig(out_path, dpi=200)
    plt.close(fig)
    print(f"Saved {out_path}")


def main() -> None:
    PLOT_DIR.mkdir(parents=True, exist_ok=True)

    csv_files = sorted(DATA_DIR.glob("*.csv"))
    if not csv_files:
        print(f"No CSV files found in {DATA_DIR}")
        return

    for csv_file in csv_files:
        data = read_csv(csv_file)
        if "t" not in data:
            print(f"Skipping {csv_file.name}: missing t column")
            continue

        plot_states(csv_file, data)
        plot_error(csv_file, data)
        plot_stepsize(csv_file, data)
        plot_3d_if_possible(csv_file, data)


if __name__ == "__main__":
    main()