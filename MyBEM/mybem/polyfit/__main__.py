"""Identify a polyfit model: writes coeffs.txt and the model config that loads it."""

import argparse
import os

from ..columns import load
from ..data import segment_ids, split
from ..drone import Drone
from ..metrics import COLS
from ..paths import MODELS, POLYFIT
from .fit import geometry, identify, score
from .physics import Geometry, hover_ct
from .terms import AXES


def write_coeffs(path, model, header):
    with open(path, "w") as f:
        for line in header:
            f.write(f"# {line}\n")
        for axis in AXES:
            for k, b in enumerate(model["axes"][axis]):
                f.write(f"# {axis} bin {k}: {len(b['terms']) - 1} terms  "
                        f"R2={b['r2']:.4f}  rms={b['rms']:.4g}  n={b['n']}\n")
        f.write("beta_edges " + " ".join(f"{e:g}" for e in model["beta_edges"]) + "\n")
        for axis in AXES:
            for k, b in enumerate(model["axes"][axis]):
                for term, coef in b["terms"].items():
                    f.write(f"{axis} {k} {term} {coef:.12g}\n")


def write_config(path, name, drone, model, coeffs):
    geo = geometry(model)
    with open(path, "w") as f:
        f.write(f"name: {name}\n")
        f.write(f"drone: {drone}\n")
        f.write("models:\n")
        f.write("  - type: polyfit\n")
        f.write(f"    radius: {geo.radius:.12g}\n")
        f.write(f"    air_density: {geo.air_density:.12g}\n")
        f.write(f"    ct_hover: {model['ct_hover']:.12g}\n")
        f.write(f"    ref_length: {geo.ref_length:.12g}\n")
        f.write(f"    coeffs: {os.path.relpath(coeffs, path.parent)}\n")
        f.write("airframe:\n")
        f.write(f"  dx: {geo.dx:.12g}\n")
        f.write(f"  dy: {geo.dy:.12g}\n")


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

    ct = hover_ct((load(s) for s in train), drone, geo)
    print(f"train={len(train)} test={len(test)} bins={a.bins} Ct,h={ct:.5f}")
    model = identify(train, drone, geo, ct, a.bins)

    out = POLYFIT / a.name
    out.mkdir(parents=True, exist_ok=True)
    coeffs = out / "coeffs.txt"
    config = MODELS / f"{a.name}.yaml"
    write_coeffs(coeffs, model,
                 [f"polyfit {a.name}",
                  f"split={a.split} drone={a.drone} bins={a.bins} "
                  f"segments={len(train)}"])
    write_config(config, a.name, a.drone, model, coeffs)

    print(f"\n{'':10s}" + "".join(f"{k:>9s}" for k in COLS))
    for fold, sids in (("train", train), ("test", test)):
        m = score(model, sids, drone)
        print(f"{fold:10s}" + "".join(f"{m[k]:9.3f}" for k in COLS))
    print(f"\nwrote {coeffs}\nwrote {config}")


if __name__ == "__main__":
    main()
