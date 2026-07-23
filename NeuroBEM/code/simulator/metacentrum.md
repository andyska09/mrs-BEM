# Running CMA-ES on MetaCentrum

Batch-submits the C++ `cmaes` tool to the PBS cluster. The binary is **built
inside each job** on the compute node (`-march=native` + heterogeneous nodes).

## 1. Log in

```bash
ssh YOUR_USERNAME@perian.metacentrum.cz
```
Frontends are for editing/submitting only — never compute on them.

## 2. First-time setup

Clone the repo (private → needs an SSH deploy key on the repo) and rsync the
gitignored dataset:

```bash
git clone git@github.com:andyska09/mrs-BEM.git
rsync -av LOCAL/NeuroBEM/CMAES-dataset/subset_20k.csv \
  YOUR_USERNAME@perian.metacentrum.cz:~/mrs-BEM/NeuroBEM/CMAES-dataset/
```

## 3. Submit

```bash
cd ~/mrs-BEM/NeuroBEM/code/simulator
python3 metacentrum/submit.py --dry-run                    # preview qsub
python3 metacentrum/submit.py --ncpus 32 --walltime 2:00:00  # full 19-param free, loss=both
python3 metacentrum/submit.py --mask 111 --loss force      # cl,cd,k force-only
```
Each `(mask, loss)` pair is one qsub. `cmaes` uses all `--ncpus` cores.

## 4. Monitor

```bash
qstat -u YOUR_USERNAME
```
The `.OU` log lands in `logs/` when the job finishes (PBS buffers it until then).

## 5. Collect results

Each run writes `~/mrs-BEM/NeuroBEM/CMAES-results/<timestamp>/` (`best.yaml`,
`convergence.csv`, `coeff.txt`, `metrics.csv`). Pull them down:

```bash
rsync -av YOUR_USERNAME@perian.metacentrum.cz:~/mrs-BEM/NeuroBEM/CMAES-results/ \
  LOCAL/NeuroBEM/CMAES-results-meta/
```

## Notes

- Build modules: `cmake gcc gsl eigen`. **Eigen is required** — the repo has no
  cmake/eigen.cmake fallback.
- `$HOME` differs per storage/node. Inside a job use absolute paths; `submit.py`
  passes them (`SIMDIR`/`DATA`), so this is handled. On a compute node, `cd
  $PBS_O_WORKDIR` to reach the submit dir.
- Request `ncpus=ompthreads=N` (submit.py does). Don't pass a thread count to
  `cmaes` — it auto-detects the allocated cores.
