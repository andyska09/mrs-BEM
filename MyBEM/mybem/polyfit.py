"""Vehicle-level polynomial gray-box model (Sun, de Visser & Chu, JoA 56(2) 2019).

Stepwise selection follows the authors' MATLAB release, not the paper's
Algorithm 1: the printed forward-selection residual, PSE formula and P^d example
are all misprinted there.

Selection runs on a cross-product matrix rather than the candidate columns, so
the full dataset is used without ever materialising an N x 895 design matrix.
"""

import argparse
from dataclasses import dataclass
from itertools import product

import numpy as np
import pandas as pd
import yaml

from .data import DATA, segment_ids, split
from .drone import ACC, ANGVEL, LINVEL, MOTORS, ROOT, Drone
from .eval import table

FLU2FRD = np.array([1.0, -1.0, -1.0])

F_IN = 5.0
F_OUT = 4.0
MAX_STEPS = 30


# --- sufficient statistics ------------------------------------------------

class Gram:
    """Accumulates [1, cols, z]^T [1, cols, z] over row blocks."""

    def __init__(self, ncol):
        self.m = np.zeros((ncol + 2, ncol + 2))

    def add(self, cols, z):
        b = np.column_stack([np.ones(len(z)), cols, z])
        self.m += b.T @ b

    @property
    def n(self):
        return self.m[0, 0]

    def centered(self):
        """Cross-products about the mean, and the column means."""
        mean = self.m[0, 1:] / self.n
        return self.m[1:, 1:] - self.n * np.outer(mean, mean), mean


def _solve(c, idx, z):
    """Least squares of column z on `idx`, from a centered cross-product matrix."""
    a = c[np.ix_(idx, idx)]
    b = c[idx, z]
    d = np.sqrt(np.diag(a))
    d[d <= 0] = 1.0
    k, *_ = np.linalg.lstsq(a / np.outer(d, d), b / d, rcond=1e-12)
    k /= d
    return k, float(k @ b)


def stepwise(c, mean, n, fixed, cand, names):
    """Forward-backward stepwise selection over a centered cross-product matrix.

    `fixed` and `cand` are column indices into `c`; the last column is the target.
    Returns (intercept, coefficients, chosen names, log rows).
    """
    z = len(c) - 1
    sst = c[z, z]
    pool, chosen, log, dropped = list(cand), [], [], None

    def fit(idx):
        if not idx:
            return np.zeros(0), 0.0
        k, ssr = _solve(c, idx, z)
        return k, ssr

    idx = list(fixed)
    _, ssr = fit(idx)
    pse = _pse(sst - ssr, sst, n, len(idx) + 1)
    pse_tol, pse_last, last = 1e-3 * pse, pse, list(idx)

    for step in range(1, MAX_STEPS + 1):
        if not pool:
            break
        k, ssr = fit(idx)
        sse = sst - ssr

        # r is orthogonal to the fitted columns, so x'r needs no pass over the data.
        xr = c[pool, z] - c[np.ix_(pool, idx)] @ k
        w = np.linalg.lstsq(c[np.ix_(idx, idx)], c[np.ix_(idx, pool)], rcond=1e-12)[0]
        vv = np.diag(c[np.ix_(pool, pool)]) - np.einsum("ij,ji->i", c[np.ix_(pool, idx)], w)
        vv = np.maximum(vv, 0)
        ok = vv > 1e-14 * np.diag(c[np.ix_(pool, pool)])
        if not ok.any():
            break
        score = np.where(ok, xr ** 2 / np.where(ok, vv, 1), -np.inf)
        j = pool[int(np.argmax(score))]

        gain = score.max()
        p = len(idx) + 1
        if gain / ((sse - gain) / (n - p - 1)) <= F_IN:
            break
        idx.append(j)
        pool.remove(j)
        if names[j] == dropped:
            idx.pop()
            break

        out, f_out = _weakest(c, idx, z, n)
        dropped = names[idx[out]] if f_out < F_OUT and idx[out] not in fixed else None
        if dropped is not None:
            idx.pop(out)

        _, ssr = fit(idx)
        pse = _pse(sst - ssr, sst, n, len(idx) + 1)
        log.append((step, [names[i] for i in idx], pse, ssr / sst,
                    np.sqrt((sst - ssr) / n)))
        if pse >= 0.99 * pse_last:
            idx = last
            break
        if pse < pse_tol:
            break
        pse_last, last = pse, list(idx)

    k, ssr = fit(idx)
    b0 = mean[z] - float(k @ mean[idx])
    return b0, k, [names[i] for i in idx], (sst - ssr) / n, ssr / sst, log


def _pse(sse, sst, n, p):
    return sse / n + (sst / n) * p / n


def _weakest(c, idx, z, n):
    """Index into `idx` of the regressor with the smallest partial F, and that F."""
    _, ssr = _solve(c, idx, z)
    s2 = (c[z, z] - ssr) / (n - len(idx) - 1)
    f = [abs(ssr - _solve(c, idx[:i] + idx[i + 1:], z)[1]) / s2 for i in range(len(idx))]
    i = int(np.argmin(f))
    return i, f[i]


# --- polynomial candidate sets, paper eq. 19-22 ---------------------------

def render(term):
    if not term:
        return "1"
    return "*".join(v if k == 1 else f"{v}^{k}" for v, k in term)


def poly(names, degree):
    """All monomials in `names` of total degree <= degree, constant included."""
    out = []
    for e in product(range(degree + 1), repeat=len(names)):
        if sum(e) <= degree:
            out.append(tuple((n, k) for n, k in zip(names, e) if k))
    return sorted(set(out), key=lambda t: (sum(k for _, k in t), render(t)))


def mul(a, b):
    """Non-repetitive products of two candidate sets."""
    out = {}
    for x, y in product(a, b):
        e = {}
        for v, k in x + y:
            e[v] = e.get(v, 0) + k
        out.setdefault(tuple(sorted(e.items())), None)
    return list(out)


def odd3(name):
    """{1, v, s(v)v^2, v^3} -- even powers stay odd-symmetric, as in Tables 12-13."""
    return [(), ((name, 1),), ((f"abs_{name}", 1), (name, 1)), ((name, 3),)]


def evaluate(terms, s):
    n = len(s["mu_x"])
    return np.column_stack([
        np.prod([s[v] ** k for v, k in t], axis=0) if t else np.ones(n)
        for t in terms])


# --- nondimensionalization, paper eq. 8-16 --------------------------------

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


# Candidate sets of paper eq. 29, 30, 36, 39, 41, 43 with their fixed regressors.
SETS = {
    "Cx": (["mu_x"], lambda: poly(["mu_x", "abs_mu_y", "mu_z"], 3)),
    "Cy": (["mu_y"], lambda: poly(["abs_mu_x", "mu_y", "mu_z"], 3)),
    "Cz": (["mu_h2", "lam2"],
           lambda: mul(poly(["abs_mu_x", "abs_mu_y", "mu_z"], 4),
                       [()] + [((v, 1),) for v in
                               ("abs_p_bar", "abs_q_bar", "abs_r_bar",
                                "abs_u_p", "abs_u_q", "abs_u_r")])),
    "Cl": (["u_p"],
           lambda: mul(mul(poly(["mu_y", "mu_z"], 5), poly(["abs_mu_x"], 2)),
                       [(), (("p_bar", 1),), (("u_p", 1),)])),
    "Cm": (["u_q"],
           lambda: mul(mul(poly(["mu_x", "mu_z"], 5), poly(["abs_mu_y"], 2)),
                       [(), (("q_bar", 1),), (("u_q", 1),)])),
    "Cn": ([], lambda: mul(mul(poly(["mu_x", "mu_y", "mu_z"], 5),
                               odd3("r_bar")), odd3("u_r"))),
}
AXES = list(SETS)


def columns(axis):
    """Regressor names for one axis: the fixed ones first, then the candidates."""
    fixed, build = SETS[axis]
    cand = [render(t) for t in build() if t and render(t) not in fixed]
    return fixed, cand


def design(axis, s):
    fixed, cand = columns(axis)
    return evaluate([_parse(n) for n in fixed + cand], s)


def _parse(name):
    if name == "1":
        return ()
    out = []
    for part in name.split("*"):
        v, _, k = part.partition("^")
        out.append((v, int(k) if k else 1))
    return tuple(out)


# --- identification -------------------------------------------------------

def bin_edges(nbins):
    return np.linspace(0.0, 90.0, nbins + 1)


def identify(ids, drone, geo, ct, nbins, source=DATA, verbose=True):
    """Stepwise-fit every axis in every |beta| bin. Returns a plain-dict model."""
    edges = bin_edges(nbins)
    grams = {(a, b): Gram(len(columns(a)[0]) + len(columns(a)[1]))
             for a in AXES for b in range(nbins)}

    for sid in ids:
        d = pd.read_csv(source / f"merged_{sid}.csv").to_numpy(float)
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
             "ref_length": geo.ref_length, "n_rotors": geo.n_rotors,
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


def predict(model, d, drone=None):
    """Model dict + merged rows -> force and torque in body FLU."""
    geo = Geometry(radius=model["radius"], air_density=model["air_density"],
                   n_rotors=model["n_rotors"])
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
                out[m, ai] += coef * evaluate([_parse(name)], sub)[:, 0]

    q = geo.air_density * geo.disc_area * (geo.radius * ob) ** 2
    f = out[:, :3] * q[:, None]
    t = out[:, 3:] * (q * model["ref_length"])[:, None]
    return f * FLU2FRD, t * FLU2FRD


def rmse(model, ids, drone, source=DATA):
    """Paper Table II metrics of the residual left by the model."""
    ef, et = [], []
    for sid in ids:
        d = pd.read_csv(source / f"merged_{sid}.csv").to_numpy(float)
        fm, tm = drone.measured(d)
        fp, tp = predict(model, d)
        ef.append(fm - fp)
        et.append(tm - tp)
    return dict(zip(("Fxy", "Fz", "F", "Mxy", "Mz", "M"),
                    table(np.vstack(ef), np.vstack(et))))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--name", default="polyfit_paper")
    ap.add_argument("--split", default="paper")
    ap.add_argument("--drone", default="paper_quad")
    ap.add_argument("--bins", type=int, default=3)
    ap.add_argument("--limit", type=int, default=0)
    a = ap.parse_args()

    drone = Drone.load(a.drone)
    geo = Geometry()
    folds = split(a.split, segment_ids())
    train = folds["train"][:a.limit] if a.limit else folds["train"]
    test = folds["test"][:a.limit] if a.limit else folds["test"]

    ct = hover_ct(pd.read_csv(DATA / f"merged_{train[0]}.csv").to_numpy(float),
                  drone, geo)
    print(f"train={len(train)} test={len(test)} bins={a.bins} Ct,h={ct:.5f}")
    model = identify(train, drone, geo, ct, a.bins)

    out = ROOT / "store" / "polyfit" / a.name
    out.mkdir(parents=True, exist_ok=True)
    with open(out / "polyfit.yaml", "w") as f:
        yaml.safe_dump(model, f, sort_keys=False)
    with open(out / "coeffs.txt", "w") as f:
        f.write("beta_edges " + " ".join(f"{e:g}" for e in model["beta_edges"]) + "\n")
        for axis in AXES:
            for k, b in enumerate(model["axes"][axis]):
                for term, coef in b["terms"].items():
                    f.write(f"{axis} {k} {term} {coef:.12g}\n")

    print(f"\n{'':10s}" + "".join(f"{k:>9s}" for k in
                                  ("Fxy", "Fz", "F", "Mxy", "Mz", "M")))
    for fold, sids in (("train", train), ("test", test)):
        m = rmse(model, sids, drone)
        print(f"{fold:10s}" + "".join(f"{m[k]:9.3f}" for k in
                                      ("Fxy", "Fz", "F", "Mxy", "Mz", "M")))
    print(f"\nwrote {out / 'polyfit.yaml'}")


if __name__ == "__main__":
    main()
