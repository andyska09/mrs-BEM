#!/usr/bin/env python3
"""Phase 1: BEM baseline error (measured - BEM predicted) over the dataset."""
import argparse
import glob
import os
import numpy as np

MASS = 0.752   # code/simulator/include/params.h
INERTIA = np.array([0.00254, 0.00214, 0.00436])

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def measured(d):
    f = MASS * d[:, 11:14]                                  # acc already includes gravity
    t = INERTIA * d[:, 1:4] + np.cross(d[:, 4:7], INERTIA * d[:, 4:7])
    return f, t


def report(name, fres, tres, rows):
    rf, rt = np.sqrt((fres ** 2).mean(0)), np.sqrt((tres ** 2).mean(0))
    print(f"\n[{name}] {len(rows) if isinstance(rows, list) else rows} rows")
    print("  force  RMSE [N]  x/y/z:", np.round(rf, 4))
    print("  torque RMSE [Nm] x/y/z:", np.round(rt, 5))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bem_dir", default=os.path.join(REPO_ROOT, "processed_data", "bem"))
    ap.add_argument("--testset", default=os.path.join(REPO_ROOT, "testset.txt"))
    args = ap.parse_args()

    test_ids = {l.strip() for l in open(args.testset) if l.strip()}
    files = sorted(glob.glob(os.path.join(args.bem_dir, "bem_*.csv")))
    if not files:
        raise SystemExit(f"No bem_*.csv in {args.bem_dir}")

    fr_all, tr_all, n_all = [], [], 0
    fr_test, tr_test, n_test = [], [], 0
    for f in files:
        fid = os.path.basename(f)[4:-4]
        d = np.loadtxt(f, delimiter=",")
        fm, tm = measured(d)
        fr, tr = fm - d[:, 29:32], tm - d[:, 32:35]
        fr_all.append(fr); tr_all.append(tr); n_all += len(d)
        if fid in test_ids:
            fr_test.append(fr); tr_test.append(tr); n_test += len(d)

    report("full dataset", np.vstack(fr_all), np.vstack(tr_all), n_all)
    if fr_test:
        report(f"test set ({len(fr_test)} segments)", np.vstack(fr_test), np.vstack(tr_test), n_test)
    else:
        print("\n(no test-set segments found in bem_dir)")


if __name__ == "__main__":
    main()
