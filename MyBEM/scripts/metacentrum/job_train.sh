#!/bin/bash
# PBS job: train one network. No build and no scratch — the env and the data live
# on shared home, and mybem.train writes straight into store/nets/.
# submit_train.py sets: ROOT EXP SEED

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

cd "$ROOT"
conda activate "$PBS_O_HOME/.conda/envs/mybem"

python -m mybem.train "configs/experiments/$EXP" --seed "$SEED"

echo "============================="
echo "Done: $(date)"
echo "============================="
