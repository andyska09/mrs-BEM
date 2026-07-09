from pathlib import Path

import numpy as np

MASS = 0.772                                    # Readme.md
INERTIA = np.array([0.0025, 0.0021, 0.0043])    # Readme.md

SRC = Path("processed_data/bem-vi-baseline")
DST = Path("processed_data/bem-baseline")


def residuals(d):
    f = MASS * d[:, 11:14]
    t = INERTIA * d[:, 1:4] + np.cross(d[:, 4:7], INERTIA * d[:, 4:7])
    return f - d[:, 29:32], t - d[:, 32:35]


def convert(src=SRC, dst=DST):
    dst.mkdir(parents=True, exist_ok=True)
    files = sorted(src.glob("bem_*.csv"))
    if not files:
        raise SystemExit(f"No bem_*.csv in {src}")
    for f in files:
        d = np.loadtxt(f, delimiter=",")
        rf, rt = residuals(d)
        out = np.hstack([d[:, :35], rf, rt])
        np.savetxt(dst / f.name, out, delimiter=",", fmt="%.6f")
    print(f"wrote {len(files)} files ({out.shape[1]} cols) to {dst}")


if __name__ == "__main__":
    convert()
