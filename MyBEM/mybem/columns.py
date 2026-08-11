"""Column indices of merged_*_seg_X.csv, per NeuroBEM/README.md."""

import pandas as pd

from .paths import DATA

ANGACC = [1, 2, 3]
ANGVEL = [4, 5, 6]
ATT = [10, 7, 8, 9]  # file order is qx,qy,qz,qw; we use w,x,y,z
ACC = [11, 12, 13]
LINVEL = [14, 15, 16]
POS = [17, 18, 19]
MOTORS = [20, 21, 22, 23]
VBAT = 28


def load(sid, source=DATA):
    return pd.read_csv(source / f"merged_{sid}.csv").to_numpy(float)