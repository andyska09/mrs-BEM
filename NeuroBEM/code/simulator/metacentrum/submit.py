#!/usr/bin/env python3
"""Submit CMA-ES aero re-identification jobs to MetaCentrum PBS.
Run from anywhere: python3 metacentrum/submit.py [--mask ...] [--loss both]
Each (mask, loss) pair becomes one qsub. See metacentrum.md for the guide."""

import argparse
import subprocess
import sys
from pathlib import Path

SIMDIR = Path(__file__).resolve().parent.parent
DEFAULT_DATA = SIMDIR.parent.parent / "CMAES-dataset" / "subset_20k.csv"
FULL_MASK = "1" * 19  # all 19 tunable REGISTRY params free (tail num_blades/air_density fixed)
JOB_SCRIPT = SIMDIR / "metacentrum" / "job.sh"


def submit(mask: str, loss: str, data: Path, args) -> None:
    outdir = data.parent.parent / "CMAES-results"
    qsub_vars = [
        f"SIMDIR={SIMDIR}",
        f"DATA={data}",
        f"OUTDIR={outdir}",
        f"MASK={mask}",
        f"LOSS={loss}",
        f"NCPUS={args.ncpus}",
    ]
    cmd = [
        "qsub",
        "-N", f"cmaes_{loss}_{mask.count('1')}p",
        "-l", f"select=1:ncpus={args.ncpus}:mem={args.mem}:scratch_local=4gb",
        "-l", f"walltime={args.walltime}",
        "-j", "oe",
        "-o", "logs/",
        "-v", ",".join(qsub_vars),
        str(JOB_SCRIPT),
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
    p = argparse.ArgumentParser(description="Submit CMA-ES jobs to MetaCentrum PBS")
    p.add_argument("--mask", nargs="+", default=[FULL_MASK],
                   help="binary registry mask(s); one job each (default: 19 tunable params free)")
    p.add_argument("--loss", nargs="+", default=["both"], choices=["force", "torque", "both"])
    p.add_argument("--data", type=Path, default=DEFAULT_DATA)
    p.add_argument("--ncpus", type=int, default=12, help="cores per job; job.sh passes nproc to cmaes --threads")
    p.add_argument("--mem", default="8gb")
    p.add_argument("--walltime", default="4:00:00")
    p.add_argument("--dry-run", action="store_true", dest="dry_run")
    args = p.parse_args()

    if not args.data.exists() and not args.dry_run:
        sys.exit(f"data CSV not found: {args.data}")
    Path("logs").mkdir(exist_ok=True)

    jobs = [(m, l) for m in args.mask for l in args.loss]
    print(f"Submitting {len(jobs)} job(s)...")
    for mask, loss in jobs:
        print(f"\n[mask={mask} loss={loss}]")
        submit(mask, loss, args.data, args)


if __name__ == "__main__":
    main()
