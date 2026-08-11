"""Drone config. Mass and inertia are defined here and nowhere else."""

from dataclasses import dataclass

import numpy as np
import yaml

from .columns import ACC, ANGACC, ANGVEL
from .paths import DRONES


@dataclass
class Drone:
    name: str
    mass: float
    inertia: np.ndarray

    @classmethod
    def load(cls, name):
        with open(DRONES / f"{name}.yaml") as f:
            c = yaml.safe_load(f)
        return cls(c["name"], float(c["mass"]), np.asarray(c["inertia"], dtype=float))

    def measured(self, d):
        """Measured force and torque from a merged segment array."""
        f = self.mass * d[:, ACC]
        w = d[:, ANGVEL]
        t = self.inertia * d[:, ANGACC] + np.cross(w, self.inertia * w)
        return f, t