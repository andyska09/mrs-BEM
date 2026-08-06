"""Segments in, windowed training tensors out.

Residuals are computed in RAM and never written.
"""

import numpy as np
import pandas as pd
import yaml

from .drone import ANGVEL, ATT, LINVEL, MOTORS, POS, ROOT

DATA = ROOT.parent / "data" / "processed_data"
PREDS = ROOT / "store" / "preds"

FEATURES = {"pos": POS, "att": ATT, "angvel": ANGVEL, "linvel": LINVEL, "motors": MOTORS}


def segment_ids(source=DATA):
    return sorted(p.name[len("merged_"):-len(".csv")]
                  for p in source.glob("merged_*_seg_*.csv"))


def split(name, ids):
    """configs/splits/<name>.yaml -> {test, val, train}.

    Pinned id lists are taken out first, then the rest is shuffled once with the
    split's seed and the sized folds are cut off it. Fold order in the yaml is
    irrelevant; train is whatever is left.
    """
    with open(ROOT / "configs" / "splits" / f"{name}.yaml") as f:
        cfg = yaml.safe_load(f)

    rules = {k: v for k, v in cfg.items() if k not in ("name", "seed")}
    folds = {k: v for k, v in rules.items() if isinstance(v, list)}
    for fold, picked in folds.items():
        unknown = set(picked) - set(ids)
        if unknown:
            raise SystemExit(f"split '{fold}': unknown ids {sorted(unknown)}")

    taken = {i for f in folds.values() for i in f}
    pool = [i for i in ids if i not in taken]
    pool = [pool[i] for i in np.random.default_rng(cfg["seed"]).permutation(len(pool))]

    for fold in sorted(k for k, v in rules.items() if not isinstance(v, list)):
        r = rules[fold]
        n = r["n"] if "n" in r else round(r["frac"] * len(pool))
        folds[fold], pool = sorted(pool[:n]), pool[n:]
    folds["train"] = sorted(pool)
    return folds


class Data:
    """All selected segments in one array, plus per-segment (start, stop) bounds."""

    def __init__(self, preds, ids, features, drone, source=DATA):
        feat, label, base, bounds, n = [], [], [], [], 0
        for sid in ids:
            d = pd.read_csv(source / f"merged_{sid}.csv").to_numpy(float)
            p = pd.read_csv(PREDS / preds / f"{sid}.csv", header=None).to_numpy(float)
            if len(p) != len(d):
                raise SystemExit(f"{sid}: {len(p)} prediction rows vs {len(d)} data rows")
            if np.abs(p[:, 0] - d[:, 0]).max() > 1e-9:
                raise SystemExit(f"{sid}: prediction time column does not match the data")

            fm, tm = drone.measured(d)
            label.append(np.hstack([fm - p[:, 1:4], tm - p[:, 4:7]]))
            feat.append(np.hstack([d[:, FEATURES[g]] for g in features]))
            base.append(p[:, 1:7])
            bounds.append((n, n + len(d)))
            n += len(d)

        self.ids = list(ids)
        self.bounds = bounds
        self.feat = np.vstack(feat).astype(np.float32)
        self.label = np.vstack(label).astype(np.float32)
        self.base = np.vstack(base)
        self.speed = (np.linalg.norm(self.feat[:, _at(features, "linvel")], axis=1)
                      if "linvel" in features else None)

    def windows(self, history, max_speed=0):
        """Start indices of windows inside one segment whose full span passes the filter."""
        ok = None if not max_speed else self.speed <= max_speed
        out = []
        for lo, hi in self.bounds:
            s = np.arange(lo, hi - history + 1)
            if ok is not None:
                bad = np.concatenate([[0], np.cumsum(~ok[lo:hi])])
                s = s[bad[s - lo + history] - bad[s - lo] == 0]
            out.append(s)
        return np.concatenate(out)

    def normalization(self, max_speed=0):
        rows = slice(None) if not max_speed else self.speed <= max_speed
        f, l = self.feat[rows], self.label[rows]
        return {"means_in": f.mean(0), "stds_in": f.std(0),
                "means_out": l.mean(0), "stds_out": l.std(0)}


def _at(features, group):
    off = sum(len(FEATURES[g]) for g in features[:features.index(group)])
    return slice(off, off + len(FEATURES[group]))
