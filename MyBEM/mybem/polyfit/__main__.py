"""Identify a polyfit model and write polyfit.yaml + the C++ coeffs.txt."""

import argparse

import yaml

from ..columns import load
from ..data import segment_ids, split
from ..drone import Drone
from ..metrics import COLS
from ..paths import POLYFIT
from .fit import identify, score
from .physics import Geometry, hover_ct
from .terms import AXES


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

    ct = hover_ct(load(train[0]), drone, geo)
    print(f"train={len(train)} test={len(test)} bins={a.bins} Ct,h={ct:.5f}")
    model = identify(train, drone, geo, ct, a.bins)

    out = POLYFIT / a.name
    out.mkdir(parents=True, exist_ok=True)
    with open(out / "polyfit.yaml", "w") as f:
        yaml.safe_dump(model, f, sort_keys=False)
    with open(out / "coeffs.txt", "w") as f:
        f.write("beta_edges " + " ".join(f"{e:g}" for e in model["beta_edges"]) + "\n")
        for axis in AXES:
            for k, b in enumerate(model["axes"][axis]):
                for term, coef in b["terms"].items():
                    f.write(f"{axis} {k} {term} {coef:.12g}\n")

    print(f"\n{'':10s}" + "".join(f"{k:>9s}" for k in COLS))
    for fold, sids in (("train", train), ("test", test)):
        m = score(model, sids, drone)
        print(f"{fold:10s}" + "".join(f"{m[k]:9.3f}" for k in COLS))
    print(f"\nwrote {out / 'polyfit.yaml'}")


if __name__ == "__main__":
    main()
