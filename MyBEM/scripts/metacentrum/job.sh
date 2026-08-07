#!/bin/bash
# PBS job for one MyBEM stage. Submit with submit.py, which sets every resource
# on the qsub line plus STAGE, ROOT, NCPUS and, per stage:
#   apply  CONFIG DATA
#   tune   MODEL DRONE DATA FREE LOSS GENS SEED RUN   (FREE uses '+' for ',')
#   train  EXP SEED
# Results go straight into $ROOT/store; scratch is only used for the build.
# The C++ binaries take their thread count from OMP_NUM_THREADS, which PBS sets
# from ompthreads.
set -euo pipefail

echo "=== $PBS_JOBID  $STAGE  $(hostname -f)  $NCPUS cores  $(date) ==="

# -march=native on heterogeneous nodes: the binary has to be built in the job.
build() {
    trap 'clean_scratch' TERM EXIT
    module add cmake gcc gsl eigen
    cp -r "$ROOT/cpp" "$SCRATCHDIR"/
    rm -rf "$SCRATCHDIR/cpp/build"
    cmake -S "$SCRATCHDIR/cpp" -B "$SCRATCHDIR/build" >/dev/null
    cmake --build "$SCRATCHDIR/build" --target "$1" -j "$NCPUS"
}

case "$STAGE" in
apply)
    build mybem-apply
    "$SCRATCHDIR/build/mybem-apply" "$ROOT/$CONFIG" "$DATA" "$ROOT/store/preds"
    ;;
tune)
    build mybem-tune
    "$SCRATCHDIR/build/mybem-tune" "$ROOT/configs/models/$MODEL" "$DATA" \
        --drone "$ROOT/configs/drones/$DRONE" \
        --free "${FREE//+/,}" --loss "$LOSS" --gens "$GENS" --seed "$SEED" \
        --out "$ROOT/store/tune/$RUN"
    ;;
train)
    cd "$ROOT"
    conda activate "$PBS_O_HOME/.conda/envs/mybem"
    nvidia-smi -L
    python -m mybem.train "configs/experiments/$EXP" --seed "$SEED" --device cuda
    ;;
esac

echo "=== done $(date) ==="
