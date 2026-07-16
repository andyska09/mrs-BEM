"""Shared helpers for the NeuroBEM EDA / analysis notebooks."""

from pathlib import Path
import re

import numpy as np
import pandas as pd
from scipy import signal

REPO_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = REPO_ROOT / "processed_data"
BEM_DIR = DATA_DIR / "bem-tuned"
BEM_PREFIX = "bem-tuned"  # file prefix inside BEM_DIR; "bem" for the other tunes
FS = 400.0

MASS = 0.772
INERTIA = np.array([0.00254, 0.00214, 0.00436])
R = 5.1 * 2.54 / 2 * 1e-2  # prop radius, params.h

COLUMNS = [
    "t",
    "ang_acc_x",
    "ang_acc_y",
    "ang_acc_z",
    "ang_vel_x",
    "ang_vel_y",
    "ang_vel_z",
    "qx",
    "qy",
    "qz",
    "qw",
    "acc_x",
    "acc_y",
    "acc_z",
    "vel_x",
    "vel_y",
    "vel_z",
    "pos_x",
    "pos_y",
    "pos_z",
    "mot_1",
    "mot_2",
    "mot_3",
    "mot_4",
    "dmot_1",
    "dmot_2",
    "dmot_3",
    "dmot_4",
    "vbat",
]
PRED = ["fx", "fy", "fz", "tx", "ty", "tz"]
# DIAG = [f"{m}_{i}" for i in range(1, 5) for m in ("vi", "mu", "as")]
BEM_COLUMNS = COLUMNS + PRED

VI = [35, 38, 41, 44]  # induced velocity per motor
MU = [36, 39, 42, 45]  # advance ratio per motor
AS = [37, 40, 43, 46]  # shaft angle of attack per motor


def flight_ids():
    r = re.compile(r"merged_(.+)_seg_\d+\.csv")
    return sorted(
        {r.match(p.name).group(1) for p in DATA_DIR.glob("merged_*_seg_*.csv")}
    )


def load_flight(flight_id):
    files = sorted(DATA_DIR.glob(f"merged_{flight_id}_seg_*.csv"))
    return pd.concat(
        [pd.read_csv(f, header=0, names=COLUMNS) for f in files], ignore_index=True
    )


def load_largest_segment(flight_id):
    files = sorted(DATA_DIR.glob(f"merged_{flight_id}_seg_*.csv"))
    path = max(files, key=lambda p: p.stat().st_size)
    return pd.read_csv(path, header=0, names=COLUMNS)


def bem_flight_ids():
    r = re.compile(rf"{re.escape(BEM_PREFIX)}_(.+)_seg_\d+\.csv")
    return sorted(
        {r.match(p.name).group(1) for p in BEM_DIR.glob(f"{BEM_PREFIX}_*_seg_*.csv")}
    )


def load_bem_flight(flight_id):
    files = sorted(BEM_DIR.glob(f"{BEM_PREFIX}_{flight_id}_seg_*.csv"))
    return pd.concat(
        [pd.read_csv(f, header=None, names=BEM_COLUMNS) for f in files],
        ignore_index=True,
    )


def load_bem(bem_dir=BEM_DIR):
    files = sorted(Path(bem_dir).glob("bem_*.csv"))
    if not files:
        raise SystemExit(f"No bem_*.csv in {bem_dir}")
    return np.vstack([np.loadtxt(f, delimiter=",") for f in files])


def measured(d):
    f = MASS * d[:, 11:14]
    t = INERTIA * d[:, 1:4] + np.cross(d[:, 4:7], INERTIA * d[:, 4:7])
    return f, t


def residuals(d):
    f, t = measured(d)
    return f - d[:, 29:32], t - d[:, 32:35]


def diagnostics(d):
    return d[:, VI], d[:, MU], d[:, AS]  # each (N, 4)


def lowpass(x, cutoff=25.0, fs=FS):
    b, a = signal.butter(4, cutoff / (fs / 2), "low")
    return signal.filtfilt(b, a, np.asarray(x, float))


def snr_db(x, cutoff=25.0):
    x = np.asarray(x, float)
    s = lowpass(x, cutoff)
    return 10 * np.log10(np.var(s) / np.var(x - s))


def noise_std(x, cutoff=25.0):
    r = np.asarray(x, float) - lowpass(x, cutoff)
    return float(1.4826 * np.median(np.abs(r - np.median(r))))


def snr_table(df, cutoff=25.0):
    cols = [c for c in df.columns if c != "t"]
    snr = {c: snr_db(df[c].to_numpy(), cutoff) for c in cols}
    return pd.Series(snr, name="snr_db").sort_values().to_frame()


def noise_corpus(cache=None):
    if cache and Path(cache).exists():
        return pd.read_csv(cache)
    rows = []
    for fid in flight_ids():
        df = load_flight(fid)
        for c in df.columns:
            if c == "t":
                continue
            x = df[c].to_numpy()
            rows.append(
                {
                    "flight": fid,
                    "channel": c,
                    "snr_db": snr_db(x),
                    "noise_std": noise_std(x),
                }
            )
    out = pd.DataFrame(rows)
    if cache:
        out.to_csv(cache, index=False)
    return out


def integrity_report(df):
    dt = np.diff(df["t"].to_numpy())
    return {
        "n_samples": len(df),
        "duration_s": round(float(dt.sum()), 2),
        "dt_std_ms": round(float(dt.std()) * 1e3, 4),
        "n_nan": int(df.isna().sum().sum()),
        "n_gaps": int((dt > 1.5 / FS).sum()),
    }
