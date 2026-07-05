#!/usr/bin/env python3
"""Build a balanced, regime-stratified subset for CMA-ES identification.

Excludes the test-set flights, bins the remaining rows on a (mu, v_i) grid,
and samples ~equally per cell so aggressive regimes are not drowned out by
hover. The chosen rows are saved (same 47-col format as bem_*.csv) so the
optimization always runs on the exact same fixed subset.
"""
import glob
import os
import numpy as np
import utils

SEED = 0
N_TARGET = 20000
N_MU, N_VI = 6, 6
OUT = os.path.join(utils.REPO_ROOT, "analysis", "subset_20k.csv")

test_ids = {l.strip() for l in open(os.path.join(utils.REPO_ROOT, "testset.txt")) if l.strip()}

rows, n_train, n_test = [], 0, 0
for f in sorted(glob.glob(os.path.join(utils.BEM_DIR, "bem_*.csv"))):
    if os.path.basename(f)[4:-4] in test_ids:
        n_test += 1
        continue
    rows.append(np.loadtxt(f, delimiter=","))
    n_train += 1
d = np.vstack(rows)

vi, mu, _ = utils.diagnostics(d)
vi_m, mu_m = vi.mean(1), mu.mean(1)

mu_b = np.digitize(mu_m, np.quantile(mu_m, np.linspace(0, 1, N_MU + 1)[1:-1]))
vi_b = np.digitize(vi_m, np.quantile(vi_m, np.linspace(0, 1, N_VI + 1)[1:-1]))
cell = mu_b * N_VI + vi_b

rng = np.random.default_rng(SEED)
cells = np.unique(cell)
per = N_TARGET // len(cells)
idx = np.concatenate([
    rng.choice(np.where(cell == c)[0], min((cell == c).sum(), per), replace=False)
    for c in cells
])

subset = d[np.sort(idx)]
np.savetxt(OUT, subset, delimiter=",", fmt="%.9g")
print(f"train flights: {n_train} (held out {n_test}) -> pooled {len(d)} rows")
print(f"regime cells: {len(cells)}, target/cell: {per}")
print(f"saved {len(subset)} rows (seed={SEED}) to {OUT}")
