import sys
from pathlib import Path

import numpy as np

MASS = 0.772                                    # Readme.md
INERTIA = np.array([0.0025, 0.0021, 0.0043])    # Readme.md


def residuals(d):
    f = MASS * d[:, 11:14]
    t = INERTIA * d[:, 1:4] + np.cross(d[:, 4:7], INERTIA * d[:, 4:7])
    return f - d[:, 29:32], t - d[:, 32:35]


folder = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("processed_data/bem")
for f in sorted(folder.glob("*.csv")):
    d = np.loadtxt(f, delimiter=",")
    rf, rt = residuals(d)
    np.savetxt(f, np.hstack([d[:, :35], rf, rt]), delimiter=",", fmt="%.6f")
    print(f.name)
