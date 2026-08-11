"""Identify the model from flight segments, and predict a wrench with it."""

import numpy as np

from ..columns import load
from ..metrics import COLS, table
from ..paths import DATA
from .physics import FLU2FRD, Geometry, coefficients, sideslip, states
from .stepwise import Gram, stepwise
from .terms import AXES, columns, design, evaluate, parse


def bin_edges(nbins):
    return np.linspace(0.0, 90.0, nbins + 1)


def identify(ids, drone, geo, ct, nbins, source=DATA, verbose=True):
    """Stepwise-fit every axis in every |beta| bin. Returns a plain-dict model."""
    edges = bin_edges(nbins)
    grams = {(a, b): Gram(len(columns(a)[0]) + len(columns(a)[1]))
             for a in AXES for b in range(nbins)}

    for sid in ids:
        d = load(sid, source)
        s, ob = states(d, geo, ct)
        f, t = drone.measured(d)
        c = coefficients(f * FLU2FRD, t * FLU2FRD, geo, ob)
        b = np.clip(np.digitize(sideslip(s), edges[1:-1]), 0, nbins - 1)
        for ai, axis in enumerate(AXES):
            x = design(axis, s)
            for k in range(nbins):
                m = b == k
                if m.any():
                    grams[(axis, k)].add(x[m], c[m, ai])

    model = {"ct_hover": ct, "radius": geo.radius, "air_density": geo.air_density,
             "dx": geo.dx, "dy": geo.dy, "n_rotors": geo.n_rotors,
             "beta_edges": edges.tolist(), "axes": {}}
    for axis in AXES:
        fixed, cand = columns(axis)
        names = fixed + cand
        model["axes"][axis] = []
        for k in range(nbins):
            c, mean = grams[(axis, k)].centered()
            n = grams[(axis, k)].n
            b0, coef, chosen, mse, r2, _ = stepwise(
                c, mean, n, list(range(len(fixed))),
                list(range(len(fixed), len(names))), names)
            model["axes"][axis].append(
                {"terms": {"1": float(b0), **dict(zip(chosen, coef.tolist()))},
                 "n": int(n), "r2": float(r2), "rms": float(np.sqrt(mse))})
            if verbose:
                print(f"  {axis} bin {k}: {len(chosen):3d} terms  "
                      f"R2={r2:.4f}  n={int(n)}")
    return model


def geometry(model):
    return Geometry(radius=model["radius"], air_density=model["air_density"],
                    dx=model["dx"], dy=model["dy"], n_rotors=model["n_rotors"])


def predict(model, d):
    """Model dict + merged rows -> force and torque in body FLU."""
    geo = geometry(model)
    s, ob = states(d, geo, model["ct_hover"])
    edges = np.asarray(model["beta_edges"])
    nb = len(edges) - 1
    b = np.clip(np.digitize(sideslip(s), edges[1:-1]), 0, nb - 1)

    out = np.zeros((len(d), 6))
    for ai, axis in enumerate(AXES):
        for k in range(nb):
            m = b == k
            if not m.any():
                continue
            sub = {key: v[m] for key, v in s.items()}
            for name, coef in model["axes"][axis][k]["terms"].items():
                out[m, ai] += coef * evaluate([parse(name)], sub)[:, 0]

    q = geo.air_density * geo.disc_area * (geo.radius * ob) ** 2
    f = out[:, :3] * q[:, None]
    t = out[:, 3:] * (q * geo.ref_length)[:, None]
    return f * FLU2FRD, t * FLU2FRD


def score(model, ids, drone, source=DATA):
    """Paper Table II metrics of the residual left by the model."""
    ef, et = [], []
    for sid in ids:
        d = load(sid, source)
        fm, tm = drone.measured(d)
        fp, tp = predict(model, d)
        ef.append(fm - fp)
        et.append(tm - tp)
    return dict(zip(COLS, table(np.vstack(ef), np.vstack(et))))
