#!/bin/bash
# PBS job: build the `cmaes` tool on the compute node (‑march=native, so build in
# the job — MetaCentrum nodes are heterogeneous) and run one CMA‑ES fit.
# Submit via submit.py, which sets these ‑v env vars:
#   SIMDIR  - absolute path to code/simulator on shared home
#   DATA    - absolute path to the flat data CSV (e.g. .../CMAES-dataset/subset_20k.csv)
#   OUTDIR  - where to copy CMAES-results back to
#   MASK    - binary registry mask (e.g. 19 ones = all tunable params free)
#   LOSS    - force | torque | both

#PBS -N cmaes
#PBS -l select=1:ncpus=12:mem=8gb:scratch_local=4gb
#PBS -l walltime=4:00:00
#PBS -j oe

set -euo pipefail
trap 'clean_scratch' TERM EXIT

echo "============================="
echo "Job:  $PBS_JOBID   Node: $(hostname -f)"
echo "Mask: $MASK   Loss: $LOSS"
echo "Started: $(date)"
echo "============================="

# eigen is required: the repo has no cmake/eigen.cmake download fallback, so
# find_package(Eigen3) must succeed via the module.
module add cmake gcc gsl eigen

cp -r "$SIMDIR"/{include,src,CMakeLists.txt,bem_config.yaml} "$SCRATCHDIR"/
mkdir -p "$SCRATCHDIR/CMAES-dataset"
cp "$DATA" "$SCRATCHDIR/CMAES-dataset/data.csv"

cd "$SCRATCHDIR"
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release >/dev/null
make cmaes -j"$NCPUS"

# Core count is passed explicitly by submit.py (NCPUS); never use `nproc` here —
# coreutils nproc honors the PBS-injected OMP_NUM_THREADS=1 and returns 1.
export OMP_NUM_THREADS="$NCPUS"
echo "--- running cmaes on $NCPUS cores ---"
./cmaes "$SCRATCHDIR/CMAES-dataset/data.csv" --cma "$MASK" --loss "$LOSS" --threads "$NCPUS"

mkdir -p "$OUTDIR"
cp -r "$SCRATCHDIR/CMAES-results/"* "$OUTDIR"/

echo "============================="
echo "Results -> $OUTDIR"
echo "Done: $(date)"
echo "============================="
