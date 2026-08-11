"""Forward-backward stepwise regression over cross-product matrices.

Follows the authors' MATLAB release, not the paper's Algorithm 1: the printed
forward-selection residual, PSE formula and P^d example are all misprinted there.
"""

import numpy as np

F_IN = 5.0
F_OUT = 4.0
MAX_STEPS = 30


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


def _pse(sse, sst, n, p):
    return sse / n + (sst / n) * p / n


def _weakest(c, idx, z, n):
    """Index into `idx` of the regressor with the smallest partial F, and that F."""
    _, ssr = _solve(c, idx, z)
    s2 = (c[z, z] - ssr) / (n - len(idx) - 1)
    f = [abs(ssr - _solve(c, idx[:i] + idx[i + 1:], z)[1]) / s2 for i in range(len(idx))]
    i = int(np.argmin(f))
    return i, f[i]


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
