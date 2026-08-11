"""Polynomial term algebra and the paper's candidate sets (eq. 19-22, 29-43).

A term is a tuple of (variable, power) pairs; `render`/`parse` are the "a*b^2"
text form that coeffs.txt carries over to PolyfitModel::load in C++.
"""

from itertools import product

import numpy as np


def render(term):
    if not term:
        return "1"
    return "*".join(v if k == 1 else f"{v}^{k}" for v, k in term)


def parse(name):
    if name == "1":
        return ()
    out = []
    for part in name.split("*"):
        v, _, k = part.partition("^")
        out.append((v, int(k) if k else 1))
    return tuple(out)


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
    return evaluate([parse(n) for n in fixed + cand], s)
