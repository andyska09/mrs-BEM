#!/usr/bin/env python3
"""Submit mybem-tune CMA-ES jobs to MetaCentrum PBS.

Each (free set, loss) pair becomes one qsub. See metacentrum.md.
"""

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JOB = Path(__file__).resolve().parent / "job_tune.sh"
DEFAULT_DATA = ROOT.parent / "data" / "CMAES-subsets" / "subset_20k.csv"
DEFAULT_OUT = ROOT / "store" / "tune"


def run_name(free, loss, args):
    """<loss>_<n>p@<hash6> over everything that defines the fit."""
    key = "|".join([args.model, args.drone, args.data.name, free, loss,
                    str(args.gens), str(args.seed)])
    n = "all" if free == "all" else f"{len(free.split(','))}p"
    return f"{loss}_{n}@{hashlib.sha256(key.encode()).hexdigest()[:6]}"


def submit(free, loss, args):
    run = run_name(free, loss, args)
    qsub_vars = [
        f"ROOT={ROOT}", f"DATA={args.data}", f"OUTDIR={args.outdir}",
        f"MODEL={args.model}", f"DRONE={args.drone}",
        f"FREE={free.replace(',', '+')}",  # qsub -v is comma-separated
        f"LOSS={loss}", f"GENS={args.gens}", f"SEED={args.seed}",
        f"RUN={run}", f"NCPUS={args.ncpus}",
    ]
    cmd = [
        "qsub",
        "-N", f"tune_{run}",
        "-l", f"select=1:ncpus={args.ncpus}:ompthreads={args.ncpus}"
              f":mem={args.mem}:scratch_local=4gb",
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
        print(f"  Submitted: {r.stdout.strip()}  -> {args.outdir}/{run}")


def main():
    p = argparse.ArgumentParser(description="Submit mybem-tune jobs to MetaCentrum PBS")
    p.add_argument("--free", nargs="+", default=["all"],
                   help="tunable names, comma-separated; one job each (default: all)")
    p.add_argument("--loss", nargs="+", default=["both"],
                   choices=["force", "torque", "both"])
    p.add_argument("--model", default="bem_default.yaml", help="under configs/models/")
    p.add_argument("--drone", default="paper_quad.yaml", help="under configs/drones/")
    p.add_argument("--data", type=Path, default=DEFAULT_DATA)
    p.add_argument("--outdir", type=Path, default=DEFAULT_OUT)
    p.add_argument("--gens", type=int, default=100)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--ncpus", type=int, default=12,
                   help="cores per job; mybem-tune uses all allocated cores")
    p.add_argument("--mem", default="8gb")
    p.add_argument("--walltime", default="4:00:00")
    p.add_argument("--dry-run", action="store_true", dest="dry_run")
    args = p.parse_args()

    if not args.data.exists() and not args.dry_run:
        sys.exit(f"data CSV not found: {args.data}")
    for sub, f in (("models", args.model), ("drones", args.drone)):
        if not (ROOT / "configs" / sub / f).exists() and not args.dry_run:
            sys.exit(f"config not found: configs/{sub}/{f}")
    Path("logs").mkdir(exist_ok=True)

    jobs = [(f, l) for f in args.free for l in args.loss]
    print(f"Submitting {len(jobs)} job(s)...")
    for free, loss in jobs:
        print(f"\n[free={free} loss={loss}]")
        submit(free, loss, args)


if __name__ == "__main__":
    main()
