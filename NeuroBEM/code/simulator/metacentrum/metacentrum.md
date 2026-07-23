# Running CMA-ES on MetaCentrum

Runs the C++ `cmaes` aero re-identification tool on the PBS cluster. Unlike a
Python job, the binary is **built inside the job** on the compute node, because
CMakeLists uses `-march=native` and MetaCentrum nodes are heterogeneous.

## 1. Log in

```bash
ssh YOUR_USERNAME@perian.metacentrum.cz
```

Other frontends: `skirit.ics.muni.cz`, `zuphux.cerit-sc.cz`, `elmo.metacentrum.cz`.
Frontends are for editing/submitting only — never compute on them.

## 2. First-time setup

Ship the simulator source + the (gitignored) dataset to your home. From local:

```bash
rsync -av --exclude build NeuroBEM/code/simulator YOUR_USERNAME@perian.metacentrum.cz:~/cmaes/code/
rsync -av NeuroBEM/CMAES-dataset/subset_20k.csv YOUR_USERNAME@perian.metacentrum.cz:~/cmaes/CMAES-dataset/
```

Home is NFS-mounted on compute nodes, so the job reads source/CSV directly and
builds in fast local `$SCRATCHDIR`.

## 3. Submit

```bash
cd ~/cmaes/code/simulator
python3 metacentrum/submit.py --dry-run                 # preview qsub
python3 metacentrum/submit.py                            # full 19-param free, loss=both
python3 metacentrum/submit.py --mask 111 --loss force    # cl,cd,k force-only
python3 metacentrum/submit.py --mask 111 11111111111111111111 --loss force both  # sweep (one job per pair)
```

Defaults: 12 CPUs, 8 GB, 4 h walltime. `cmaes` runs one OpenMP thread per
allocated core (it does **not** take `--threads`; see the note in `job.sh`).

## 4. Monitor

```bash
qstat -u YOUR_USERNAME
tail -f logs/cmaes.o*        # job stdout (build + convergence)
```

## 5. Collect results

Each run writes `~/cmaes/CMAES-results/<timestamp>/` with `best.yaml`,
`convergence.csv`, `coeff.txt`, `metrics.csv`. Pull them down:

```bash
rsync -av --ignore-existing YOUR_USERNAME@perian.metacentrum.cz:~/cmaes/CMAES-results/ ./NeuroBEM/CMAES-results/
```

## Notes

- `--cma MASK`: binary, one bit per REGISTRY entry (19 tunable + 2 fixed tail). 19 ones = all tunable free.
- `--loss force|torque|both` selects which residual terms enter the objective.
- Build needs modules `cmake gcc gsl eigen` (job.sh loads them). Eigen is
  **required** — the repo has no cmake/eigen.cmake fallback, so find_package(Eigen3)
  must resolve via the module. If a name fails, locate it with `module avail eigen`.
- MetaCentrum `$HOME` differs per storage/node — job.sh uses absolute paths
  (`$SIMDIR`/`$DATA` from submit.py), so this is handled; but never rely on `~` in a job.
