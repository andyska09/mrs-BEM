"""Train a network on base-model residuals."""

import argparse
import hashlib
import json
import math
import time

import numpy as np
import torch
import yaml
from torch.utils.tensorboard import SummaryWriter
from tqdm import tqdm

from .data import Data, segment_ids, split
from .drone import Drone
from .nets import create_net
from .paths import NETS


def digest(cfg):
    return hashlib.sha256(json.dumps(cfg, sort_keys=True).encode()).hexdigest()[:6]


def cosine_restarts(step, first_decay_steps, t_mul, m_mul, alpha):
    """tf.keras.experimental.CosineDecayRestarts as a step -> lr multiplier."""
    frac = step / first_decay_steps
    if t_mul == 1.0:
        i, frac = math.floor(frac), frac % 1.0
    else:
        i = math.floor(math.log(1 - frac * (1 - t_mul)) / math.log(t_mul))
        frac = (frac - (1 - t_mul ** i) / (1 - t_mul)) / t_mul ** i
    return (1 - alpha) * 0.5 * m_mul ** i * (1 + math.cos(math.pi * frac)) + alpha


def loss_fn(pred, label, cfg):
    """Per-axis weighted MSE on normalized residuals, force and torque separately."""
    se = (pred - label) ** 2
    w = torch.as_tensor(cfg["axis_weight_force"] + cfg["axis_weight_torque"],
                        dtype=pred.dtype, device=pred.device)
    w = w * torch.as_tensor([cfg["weight_force"]] * 3 + [cfg["weight_torque"]] * 3,
                            dtype=pred.dtype, device=pred.device)
    return se.mean(0) @ w


def batches(feat, label, starts, history, size, gen=None):
    order = (torch.randperm(len(starts), generator=gen).to(starts.device)
             if gen is not None else torch.arange(len(starts), device=starts.device))
    off = torch.arange(history, device=starts.device)
    for i in range(0, len(starts), size):
        s = starts[order[i:i + size]]
        yield feat[s[:, None] + off], label[s + history - 1]


def normalize(data, norm, device):
    def t(a):
        return torch.as_tensor(np.asarray(a), dtype=torch.float32, device=device)
    return ((t(data.feat) - t(norm["means_in"])) / t(norm["stds_in"]),
            (t(data.label) - t(norm["means_out"])) / t(norm["stds_out"]))


def run(exp_path, seed=None, limit=None, epochs=None, device=None, threads=1):
    with open(exp_path) as f:
        cfg = yaml.safe_load(f)
    # Machine settings are not part of the experiment, so not part of its hash.
    cfg.pop("device", None)
    cfg.pop("threads", None)
    if seed is not None:
        cfg["seed"] = seed
    if epochs is not None:
        cfg["optim"]["epochs"] = epochs
    history, cut = cfg["history"], cfg["max_speed"]
    device = torch.device(device or "cpu")
    # The model is ~28k params; multithreading tiny ops costs more than it saves.
    torch.set_num_threads(threads)
    drone = Drone.load(cfg["drone"])

    folds = split(cfg["split"], segment_ids())
    if limit:
        folds = {k: v[:limit] for k, v in folds.items()}
    print(f"{cfg['name']}  preds={cfg['preds']}  split={cfg['split']}  "
          f"device={device}  "
          f"train/val/test = {len(folds['train'])}/{len(folds['val'])}/{len(folds['test'])}")

    train = Data(cfg["preds"], folds["train"], cfg["features"], drone)
    val = Data(cfg["preds"], folds["val"], cfg["features"], drone)
    norm = train.normalization(cut)
    tw, vw = train.windows(history, cut), val.windows(history, cut)
    print(f"rows {len(train.feat)}/{len(val.feat)}   windows {len(tw)}/{len(vw)}")

    saved = {**cfg, "segments": folds}
    h = digest(saved)
    # Runs sharing a group differ only by seed, which is what report.py averages.
    saved["group"] = digest({k: v for k, v in saved.items() if k != "seed"})
    out = NETS / f"{cfg['name']}@{h}"
    (out / "tb").mkdir(parents=True, exist_ok=True)
    with open(out / "config.yaml", "w") as f:
        yaml.safe_dump(saved, f, sort_keys=False)
    with open(out / "normalization.yaml", "w") as f:
        yaml.safe_dump({"history": history, "features": cfg["features"],
                        "mass": drone.mass, "inertia": drone.inertia.tolist(),
                        **{k: v.tolist() for k, v in norm.items()}}, f, sort_keys=False)

    tf, tl = normalize(train, norm, device)
    vf, vl = normalize(val, norm, device)
    tw = torch.as_tensor(tw, device=device)
    vw = torch.as_tensor(vw, device=device)
    scale = torch.as_tensor(norm["stds_out"], dtype=torch.float32, device=device)

    torch.manual_seed(cfg["seed"])
    net = create_net(cfg["net"], history, tf.shape[1]).to(device)
    print(f"{cfg['net']['arch']}: {sum(p.numel() for p in net.parameters())} parameters")

    o = cfg["optim"]
    opt = torch.optim.Adam(net.parameters(), lr=o["lr"], weight_decay=o["l2"])
    sched = torch.optim.lr_scheduler.LambdaLR(
        opt, lambda s: cosine_restarts(s, **o["scheduler"]))
    gen = torch.Generator().manual_seed(cfg["seed"])
    writer = SummaryWriter(out / "tb")

    best = math.inf
    for epoch in range(o["epochs"]):
        t0 = time.time()
        net.train()
        total = n = 0
        for x, y in tqdm(batches(tf, tl, tw, history, cfg["batch_size"], gen),
                         total=(len(tw) + cfg["batch_size"] - 1) // cfg["batch_size"],
                         desc=f"epoch {epoch + 1}/{o['epochs']}", leave=False):
            loss = loss_fn(net(x), y, cfg["loss"])
            opt.zero_grad(set_to_none=True)
            loss.backward()
            opt.step()
            sched.step()
            total += loss.item()
            n += 1
        train_loss = total / n

        net.eval()
        total = n = 0
        err = []
        with torch.no_grad():
            for x, y in batches(vf, vl, vw, history, 4096):
                p = net(x)
                total += loss_fn(p, y, cfg["loss"]).item()
                n += 1
                err.append((p - y) * scale)
        val_loss = total / n
        err = torch.cat(err)
        rf = err[:, :3].pow(2).mean().sqrt().item()
        rt = err[:, 3:].pow(2).mean().sqrt().item()

        for k, v in [("Train/Loss", train_loss), ("Val/Loss", val_loss),
                     ("Val/Force_RMSE", rf), ("Val/Torque_RMSE", rt),
                     ("Train/LR", sched.get_last_lr()[0])]:
            writer.add_scalar(k, v, epoch)
        print(f"epoch {epoch + 1:3d}  train {train_loss:.4f}  val {val_loss:.4f}  "
              f"F {rf:.3f} N  M {rt:.4f} Nm  {time.time() - t0:.0f}s"
              + ("  *" if val_loss < best else ""))

        if val_loss < best:
            best = val_loss
            torch.save({"state_dict": net.state_dict(), "net": cfg["net"],
                        "history": history, "features": cfg["features"],
                        "epoch": epoch, "val_loss": val_loss}, out / "model.pt")
    writer.close()
    print(f"\nbest val loss {best:.4f} -> {out}")
    return out


def main():
    p = argparse.ArgumentParser()
    p.add_argument("experiment")
    p.add_argument("--seed", type=int)
    p.add_argument("--limit", type=int, help="use only the first N segments per fold")
    p.add_argument("--epochs", type=int)
    p.add_argument("--device", choices=["cpu", "cuda", "mps"])
    p.add_argument("--threads", type=int, default=1)
    a = p.parse_args()
    run(a.experiment, a.seed, a.limit, a.epochs, a.device, a.threads)


if __name__ == "__main__":
    main()
