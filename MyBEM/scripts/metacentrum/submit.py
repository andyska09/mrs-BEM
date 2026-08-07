#!/usr/bin/env python3
"""Submit MyBEM apply/tune/train jobs to MetaCentrum PBS. See metacentrum.md."""

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JOB = Path(__file__).resolve().parent / "job.sh"
DATA = ROOT.parent / "data"


def jobs(a):
    """(job name, stage-specific env) pairs — one qsub each."""
    if a.stage == "apply":
        for c in a.config:
            yield f"apply_{Path(c).stem}", {"CONFIG": c, "DATA": a.data}
    elif a.stage == "tune":
        for free in a.free:
            for loss in a.loss:
                key = "|".join(map(str, [a.model, a.drone, a.data.name, free,
                                         loss, a.gens, a.seed]))
                n = "all" if free == "all" else f"{len(free.split(','))}p"
                run = f"{loss}_{n}@{hashlib.sha256(key.encode()).hexdigest()[:6]}"
                yield f"tune_{run.replace('@', '_')}", {
                    "MODEL": a.model, "DRONE": a.drone, "DATA": a.data,
                    "FREE": free.replace(",", "+"),  # qsub -v is comma-separated
                    "LOSS": loss, "GENS": a.gens, "SEED": a.seed, "RUN": run,
                }
    else:
        for e in a.exp:
            for s in a.seeds:
                yield f"train_{Path(e).stem}_s{s}", {"EXP": e, "SEED": s}


def select(a):
    if a.stage == "train":
        return f"select=1:ncpus={a.ncpus}:ngpus=1:gpu_mem={a.gpu_mem}:mem={a.mem}"
    return (f"select=1:ncpus={a.ncpus}:ompthreads={a.ncpus}"
            f":mem={a.mem}:scratch_local=4gb")


def submit(a, name, env):
    env = {"STAGE": a.stage, "ROOT": ROOT, "NCPUS": a.ncpus, **env}
    cmd = ["qsub", "-N", name]
    if a.stage == "train":
        cmd += ["-q", "gpu"]
    cmd += ["-l", select(a), "-l", f"walltime={a.walltime}",
            "-j", "oe", "-o", f"{ROOT}/logs/",
            "-v", ",".join(f"{k}={v}" for k, v in env.items()), str(JOB)]
    if a.dry_run:
        print("DRY RUN:", " ".join(cmd))
        return
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        print(f"  ERROR: {r.stderr.strip()}", file=sys.stderr)
    else:
        print(f"  Submitted: {r.stdout.strip()}")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="stage", required=True)

    def common(sp, ncpus, mem, walltime):
        sp.add_argument("--ncpus", type=int, default=ncpus)
        sp.add_argument("--mem", default=mem)
        sp.add_argument("--walltime", default=walltime)
        sp.add_argument("--dry-run", action="store_true", dest="dry_run")

    ap = sub.add_parser("apply", help="run one model over every segment")
    ap.add_argument("--config", nargs="+", default=["configs/models/bem_default.yaml"],
                    help="model yaml(s) relative to MyBEM/; one job each")
    ap.add_argument("--data", type=Path, default=DATA / "processed_data")
    common(ap, 32, "8gb", "2:00:00")

    tp = sub.add_parser("tune", help="CMA-ES fit")
    tp.add_argument("--free", nargs="+", default=["all"],
                    help="tunable names, comma-separated; one job each")
    tp.add_argument("--loss", nargs="+", default=["both"],
                    choices=["force", "torque", "both"])
    tp.add_argument("--model", default="bem_default.yaml", help="under configs/models/")
    tp.add_argument("--drone", default="paper_quad.yaml", help="under configs/drones/")
    tp.add_argument("--data", type=Path, default=DATA / "CMAES-subsets" / "subset_20k.csv")
    tp.add_argument("--gens", type=int, default=100)
    tp.add_argument("--seed", type=int, default=0)
    common(tp, 32, "8gb", "4:00:00")

    rp = sub.add_parser("train", help="train a network on one GPU")
    rp.add_argument("--exp", nargs="+", default=["tcn_baseline.yaml"],
                    help="experiment yaml(s) under configs/experiments/")
    rp.add_argument("--seeds", nargs="+", type=int, default=[0])
    rp.add_argument("--gpu-mem", default="10gb", dest="gpu_mem")
    common(rp, 4, "16gb", "8:00:00")

    a = p.parse_args()

    if a.stage == "apply":
        need = [a.data, *(ROOT / c for c in a.config)]
    elif a.stage == "tune":
        need = [a.data, ROOT / "configs/models" / a.model,
                ROOT / "configs/drones" / a.drone]
    else:
        need = [ROOT / "configs/experiments" / e for e in a.exp]
    for x in need:
        if not x.exists():
            sys.exit(f"not found: {x}")
    (ROOT / "logs").mkdir(exist_ok=True)

    todo = list(jobs(a))
    print(f"Submitting {len(todo)} {a.stage} job(s)...")
    for name, env in todo:
        print(f"\n[{name}]")
        submit(a, name, env)


if __name__ == "__main__":
    main()
