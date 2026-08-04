"""Per-segment speed envelope, and the two reduced prediction folders built from it.

bem-slow: train segments with max body speed <= CUT (the paper's reduced set).
bem-ctrl: random full-envelope train segments, same row count as bem-slow.
Both also get the 13 testset.txt segments. Hard links from processed_data/bem,
renamed to the <base_type>_ prefix loader.py expects.

Usage: python3 analysis/speed_envelope.py [CUT]
"""
import os
import sys

import numpy as np
import pandas as pd

import utils

CUT = float(sys.argv[1]) if len(sys.argv) > 1 else 5.0
SRC = utils.DATA_DIR / "bem"

test_ids = {l.strip() for l in open(utils.REPO_ROOT / "testset.txt") if l.strip()}

rows = []
for f in sorted(SRC.glob("bem_*_seg_*.csv")):
    sid = f.stem[4:]
    s = np.linalg.norm(np.loadtxt(f, delimiter=",", usecols=(14, 15, 16)), axis=1)
    rows.append({"segment": sid, "split": "test" if sid in test_ids else "train",
                 "n": len(s), "max": s.max()})

df = pd.DataFrame(rows).sort_values("max").reset_index(drop=True)
test, train = df[df.split == "test"], df[df.split == "train"]
slow = train[train["max"] <= CUT]

rng = np.random.default_rng(0)
order = rng.permutation(len(train))
cum = train.n.values[order].cumsum()
ctrl = train.iloc[order[: np.searchsorted(cum, slow.n.sum()) + 1]]

for name, sel in [("bem-slow", slow), ("bem-ctrl", ctrl)]:
    dst = utils.DATA_DIR / name
    dst.mkdir(exist_ok=True)
    for f in dst.glob("*.csv"):
        f.unlink()
    for sid in list(sel.segment) + list(test.segment):
        os.link(SRC / f"bem_{sid}.csv", dst / f"{name}_{sid}.csv")
    print(f"{name}: {len(sel)} segments, {sel.n.sum()} rows, max {sel['max'].max():.1f} m/s")

print(f"\nfull train: {len(train)} segments, {train.n.sum()} rows, max {train['max'].max():.1f} m/s")
print(f"test: {len(test)} segments, {test.n.sum()} rows, "
      f"{(test['max'] > CUT).sum()}/{len(test)} above {CUT} m/s, max {test['max'].max():.1f} m/s")
