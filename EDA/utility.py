"""Helpers for the NeuroBEM noise / data-quality EDA."""
from pathlib import Path
import re

import numpy as np
import pandas as pd
from scipy import signal

DATA_DIR = Path(__file__).resolve().parent.parent / "processed_data"
FS = 400.0

COLUMNS = [
    "t",
    "ang_acc_x", "ang_acc_y", "ang_acc_z",
    "ang_vel_x", "ang_vel_y", "ang_vel_z",
    "qx", "qy", "qz", "qw",
    "acc_x", "acc_y", "acc_z",
    "vel_x", "vel_y", "vel_z",
    "pos_x", "pos_y", "pos_z",
    "mot_1", "mot_2", "mot_3", "mot_4",
    "dmot_1", "dmot_2", "dmot_3", "dmot_4",
    "vbat",
]


def load_largest_segment(flight_id):
    files = sorted(DATA_DIR.glob(f"merged_{flight_id}_seg_*.csv"))
    path = max(files, key=lambda p: p.stat().st_size)
    return pd.read_csv(path, header=0, names=COLUMNS)


def flight_ids():
    r = re.compile(r"merged_(.+)_seg_\d+\.csv")
    return sorted({r.match(p.name).group(1) for p in DATA_DIR.glob("merged_*_seg_*.csv")})


def load_flight(flight_id):
    files = sorted(DATA_DIR.glob(f"merged_{flight_id}_seg_*.csv"))
    return pd.concat([pd.read_csv(f, header=0, names=COLUMNS) for f in files],
                     ignore_index=True)


BEM_DIR = DATA_DIR / "bem"
PRED = ["fx", "fy", "fz", "tx", "ty", "tz"]


def bem_flight_ids():
    r = re.compile(r"bem_(.+)_seg_\d+\.csv")
    return sorted({r.match(p.name).group(1) for p in BEM_DIR.glob("bem_*_seg_*.csv")})


def load_bem_flight(flight_id):
    files = sorted(BEM_DIR.glob(f"bem_{flight_id}_seg_*.csv"))
    return pd.concat([pd.read_csv(f, header=None, names=COLUMNS + PRED) for f in files],
                     ignore_index=True)


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
            rows.append({"flight": fid, "channel": c,
                         "snr_db": snr_db(x), "noise_std": noise_std(x)})
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
