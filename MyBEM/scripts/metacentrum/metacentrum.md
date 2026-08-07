# Running MyBEM on MetaCentrum

`submit.py` builds one `qsub` per job, `job.sh` runs it. Three stages:

| stage | resources | parallelism |
|---|---|---|
| `apply` | 32 CPUs, 8 GB, 2 h | `mybem-apply` runs 32 segments at once |
| `tune` | 32 CPUs, 8 GB, 4 h | `mybem-tune` splits the CMA-ES fitness across samples |
| `train` | 4 CPUs, 1 GPU (10 GB), 16 GB, 8 h, queue `gpu` | `--device cuda` |

`apply`/`tune` build the C++ binary inside the job in `$SCRATCHDIR`, because the
frontends and the compute nodes run different Debian releases. **`-march=native`
is off** — with gcc 9.2 and Eigen 3.3.7 on the znver2 nodes it corrupts the heap
and `mybem-tune` aborts with `double free or corruption (out)`; it was worth ~10%.
Everything else reads and writes shared home directly, so
there is no copy-back step and `apply` resumes — it skips segments that already
have an output. Both binaries take their thread count from `OMP_NUM_THREADS`,
which PBS sets from `ompthreads`.

## 1. Log in

```
ssh YOUR_USERNAME@perian.metacentrum.cz
```
Frontends are for editing and submitting only — never compute on them.

## 2. First-time setup

```
git clone git@github.com:andyska09/mrs-BEM.git
```
```
rsync -av LOCAL/data/ YOUR_USERNAME@perian.metacentrum.cz:~/mrs-BEM/data/
```
```
conda env create -f ~/mrs-BEM/MyBEM/environment.yml
```

`data/` is gitignored: `processed_data/` is 1.3 GB (247 segments), `CMAES-subsets/`
8.6 MB. `submit.py` finds both relative to the repo root.

## 3. Submit

Run from `~/mrs-BEM/MyBEM`.

```
python3 scripts/metacentrum/submit.py apply --dry-run
```
```
python3 scripts/metacentrum/submit.py apply --config configs/models/bem_default.yaml
```
```
python3 scripts/metacentrum/submit.py tune --free all --loss both
```
```
python3 scripts/metacentrum/submit.py tune --free lift_coefficient,drag_coefficient,hinge_spring_constant --loss force torque
```
```
python3 scripts/metacentrum/submit.py train --exp tcn_baseline.yaml --seeds 0 1 2
```

Every list argument multiplies out into separate jobs: `--free A B --loss force both`
is four. Resources are overridable per submit with `--ncpus --mem --walltime`
(`--gpu-mem` for `train`).

## 4. Monitor

```
qstat -u YOUR_USERNAME
```
Logs land in `MyBEM/logs/` when the job finishes (PBS buffers them until then).
A `train` job prints `nvidia-smi -L` at the top so you can see which card it got.

## 5. Collect

Results are already on shared home — `MyBEM/store/{preds,tune,nets}/`. Pull them down:

```
rsync -av YOUR_USERNAME@perian.metacentrum.cz:~/mrs-BEM/MyBEM/store/ LOCAL/MyBEM/store/
```

## Notes

- Build modules: `cmake gcc gsl eigen`. GSL and Eigen are both required — CMake has
  no download fallback for either.
- GPU jobs need `-q gpu` *and* `ngpus=1` (`submit.py` adds both). The pip `torch`
  wheel ships its own CUDA runtime, so no `cuda` module is needed.
- `$HOME` differs per storage node. `submit.py` passes absolute paths in `-v`.
- `-v` is comma-separated, so `--free cl,cd,k` crosses as `cl+cd+k` and `job.sh`
  swaps it back.
