"""Evaluate a trained network in the format of paper Table II."""

import argparse

import numpy as np
import torch
import yaml

from .data import Data, segment_ids, split
from .drone import Drone
from .nets import create_net
from .train import NETS, normalize

# RSS21_Bauersfeld.md:437-443
PAPER = {"BEM": [0.803, 1.265, 0.982, 0.090, 0.017, 0.074],
         "BEM+NN": [0.204, 0.504, 0.335, 0.014, 0.004, 0.012]}
COLS = ["Fxy", "Fz", "F", "Mxy", "Mz", "M"]


def rmse(res):
    """Per-axis RMSE collapsed the way the paper reports it."""
    a = np.sqrt((res ** 2).mean(0))
    return [np.sqrt((a[0] ** 2 + a[1] ** 2) / 2), a[2], np.sqrt((a ** 2).mean())]


def table(force, torque):
    return rmse(force) + rmse(torque)


def run(name, fold="test", verbose=True):
    out = NETS / name
    with open(out / "config.yaml") as f:
        cfg = yaml.safe_load(f)
    with open(out / "normalization.yaml") as f:
        norm = yaml.safe_load(f)
    history = cfg["history"]
    device = torch.device(cfg["device"])
    torch.set_num_threads(cfg["threads"])

    ids = cfg["segments"][fold] if "segments" in cfg else split(cfg["split"], segment_ids())[fold]
    data = Data(cfg["preds"], ids, cfg["features"], Drone.load(cfg["drone"]))
    starts = data.windows(history)  # evaluation always sees the full envelope
    feat, _ = normalize(data, norm, device)
    starts_t = torch.as_tensor(starts, device=device)

    ckpt = torch.load(out / "model.pt", map_location=device, weights_only=False)
    net = create_net(ckpt["net"], history, feat.shape[1]).to(device)
    net.load_state_dict(ckpt["state_dict"])
    net.eval()

    pred = []
    off = torch.arange(history, device=device)
    with torch.no_grad():
        for i in range(0, len(starts_t), 4096):
            s = starts_t[i:i + 4096]
            pred.append(net(feat[s[:, None] + off]))
    pred = torch.cat(pred).cpu().numpy() * np.asarray(norm["stds_out"]) \
        + np.asarray(norm["means_out"])

    label = data.label[starts + history - 1].astype(float)
    left = label - pred

    every = data.label.astype(float)
    rows = {"BEM": table(every[:, :3], every[:, 3:]),
            "BEM+NN": table(left[:, :3], left[:, 3:])}
    if verbose:
        print(f"\n{name} on {fold}: {len(ids)} segments, {len(starts)} windows "
              f"(best epoch {ckpt['epoch'] + 1}, val loss {ckpt['val_loss']:.4f})\n")
        print(f"{'':12}{'Fxy':>7} {'Fz':>7} {'F':>7}   {'Mxy':>8} {'Mz':>8} {'M':>8}")
        for k, v in list(rows.items()) + [(f"paper {k}", v) for k, v in PAPER.items()]:
            print(f"{k:12}{v[0]:7.3f} {v[1]:7.3f} {v[2]:7.3f}   "
                  f"{v[3]:8.4f} {v[4]:8.4f} {v[5]:8.4f}")
    return rows


def main():
    p = argparse.ArgumentParser()
    p.add_argument("net")
    p.add_argument("--on", default="test", choices=["train", "val", "test"])
    a = p.parse_args()
    run(a.net, a.on)


if __name__ == "__main__":
    main()
