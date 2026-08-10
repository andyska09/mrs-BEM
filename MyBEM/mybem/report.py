"""Cross-run table over store/nets, grouped by experiment name, mean +- std over seeds."""

import argparse
from collections import defaultdict

import numpy as np
import torch
import yaml

from .eval import PAPER, run
from .train import NETS


def collect(match):
    groups = defaultdict(list)
    for d in sorted(NETS.iterdir()):
        if not (d / "model.pt").exists():
            continue
        with open(d / "config.yaml") as f:
            cfg = yaml.safe_load(f)
        if match in cfg["name"]:
            groups[cfg["name"]].append(d)
    return groups


def stats(d):
    ckpt = torch.load(d / "model.pt", map_location="cpu", weights_only=False)
    n = sum(v.numel() for v in ckpt["state_dict"].values())
    return n, ckpt["val_loss"]


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--on", default="test", choices=["train", "val", "test"])
    p.add_argument("--match", default="", help="substring filter on the experiment name")
    p.add_argument("--quick", action="store_true", help="val loss only, skip evaluation")
    a = p.parse_args()

    groups = collect(a.match)
    if not groups:
        raise SystemExit(f"no finished runs matching '{a.match}' in {NETS}")

    print(f"{'run':22}{'n':>2} {'params':>8} {'val':>8}", end="")
    if not a.quick:
        print(f"   {'Fxy':>13} {'Fz':>13} {'F':>13}   {'Mxy':>15} {'Mz':>15} {'M':>15}", end="")
    print()

    bem = None
    for name, dirs in groups.items():
        params, val = zip(*[stats(d) for d in dirs])
        print(f"{name:22}{len(dirs):>2} {params[0]:>8} {np.mean(val):>8.4f}", end="")
        if not a.quick:
            rows = [run(d.name, a.on, verbose=False) for d in dirs]
            bem = rows[0]["BEM"]
            v = np.array([r["BEM+NN"] for r in rows])
            m, s = v.mean(0), v.std(0)
            for i in range(3):
                print(f"   {m[i]:6.3f}±{s[i]:.3f}", end="")
            for i in range(3, 6):
                print(f"   {m[i]:7.4f}±{s[i]:.4f}", end="")
        print()

    if bem is not None:
        for k, v in [("BEM", bem), *PAPER.items()]:
            print(f"{k if k == 'BEM' else 'paper ' + k:22}{'':2} {'':>8} {'':>8}", end="")
            for i in range(3):
                print(f"   {v[i]:6.3f}      ", end="")
            for i in range(3, 6):
                print(f"   {v[i]:7.4f}       ", end="")
            print()


if __name__ == "__main__":
    main()
