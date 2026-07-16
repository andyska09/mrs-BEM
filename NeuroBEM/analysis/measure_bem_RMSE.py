#!/usr/bin/env python3
"""BEM prediction error in the format of NeuroBEM paper Table II.

Reports Fxy, Fz, Mxy, Mz, F, M where the horizontal columns are per-axis RMSE
(x,y collapsed to one) and F/M are the RMS across the three axes:
    Fxy = sqrt((Fx^2 + Fy^2) / 2)      F = sqrt((Fx^2 + Fy^2 + Fz^2) / 3)

Usage: measure_bem_RMSE.py [--bem_dir processed_data/<folder>]
"""

import argparse
import glob
import os
import numpy as np

MASS = 0.772  # dataset README / paper
INERTIA = np.array([0.00254, 0.00214, 0.00436])  # code/simulator/include/params.h

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def measured(d):
    f = MASS * d[:, 11:14]  # acc already includes gravity
    t = INERTIA * d[:, 1:4] + np.cross(d[:, 4:7], INERTIA * d[:, 4:7])
    return f, t


def metrics(res):
    a = np.sqrt((res**2).mean(0))  # per-axis RMSE [x, y, z]
    xy = np.sqrt((a[0] ** 2 + a[1] ** 2) / 2)
    tot = np.sqrt((a**2).mean())
    return xy, a[2], tot


def report(name, fres, tres, rows):
    fxy, fz, F = metrics(fres)
    mxy, mz, M = metrics(tres)
    print(
        f"{name:16s} {rows:>9d}  {fxy:6.3f} {fz:6.3f}  {mxy:7.4f} {mz:7.4f}  {F:6.3f} {M:7.4f}"
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bem_dir", default=os.path.join(REPO_ROOT, "processed_data", "bem"))
    ap.add_argument("--testset", default=os.path.join(REPO_ROOT, "testset.txt"))
    args = ap.parse_args()

    test_ids = {l.strip() for l in open(args.testset) if l.strip()}
    files = sorted(glob.glob(os.path.join(args.bem_dir, "*.csv")))
    if not files:
        raise SystemExit(f"No csv in {args.bem_dir}")

    fr_all, tr_all, n_all = [], [], 0
    fr_test, tr_test, n_test = [], [], 0
    for f in files:
        fid = os.path.basename(f).split("_", 1)[1][:-4]
        d = np.loadtxt(f, delimiter=",")
        fm, tm = measured(d)
        fr, tr = fm - d[:, 29:32], tm - d[:, 32:35]
        fr_all.append(fr)
        tr_all.append(tr)
        n_all += len(d)
        if fid in test_ids:
            fr_test.append(fr)
            tr_test.append(tr)
            n_test += len(d)

    print(f"\n{os.path.basename(args.bem_dir.rstrip('/'))}")
    print(f"{'':16s} {'rows':>9s}  {'Fxy':>6s} {'Fz':>6s}  {'Mxy':>7s} {'Mz':>7s}  {'F':>6s} {'M':>7s}")
    report("full dataset", np.vstack(fr_all), np.vstack(tr_all), n_all)
    if fr_test:
        report("test set", np.vstack(fr_test), np.vstack(tr_test), n_test)
    else:
        print("(no test-set segments found)")


if __name__ == "__main__":
    main()
