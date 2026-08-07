#!/usr/bin/env python3
"""Submit mybem.train jobs to MetaCentrum PBS.

Each (experiment, seed) pair becomes one qsub. See metacentrum.md.
"""

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JOB = Path(__file__).resolve().parent / "job_train.sh"

def submit(exp, seed, args):
    qsub_vars = [f"ROOT={ROOT}", f"EXP={exp}", f"SEED={seed}"]
    cmd = [
        "qsub",
        "-N", f"train_{Path(exp).stem}_s{seed}",
        "-l", f"select=1:ncpus={args.ncpus}:mem={args.mem}",
        "-l", f"walltime={args.walltime}",
        "-j", "oe",
        "-o", "logs/",
        "-v", ",".join(qsub_vars),
        str(JOB),
    ]
    if args.dry_run:
        print("DRY RUN:", " ".join(cmd))
        return
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"  ERROR: {r.stderr.strip()}", file=sys.stderr)
    else:
        print(f"  Submitted: {r.stdout.strip()}")


def main():
    p = argparse.ArgumentParser(description="Submit mybem.train jobs to MetaCentrum PBS")
    p.add_argument("--exp", nargs="+", default=["tcn_baseline.yaml"],
                   help="experiment yaml(s) under configs/experiments/")
    p.add_argument("--seeds", nargs="+", type=int, default=[0])
    p.add_argument("--ncpus", type=int, default=2,
                   help="torch runs single-threaded; 2 covers the loader")
    p.add_argument("--mem", default="8gb")
    p.add_argument("--walltime", default="8:00:00")
    p.add_argument("--dry-run", action="store_true", dest="dry_run")
    args = p.parse_args()

    for e in args.exp:
        if not (ROOT / "configs" / "experiments" / e).exists() and not args.dry_run:
            sys.exit(f"experiment not found: configs/experiments/{e}")
    Path("logs").mkdir(exist_ok=True)

    jobs = [(e, s) for e in args.exp for s in args.seeds]
    print(f"Submitting {len(jobs)} job(s)...")
    for exp, seed in jobs:
        print(f"\n[exp={exp} seed={seed}]")
        submit(exp, seed, args)


if __name__ == "__main__":
    main()
