"""Paper Table II metrics. Numpy only, so no model code has to import torch."""

import numpy as np

# RSS21_Bauersfeld.md:437-443
PAPER = {"BEM": [0.803, 1.265, 0.982, 0.090, 0.017, 0.074],
         "BEM+NN": [0.204, 0.504, 0.335, 0.014, 0.004, 0.012]}
COLS = ["Fxy", "Fz", "F", "Mxy", "Mz", "M"]


def rmse(res):
    """Per-axis RMSE collapsed the way the paper reports it."""
    a = np.sqrt((res ** 2).mean(0))
    return [np.sqrt((a[0] ** 2 + a[1] ** 2) / 2), a[2], np.sqrt((a ** 2).mean())]


def table(force, torque):
    return rmse(force) + rmse(torque)
