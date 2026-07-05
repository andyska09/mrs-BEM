import glob
import os
import numpy as np

MASS = 0.752
INERTIA = np.array([0.00254, 0.00214, 0.00436])
R = 5.1 * 2.54 / 2 * 1e-2   # prop radius, params.h

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BEM_DIR = os.path.join(REPO_ROOT, "processed_data", "bem")

VI = [35, 38, 41, 44]      # induced velocity per motor
MU = [36, 39, 42, 45]      # advance ratio per motor
AS = [37, 40, 43, 46]      # shaft angle of attack per motor


def load_bem(bem_dir=BEM_DIR):
    files = sorted(glob.glob(os.path.join(bem_dir, "bem_*.csv")))
    if not files:
        raise SystemExit(f"No bem_*.csv in {bem_dir}")
    return np.vstack([np.loadtxt(f, delimiter=",") for f in files])


def measured(d):
    f = MASS * d[:, 11:14]
    t = INERTIA * d[:, 1:4] + np.cross(d[:, 4:7], INERTIA * d[:, 4:7])
    return f, t


def residuals(d):
    f, t = measured(d)
    return f - d[:, 29:32], t - d[:, 32:35]


def diagnostics(d):
    return d[:, VI], d[:, MU], d[:, AS]   # each (N, 4)
