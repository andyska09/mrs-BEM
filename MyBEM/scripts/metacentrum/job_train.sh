#!/bin/bash
# PBS job: train one network. No build and no scratch — the venv and the data
# live on shared home, and mybem.train writes straight into store/nets/.
# submit_train.py sets: ROOT VENV EXP SEED

#PBS -N mybem-train
#PBS -l select=1:ncpus=2:mem=8gb
#PBS -l walltime=8:00:00
#PBS -j oe

set -euo pipefail

echo "============================="
echo "Job:  $PBS_JOBID   Node: $(hostname -f)"
echo "Exp:  $EXP   Seed: $SEED"
echo "Started: $(date)"
echo "============================="

source "$VENV/bin/activate"
cd "$ROOT"
python -m mybem.train "configs/experiments/$EXP" --seed "$SEED"

echo "============================="
echo "Done: $(date)"
echo "============================="
