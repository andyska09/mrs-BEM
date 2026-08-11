"""Nondimensionalization of the merged rows into the paper's states (eq. 8-16)."""

from dataclasses import dataclass

import numpy as np

from ..columns import ACC, ANGVEL, LINVEL, MOTORS

FLU2FRD = np.array([1.0, -1.0, -1.0])


@dataclass
class Geometry:
    radius: float = 0.06477
    air_density: float = 1.204
    dx: float = 0.078
    dy: float = 0.100
    n_rotors: int = 4

    @property
    def ref_length(self):
        return float(np.hypot(self.dx, self.dy))

    @property
    def disc_area(self):
        return self.n_rotors * np.pi * self.radius ** 2


def induced(mu_x, mu_y, mu_z, ct_hover, iters=64):
    nu = np.full_like(mu_x, np.sqrt(ct_hover / 2))
    h = mu_x ** 2 + mu_y ** 2
    for _ in range(iters):
        nu = ct_hover / (2 * np.sqrt(h + (nu - mu_z) ** 2))
    return nu


def hover_ct(d, drone, geo, mu_max=0.05):
    """Thrust coefficient near hover, the Ct,h of the induced-velocity solve."""
    ob = np.sqrt((d[:, MOTORS] ** 2).mean(1))
    near = np.linalg.norm(d[:, LINVEL], axis=1) / (ob * geo.radius) < mu_max
    q = geo.air_density * geo.disc_area * (geo.radius * ob) ** 2
    return float((drone.mass * d[near, ACC[2]] / q[near]).mean())


def states(d, geo, ct_hover):
    """Merged-CSV rows -> the paper's dimensionless states, body FRD."""
    mot = d[:, MOTORS]
    ob = np.sqrt((mot ** 2).mean(1))
    tip = ob * geo.radius
    mu = (d[:, LINVEL] * FLU2FRD) / tip[:, None]
    rate = (d[:, ANGVEL] * FLU2FRD) * geo.ref_length / tip[:, None]

    # Signs mirror Airframe::offsets() and spinCW(): back-right, front-right,
    # back-left, front-left.
    w2 = (mot / ob[:, None]) ** 2
    u = np.column_stack([w2 @ [-1, -1, 1, 1], w2 @ [-1, 1, -1, 1],
                         w2 @ [1, -1, -1, 1]])

    nu = induced(mu[:, 0], mu[:, 1], mu[:, 2], ct_hover)
    s = {"mu_x": mu[:, 0], "mu_y": mu[:, 1], "mu_z": mu[:, 2],
         "p_bar": rate[:, 0], "q_bar": rate[:, 1], "r_bar": rate[:, 2],
         "u_p": u[:, 0], "u_q": u[:, 1], "u_r": u[:, 2],
         "nu_in": nu,
         "mu_h2": mu[:, 0] ** 2 + mu[:, 1] ** 2,
         "lam2": (nu - mu[:, 2]) ** 2}
    s.update({f"abs_{k}": np.abs(v) for k, v in list(s.items())})
    return s, ob


def sideslip(s):
    """|beta| in degrees, paper eq. 2. Zero where airspeed vanishes."""
    h = np.hypot(s["mu_x"], s["mu_y"])
    return np.degrees(np.arcsin(np.divide(s["abs_mu_y"], h, out=np.zeros_like(h),
                                          where=h > 1e-9)))


def coefficients(f, t, geo, ob):
    """Measured wrench (FRD) -> Cx,Cy,Cz,Cl,Cm,Cn."""
    q = geo.air_density * geo.disc_area * (geo.radius * ob) ** 2
    return np.column_stack([f / q[:, None], t / (q * geo.ref_length)[:, None]])
