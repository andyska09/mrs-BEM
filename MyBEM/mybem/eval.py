"""Evaluate any model in the format of paper Table II.

A model is a folder name, hash included: a prediction folder under store/preds (a base
model applied by mybem-apply) or a run under store/nets (base model + residual net).
Every model is scored against the same measurement on the same rows.
"""

import argparse

import numpy as np
import torch
import yaml

from .columns import load
from .data import DATA, Data, preds_dir, read_preds, segment_ids, split
from .drone import Drone
from .metrics import PAPER, table
from .nets import create_net
from .paths import NETS
from .train import normalize


def resolve(name):
    if (NETS / name / "model.pt").exists():
        return "net", NETS / name
    return "base", preds_dir(name)


def base_predict(path, ids, source=DATA):
    return np.vstack([read_preds(path, sid, load(sid, source)) for sid in ids])


def net_predict(path, ids, drone, device=None):
    """Base prediction plus the net's residual, NaN where a window does not fit."""
    with open(path / "config.yaml") as f:
        cfg = yaml.safe_load(f)
    with open(path / "normalization.yaml") as f:
        norm = yaml.safe_load(f)
    history = cfg["history"]
    device = torch.device(device or "cpu")
    torch.set_num_threads(1)

    data = Data(cfg["preds"], ids, cfg["features"], drone)
    feat, _ = normalize(data, norm, device)
    starts = data.windows(history)  # evaluation always sees the full envelope

    ckpt = torch.load(path / "model.pt", map_location=device, weights_only=False)
    net = create_net(ckpt["net"], history, feat.shape[1]).to(device)
    net.load_state_dict(ckpt["state_dict"])
    net.eval()

    starts_t = torch.as_tensor(starts, device=device)
    off = torch.arange(history, device=device)
    out = []
    with torch.no_grad():
        for i in range(0, len(starts_t), 4096):
            s = starts_t[i:i + 4096]
            out.append(net(feat[s[:, None] + off]))
    res = torch.cat(out).cpu().numpy() * np.asarray(norm["stds_out"]) \
        + np.asarray(norm["means_out"])

    pred = np.full_like(data.base, np.nan)
    ends = starts + history - 1
    pred[ends] = data.base[ends] + res
    return pred


def evaluate(names, ids, drone, device=None, source=DATA):
    """{name: Table II row}, all models scored on the rows every model predicts."""
    meas = np.vstack([np.hstack(drone.measured(load(sid, source))) for sid in ids])

    pred = {}
    for name in names:
        kind, path = resolve(name)
        pred[name] = (base_predict(path, ids, source) if kind == "base"
                      else net_predict(path, ids, drone, device))

    ok = np.all([np.isfinite(p).all(1) for p in pred.values()], axis=0)
    rows = {n: table((meas - p)[ok, :3], (meas - p)[ok, 3:]) for n, p in pred.items()}
    return rows, int(ok.sum())


def print_table(rows):
    w = max([len(k) for k in rows] + [len(k) + 6 for k in PAPER]) + 2
    print(f"{'':{w}}{'Fxy':>7} {'Fz':>7} {'F':>7}   {'Mxy':>8} {'Mz':>8} {'M':>8}")
    for k, v in list(rows.items()) + [(f"paper {k}", v) for k, v in PAPER.items()]:
        print(f"{k:{w}}{v[0]:7.3f} {v[1]:7.3f} {v[2]:7.3f}   "
              f"{v[3]:8.4f} {v[4]:8.4f} {v[5]:8.4f}")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("models", nargs="+",
                   help="folder names under store/preds or store/nets, e.g. "
                        "bem_default@4bf0d0 arch_lstm_64@d1ef8d")
    p.add_argument("--on", default="test", choices=["train", "val", "test"])
    p.add_argument("--split", default="paper")
    p.add_argument("--drone", default="paper_quad")
    p.add_argument("--device", choices=["cpu", "cuda", "mps"])
    a = p.parse_args()

    ids = split(a.split, segment_ids())[a.on]
    rows, n = evaluate(a.models, ids, Drone.load(a.drone), a.device)
    print(f"\n{a.split}/{a.on}: {len(ids)} segments, {n} rows\n")
    print_table(rows)


if __name__ == "__main__":
    main()
