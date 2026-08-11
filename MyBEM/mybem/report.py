"""Cross-run table over store/nets, grouped by experiment name, mean +- std over seeds."""

import argparse
from collections import defaultdict

import numpy as np
import torch
import yaml

from .data import segment_ids, split
from .drone import Drone
from .eval import evaluate
from .metrics import PAPER
from .paths import NETS


def collect(match):
    """Runs keyed by group: same experiment down to the seed. Value is (label, dirs)."""
    groups = defaultdict(list)
    for d in sorted(NETS.iterdir()):
        if not (d / "model.pt").exists():
            continue
        with open(d / "config.yaml") as f:
            cfg = yaml.safe_load(f)
        if match in cfg["name"]:
            groups[(cfg["name"], cfg.get("group", ""))].append((d, cfg))

    names = [n for n, _ in groups]
    return {(n if names.count(n) == 1 else f"{n}~{g}"): runs
            for (n, g), runs in groups.items()}


def stats(d):
    ckpt = torch.load(d / "model.pt", map_location="cpu", weights_only=False)
    n = sum(v.numel() for v in ckpt["state_dict"].values())
    return n, ckpt["val_loss"]


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--on", default="test", choices=["train", "val", "test"])
    p.add_argument("--match", default="", help="substring filter on the experiment name")
    p.add_argument("--quick", action="store_true", help="val loss only, skip evaluation")
    p.add_argument("--split", default="paper")
    p.add_argument("--drone", default="paper_quad")
    p.add_argument("--device", choices=["cpu", "cuda", "mps"])
    a = p.parse_args()

    groups = collect(a.match)
    if not groups:
        raise SystemExit(f"no finished runs matching '{a.match}' in {NETS}")

    rows = {}
    bases = sorted({cfg["preds"] for g in groups.values() for _, cfg in g})
    if not a.quick:
        names = [d.name for g in groups.values() for d, _ in g]
        rows, _ = evaluate(names + bases, split(a.split, segment_ids())[a.on],
                           Drone.load(a.drone), a.device)

    print(f"{'run':22}{'n':>2} {'params':>8} {'val':>8}", end="")
    if not a.quick:
        print(f"   {'Fxy':>13} {'Fz':>13} {'F':>13}   {'Mxy':>15} {'Mz':>15} {'M':>15}", end="")
    print()

    for name, runs in groups.items():
        params, val = zip(*[stats(d) for d, _ in runs])
        print(f"{name:22}{len(runs):>2} {params[0]:>8} {np.mean(val):>8.4f}", end="")
        if not a.quick:
            v = np.array([rows[d.name] for d, _ in runs])
            m, s = v.mean(0), v.std(0)
            for i in range(3):
                print(f"   {m[i]:6.3f}±{s[i]:.3f}", end="")
            for i in range(3, 6):
                print(f"   {m[i]:7.4f}±{s[i]:.4f}", end="")
        print()

    if not a.quick:
        for k, v in [*((b, rows[b]) for b in bases), *PAPER.items()]:
            print(f"{k if k in bases else 'paper ' + k:22}{'':2} {'':>8} {'':>8}", end="")
            for i in range(3):
                print(f"   {v[i]:6.3f}      ", end="")
            for i in range(3, 6):
                print(f"   {v[i]:7.4f}       ", end="")
            print()


if __name__ == "__main__":
    main()
