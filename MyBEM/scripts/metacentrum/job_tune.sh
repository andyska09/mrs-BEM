#!/bin/bash
# PBS job: build mybem-tune on the compute node (-march=native, and MetaCentrum
# nodes are heterogeneous) and run one CMA-ES fit.
# submit_tune.py sets: ROOT DATA OUTDIR MODEL DRONE FREE LOSS GENS SEED RUN NCPUS
# FREE arrives with '+' instead of ',' because qsub -v is comma-separated.

# submit_tune.py always overrides -l on the qsub command line; these are fallback only.
#PBS -N mybem-tune
#PBS -l select=1:ncpus=12:mem=8gb:scratch_local=4gb
#PBS -l walltime=4:00:00
#PBS -j oe

set -euo pipefail
trap 'clean_scratch' TERM EXIT

echo "============================="
echo "Job:  $PBS_JOBID   Node: $(hostname -f)"
echo "Free: $FREE   Loss: $LOSS   Gens: $GENS   Seed: $SEED"
echo "Started: $(date)"
echo "============================="

# eigen is required: CMakeLists has no download fallback, find_package must succeed.
module add cmake gcc gsl eigen

cp -r "$ROOT/cpp" "$ROOT/configs" "$SCRATCHDIR"/
cp "$DATA" "$SCRATCHDIR/data.csv"
rm -rf "$SCRATCHDIR/cpp/build"

cd "$SCRATCHDIR"
cmake -S cpp -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build --target mybem-tune -j "$NCPUS"

# No --threads: mybem-tune uses all allocated cores (omp_get_num_procs).
echo "--- running mybem-tune on $NCPUS cores ---"
./build/mybem-tune "configs/models/$MODEL" data.csv \
    --drone "configs/drones/$DRONE" \
    --free "${FREE//+/,}" --loss "$LOSS" --gens "$GENS" --seed "$SEED" \
    --out "$SCRATCHDIR/$RUN"

mkdir -p "$OUTDIR"
cp -r "$SCRATCHDIR/$RUN" "$OUTDIR"/

echo "============================="
echo "Results -> $OUTDIR/$RUN"
echo "Done: $(date)"
echo "============================="
