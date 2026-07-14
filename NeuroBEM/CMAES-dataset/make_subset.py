"""Build the fixed, regime-stratified subset for CMA-ES aero identification.

Pools all non-test flights from processed_data/bem-vi-baseline (47-col, carries
the per-motor vi/mu diagnostics), bins rows on a (mu, vi) grid, and samples
~equally per cell so aggressive regimes are not drowned out by hover. Writes the
chosen rows next to this script so the optimization always runs on the same set.
"""
import glob
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "analysis"))
import utils

SEED = 0
N_TARGET = 20000
N_MU, N_VI = 6, 6
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(utils.REPO_ROOT, "processed_data", "bem-vi-baseline")
OUT = os.path.join(HERE, "subset_20k.csv")

test_ids = {l.strip() for l in open(os.path.join(utils.REPO_ROOT, "testset.txt")) if l.strip()}

rows, n_train, n_test = [], 0, 0
for f in sorted(glob.glob(os.path.join(SRC, "bem_*.csv"))):
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
