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


DST.mkdir(parents=True, exist_ok=True)
for f in sorted(SRC.glob("bem_*.csv")):
    d = np.loadtxt(f, delimiter=",")
    rf, rt = residuals(d)
    np.savetxt(DST / f.name, np.hstack([d[:, :35], rf, rt]), delimiter=",", fmt="%.6f")
    print(f.name)
