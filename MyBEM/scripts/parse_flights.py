"""NeuroBEM/Flights.txt -> MyBEM/configs/flights.csv, one row per segment.

Run once. The output is committed and hand-editable; the paper dataset is frozen,
so this is not a pipeline stage. `comment` is verbatim, so a bad parse loses nothing.
"""

import argparse
import csv
import re
from collections import Counter
from pathlib import Path

import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
LINVEL = [14, 15, 16]

# First match wins: "3d circle" and "battery test ... short circle" before "circle".
FAMILIES = [
    ("3d circle", "circle_3d"),
    ("wobbly circle", "wobbly_circle"),
    ("battery test", "battery_test"),
    ("linear oscillation", "oscillation_linear"),
    ("vertical oscillation", "oscillation_vertical"),
    ("random points", "random_points"),
    ("satellite", "satellite"),
    ("lemniscate", "lemniscate"),
    ("ellipse", "ellipse"),
    ("cpc", "cpc"),
    ("circle", "circle"),
]

LINE = re.compile(r'\s*dataset\s*=\s*"([^"]+)"\s*;\s*%\s*(.*)')


def parse(comment):
    c = comment.lower()
    vel = re.search(r"vel\s*=\s*([\d.]+)", c)
    twr = re.search(r"([\d.]+)\s*twr", c)
    return {"family": next((f for k, f in FAMILIES if k in c), "other"),
            "vel": vel.group(1) if vel else "",
            "twr": twr.group(1) if twr else "",
            "ccw": int("ccw" in c)}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--flights", type=Path, default=ROOT.parent / "NeuroBEM" / "Flights.txt")
    p.add_argument("--data", type=Path, default=ROOT.parent / "data" / "processed_data")
    p.add_argument("--out", type=Path, default=ROOT / "configs" / "flights.csv")
    a = p.parse_args()

    labels = {}
    for line in a.flights.read_text().splitlines():
        m = LINE.match(line)
        if m:
            labels[m.group(1)] = m.group(2).strip().rstrip(",")

    rows = []
    for path in sorted(a.data.glob("merged_*_seg_*.csv")):
        sid = path.name[len("merged_"):-len(".csv")]
        flight, seg = sid.rsplit("_seg_", 1)
        if flight not in labels:
            print(f"no Flights.txt entry for {flight}")
            continue
        v = pd.read_csv(path, usecols=LINVEL).to_numpy(float)
        rows.append({"id": sid, "flight": flight, "seg": int(seg),
                     **parse(labels[flight]), "n": len(v),
                     "vmax": round(float(np.linalg.norm(v, axis=1).max()), 3),
                     "comment": labels[flight]})

    if not rows:
        raise SystemExit(f"no segments under {a.data}")

    with open(a.out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)

    flights = {r["flight"] for r in rows}
    print(f"{len(rows)} segments, {len(flights)} flights -> {a.out}")
    for family, n in Counter(r["family"] for r in rows).most_common():
        print(f"  {family:22} {n:4d}")
    missing = sorted(set(labels) - flights)
    if missing:
        print(f"{len(missing)} flights with no segments: {' '.join(missing)}")


if __name__ == "__main__":
    main()
