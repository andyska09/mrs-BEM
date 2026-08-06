"""Drone config and the merged-CSV column layout.

Mass and inertia are defined here and nowhere else.
"""

from dataclasses import dataclass
from pathlib import Path

import numpy as np
import yaml

ROOT = Path(__file__).resolve().parent.parent

# Column indices of merged_*_seg_X.csv, per NeuroBEM/README.md.
ANGACC = [1, 2, 3]
ANGVEL = [4, 5, 6]
ATT = [10, 7, 8, 9]  # file order is qx,qy,qz,qw; we use w,x,y,z
ACC = [11, 12, 13]
LINVEL = [14, 15, 16]
POS = [17, 18, 19]
MOTORS = [20, 21, 22, 23]
VBAT = 28


@dataclass
class Drone:
    name: str
    mass: float
    inertia: np.ndarray

    @classmethod
    def load(cls, name):
        with open(ROOT / "configs" / "drones" / f"{name}.yaml") as f:
            c = yaml.safe_load(f)
        return cls(c["name"], float(c["mass"]), np.asarray(c["inertia"], dtype=float))

    def measured(self, d):
        """Measured force and torque from a merged segment array."""
        f = self.mass * d[:, ACC]
        w = d[:, ANGVEL]
        t = self.inertia * d[:, ANGACC] + np.cross(w, self.inertia * w)
        return f, t
