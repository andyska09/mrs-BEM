# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## IMPORTANT NOTE TO ALL AI CHATBOTS

When talking to me start the message with "TARS:". When writing answer always keep it concise and information dense. But do not skip important stuff. When I tell you to fuck off or shout and curse at you DO NOT apologize, it wastes tokens, just follow orders. Next when writing code DO NOT write stupid comments and docstrings. Keep the code clean and high quality. When implementing stuff KEEP IT SIMPLE. This important. I do not want to read 1000 lines of diffs, I want to look at the change and know what it does. As one artist said "One good girl is worth a thousand bitches.".

Also IDE is wrongly configured, so it will report wrong include errors etc. So do not worry about it. Only investigate when neccessary. 

When listing commands to me, one command one line. If there is more than one command or it is super duper complex i will spank you. HARD.

## My goal, my rules and my story

This repo's goal is to recreate the NeuroBEM paper and port it into the **Agilicious** simulator for closed-loop evaluation. `NeuroBEM/analysis/` is my own workspace to play with and analyze the data and outputs of the BEM model. `agilicious/simulator/` is part of the simulator I received from my supervisor (from the same lab as the paper) that implements BEM inside — the deployment target.

From the MRS web page:

> **Learned Hybrid Aerodynamic Modeling for Agile Multirotor Flight**
> While control, planning, and perception have already reached significant milestones in UAV research, modeling aerodynamics remains one of the most open-ended problems in the field. This is caused by the complex phenomena that emerge during agile motion of multirotor UAVs, where each propeller blade is itself a wing interacting with all others. Classical modeling and gray-box approaches quickly fall short, and for agile flight of larger drones, this becomes a bottleneck that must be addressed. This project focuses on hybrid aerodynamic modeling that combines first-principles theory with machine learning.
> [1] M. O'Connell et al. "Neural-Fly enables rapid learning for agile flight in strong winds." Science Robotics, 2022.
> [2] L. Bauersfeld et al. "NeuroBEM: Hybrid Aerodynamic Quadrotor Model." RSS, 2021.
>
> Advisor: Michal Pliska (pliskmic@fel.cvut.cz)

### Current direction ([notes/current_goal.md](notes/current_goal.md))

Advisor's latest steer (Michal Pliska): the NN I can train myself once things run — the point is to **(1) get the same results as the paper, (2) try CMA-ES for parameter tuning to see what it's capable of finding, and (3) get the whole pipeline working.** Our main task afterwards is to **apply it to the Eagle drone in a closed-loop simulation.**

So the active objective: **port NeuroBEM into the Agilicious simulator and reproduce the paper's closed-loop tracking (Table III) entirely in sim.** Agilicious already ships the BEM rotor model; the work is to port the NN residual, wire it into the sim loop, and build the evaluation pipeline. No mocap access — all data and ground truth come from the public NeuroBEM dataset; we never record our own flights. The full spec is [notes/current_goal.md](notes/current_goal.md); the 7-step onboarding recipe for a new drone is [notes/pipeline_description.md](notes/pipeline_description.md).

- **NN residual (fixed interface):** input `20×10` (3 body linvel, 3 body rates, 4 motor speeds) @ 2.5 ms = 50 ms history; output 6 = residual force+torque added to BEM. Two heads (force/torque), strictly causal, **bounded output** (an unbounded torque residual crashed the paper's ablation), exported to C++ (ONNX Runtime / libtorch) at 1 kHz with the training normalization constants shipped alongside.
- **Sim loop (per 1 ms tick):** low-level controller → first-order motor model → BEM+NN → rigid-body integrate. Use **symplectic Euler @ 1 ms** (Agilicious defaults to RK4 — switch it) and a BetaFlight-style rate loop, or comparisons to the paper are invalid.
- **Evaluation ladder (in order):** (1) single-step RMSE on the held-out split (Table I target ≈ 0.352 N / 5.3e-3 Nm), (2) open-loop motor-replay rollout (position drift over ~1 s windows), (3) closed-loop MPC flying the reference trajectories vs. the recorded flights (Table III). Model selection uses trajectory drift, not per-step RMSE.
- **CMA-ES tuning:** re-fitting the BEM aero params `(cl, cd, kβ)` from flight data (see "re-identification" below) is now an explicit advisor ask — evaluate what CMA-ES can recover before touching the NN. The `processed_data/` `bem` / `2026-07-15-00-00-00` / `2026-07-21-14-54-19` / `bem-vi-baseline` / `bem-007` / `bem-agi` subfolders are outputs of different tunes (the two timestamped ones — formerly `bem-01`/`bem-02` — are named for their CMA-ES run) ([notes/results.md](notes/results.md)).
- **Out of scope until the ladder reproduces the paper:** architecture swaps, multi-step/rollout training, BEM-solver improvements.

## Repository layout

This repo is a workspace of four loosely-coupled parts:

- **[NeuroBEM/](NeuroBEM/)** — the main working tree: the NeuroBEM framework ([NeuroBEM/code/](NeuroBEM/code/)), my exploratory analysis ([NeuroBEM/analysis/](NeuroBEM/analysis/)), dataset docs ([NeuroBEM/README.md](NeuroBEM/README.md), [NeuroBEM/Flights.txt](NeuroBEM/Flights.txt), [NeuroBEM/testset.txt](NeuroBEM/testset.txt)), and gitignored flight data/outputs (`processed_data/`, `raw_data/`, `pdf/`, `bem+nn/`). [NeuroBEM/code/README.md](NeuroBEM/code/README.md) is the authoritative end-to-end tutorial for the *pipeline*.
- **[agilicious/simulator/](agilicious/simulator/)** — extracted Agilicious C++ simulator sources (models, low-level controllers, `bem/`), the **deployment target** for the final closed-loop sim (formerly referred to as `my_bem/`). Reference source only — no build system or headers checked in here.
- **[notes/](notes/)** — living project notes: `current_goal.md` (objective + fixed NN interface + eval ladder), `pipeline_description.md` (7-step recipe for onboarding a new drone), `bem_parameters.md` (full inventory of every C++ BEM param, where it acts, whether tuning it does anything — groundwork for full-param CMA-ES; includes the canonical **REGISTRY bit ↔ code-variable** table that decodes `--cma MASK`), `bem_limitations.md` (research notes challenging the paper's BEM assumptions & generalization claim), `data_sources.md` (per-column provenance of the 29-col merged CSV), `results.md` + `cmaes_results.md` (BEM tuning / CMA-ES fit results in paper Table II format, and what each `processed_data/bem-*` folder contains).
- **[papers/](papers/)** — [papers/RSS21_Bauersfeld.pdf](papers/RSS21_Bauersfeld.pdf) (NeuroBEM) with grep-able transcription [papers/RSS21_Bauersfeld.md](papers/RSS21_Bauersfeld.md), and `scirobotics.abm6597.pdf` (Neural-Fly). The `.md` is the source of truth for *why* the code is shaped as it is.

### Root-level dataset docs (under `NeuroBEM/`)

- [NeuroBEM/README.md](NeuroBEM/README.md) — dataset-level README: folder layout and exact CSV column orders (29 cols for processed data; +12 cols 30–41 for `predictions/` = predicted force/torque + residuals). Drone mass 0.772 kg, diagonal inertia [0.0025, 0.0021, 0.0043]. Its `predictions/` folder is the dataset name for the stage-2 output the pipeline docs call `MODEL/`/`bem+nn/`.
- [NeuroBEM/Flights.txt](NeuroBEM/Flights.txt) — catalog of all 95 flights as MATLAB `dataset = "<timestamp>"` lines, each commented with its trajectory type.
- [NeuroBEM/testset.txt](NeuroBEM/testset.txt) — 13 held-out `<ID>_seg_X` segments (dataset-level hold-out). The pipeline's own `testset.txt` consumed by `get_datafiles.bash` lives under [NeuroBEM/code/Python/data/](NeuroBEM/code/Python/data/).

## Architecture: the three-stage pipeline

Data flows through three loosely-coupled stages that communicate via CSV files on disk. There is no single orchestrator — each stage is run manually and writes files the next stage reads.

1. **BEM identification & physical model (MATLAB + C++)** — Identify propeller physical parameters from thrust-test-stand data, then encode them into a C++ simulator.
   - MATLAB scripts under [NeuroBEM/code/Matlab/BEM/](NeuroBEM/code/Matlab/BEM/) fit lift/drag coefficients and the hinge-spring constant (`ParameterID.m`, `QuadraticFit.m`, `Coning.m`). Geometry/identified params are hand-edited into `Matlab/BEM/subroutines/setParam.m`.
   - The flapping/coning equations are derived symbolically in `Maple/BEM_Derivation.mw` and the generated formulae are implemented in both MATLAB subroutines and the C++ simulator (`calc_a0`/`calc_a1s`/`calc_b1s` ↔ `calculateConing.cpp` etc.). MATLAB is only a tool to produce values for C++; it is intentionally not optimized.

2. **Flight-data processing (MATLAB) → base-model application (C++)** — Merge sensor sources, then run the BEM over them to produce training data.
   - `Matlab/OptiTrack/MergeAndPreprocessData.m` merges Rosbag (OptiTrack pose) + Betaflight motor-speed logs + trajectory into `merged_*` and `merged_*_seg_X.csv` files. The `_seg_X` segment files (airborne portions) are the ones used downstream.
   - The C++ `bem-model` executable ([NeuroBEM/code/simulator/](NeuroBEM/code/simulator/)) reads each `merged_*_seg_*.csv`, predicts forces/torques, and writes `MODEL/MODEL_<flight>_seg_X.csv` (the NN training data = measured minus predicted). Run via `Scripts/applyBM.sh DATAFOLDER`. It also dumps the effective aero params as `params.yaml` next to the output file ([simulator.cpp:31](NeuroBEM/code/simulator/src/simulator/simulator.cpp#L31) → `Quadcopter::log_params` → `Propeller_s/Quadcopter_s::log_params` in [params.h:132](NeuroBEM/code/simulator/include/params.h#L132)), so the run records exactly which coefficients produced it.

3. **Neural network (Python/TensorFlow)** — Train a network on the residuals. Code under [NeuroBEM/code/Python/](NeuroBEM/code/Python/).

**Python env:** all Python (`NeuroBEM/analysis/` + `NeuroBEM/code/Python/`) runs in the `neurobem` conda env (py3.11, deps in [NeuroBEM/requirements.txt](NeuroBEM/requirements.txt)) — `conda run -n neurobem python ...`.

### Key cross-stage coupling to be aware of

- **`MODEL` is a compile-time choice.** The base model is selected by `#define MODEL` in [NeuroBEM/code/simulator/include/params.h](NeuroBEM/code/simulator/include/params.h): `1` = BEM, `0` = quadratic fit, `-1` = none. Changing it **requires rebuilding the simulator**, and the `model` variable in `Scripts/applyBM.sh` (and the `base_type` in the Python settings) must be changed to match. `MODEL` also drives the `CHORD`/`POLAR`/`DIST` sub-defines.
- **Identified parameters are duplicated by hand**, not shared via a file. Values from MATLAB land in `setParam.m`, in `params.h` for the simulator, and ultimately in agilicious `sim_*.yaml`/`.hpp` files. When a physical parameter changes, all copies must change.
- **Aero params are runtime-overridable (per instance).** The `params.h` `#define`s are the compile-time *defaults*, but `param` is a plain mutable instance member — `Propeller_s param = Propeller_s()` ([propeller.h:85](NeuroBEM/code/simulator/include/propeller.h#L85)), `Quadcopter_s param` ([quadcopter.h:47](NeuroBEM/code/simulator/include/quadcopter.h#L47)) — so any param can be changed at runtime without a rebuild. The plumbing: `Propeller_s::field(name)`/`Quadcopter_s::field(name)` ([params.h:112](NeuroBEM/code/simulator/include/params.h#L112), [params.h:150](NeuroBEM/code/simulator/include/params.h#L150)) return a `double*` to the named member (poor-man's reflection; unknown name → `nullptr`), and `Propeller::setParams`/`Quadcopter::setParams` ([propeller.cpp:148](NeuroBEM/code/simulator/src/simulator/propeller.cpp#L148), [quadcopter.cpp:29](NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L29)) write through them by name — `Propeller::setParams` also copies into `solver->p` (the GSL helper's `Propeller_s`) and invalidates caches; `Quadcopter::setParams` re-places motors and forwards leftover names to each `Motor`. This is what the `cmaes` tool drives (see re-identification below). Only structural/`#define`-gated choices (`MODEL`/`POLAR`/`CHORD`, which fields even exist) still need a rebuild.
- The Python `base_type` setting (`"bem"`/`"fit"`/`"none"`) selects which `MODEL`-output subfolder to train on.

### Python NN internals

- Entry points: `train.py` and `test.py`, both `--settings_file config/bem_settings.yaml`. `Learner` ([NeuroBEM/code/Python/learner.py](NeuroBEM/code/Python/learner.py)) owns the full TF training/eval loop; `config/settings.py` parses and validates the YAML (asserts data dirs exist, sets `CUDA_VISIBLE_DEVICES`, etc.).
- `utils/` holds the pieces wired together by the learner: `nets.py` (MLP / TCN / RNN architectures, selected by `network.architecture`), `loader.py` + `window_generator.py` (sliding-window dataset, `history_len`), `loss.py` (separate `ForceLoss`/`TorqueLoss` with per-axis weights), `normalization.py`, `visualization.py`.
- The network input feature set is configured by the `dataloading.use_*` flags in the YAML (linvel/angvel/motors etc.), so feature length depends on config and must stay consistent with the deployed ONNX/TensorRT shape (`H`=history_len, `FL`=feature length).
- Deployment path for agilicious is TensorRT: trained `.pb` → ONNX (`tf2onnx`) → serialized `.trt` engine, verified with [NeuroBEM/code/Python/trt/](NeuroBEM/code/Python/trt/) against `predict_from_pb.py` output. Engines are machine-specific and gitignored.

## Model background (from the NeuroBEM paper)

[papers/RSS21_Bauersfeld.pdf](papers/RSS21_Bauersfeld.pdf) (grep-able markdown at [papers/RSS21_Bauersfeld.md](papers/RSS21_Bauersfeld.md)) is the source of truth for *why* the code is shaped this way. The whole framework computes one equation:

```
f = f_prop + f_res        τ = τ_prop + τ_res
```

A **rotor model** (first principles) predicts `f_prop`/`τ_prop`; a **neural network** predicts the residuals `f_res`/`τ_res` (body/frame aero + rotor-to-rotor interactions the rotor model ignores). This split is exactly stages 1–2 (rotor model in C++) vs stage 3 (NN in Python) above.

- **The `MODEL` define maps to the paper's three rotor models:** `-1` None (predict zeros, a naive baseline), `0` Quadratic/"Fit" (thrust & torque ∝ Ω², coefficients from a static test stand — good only near hover), `1` BEM (the accurate model). Paper Table II/III compares None / Fit / BEM each ±NN; "BEM+NN" is the proposed method.
- **BEM internals** (the C++ `simulator/src/simulator/` files): blade-element-momentum theory needs the **induced velocity `vi`**, which has no closed form — it's solved numerically via GSL (`gslHelper.cpp` / `propeller.cpp::_calculateInducedVelocity`, the runtime-dominant step, ~100 µs). With `vi` known, the **coning angle `a0`** and **flapping angles `a1`/`b1`** are evaluated (`calculateConing.cpp`, `calculateLongitudinalFlapping.cpp`, `calculateLateralFlapping.cpp` — the long auto-generated expressions from the Maple worksheet). There is special handling for **vortex-ring state** (descending into own downwash) where momentum theory breaks down and an empirical quartic fit for `vi` is used.
- **Lift/drag polar:** `cl(α)=cl,0·sinα·cosα`, `cd(α)=cd,0·sin²α` (the `POLAR` setting; `param.cl`/`param.cd`). The coning/flapping use the separate *linear* coefficients (`param.a`/`param.d`) for tractability — this is why changing `param.cl`/`param.cd` doesn't move the angles. `kβ` is the hinge-spring stiffness; it appears directly in the final propeller torque `τ_P`.
- **The NN feature set is dictated by the paper:** inputs are linear velocity, angular velocity, and motor speeds over a history of **h = 20** samples at **δt = 2.5 ms** (= 50 ms of context). This is exactly `dataloading.history_len: 20` + `use_linvel/use_angvel/use_motors: True` in `bem_settings.yaml`. The paper's ablation (Table I) selected **TCN-medium**, hence `network.architecture: "TCN"` is the intended default; MLP/RNN exist for comparison.
- **Quadrotor & sim constants:** motor first-order dynamics with time constant **τΩ = 33 ms** (`params.h: tau = 0.033`); the closed-loop simulator integrates with a **symplectic Euler** scheme at 1 ms (chosen for energy conservation); platform mass ≈ 0.772 kg with diagonal inertia (the `quadrotor:` block in the YAML).
- **Data-processing rationale** (stage 2 MATLAB): Vicon pose at 400 Hz and onboard IMU + motor speeds at 1 kHz are asynchronous, so `MergeAndPreprocessData` fits **cubic splines** to fuse them and differentiates the splines to get low-noise linear velocity / angular acceleration. Time sync (offset + ~2.4% clock skew) is recovered by correlating gyro rates against the spline (the `align_data` subroutine); motor speeds get a 4th-order Butterworth low-pass. Full dataset: 96 flights / 1.8 M points, split 70/20/10 — matching the counts produced by `get_datafiles.bash`.

## Exploratory analysis (`NeuroBEM/analysis/`)

A separate Python workspace (formerly `EDA/`) for data-quality analysis and BEM-baseline evaluation. Unlike the pipeline, these scripts read the **committed-locally, gitignored** `NeuroBEM/processed_data/` directly, so they only run where that data is present.

- [NeuroBEM/analysis/utils.py](NeuroBEM/analysis/utils.py) — shared loaders, noise metrics, and the **canonical column layout**. `load_flight`/`load_largest_segment` read `processed_data/merged_*_seg_*.csv` (29-column merged order hard-coded in `COLUMNS`). `load_bem_flight`/`load_bem` read `processed_data/bem/bem_*_seg_*.csv`. The current `bem-model` writes **35 columns** (29 merged + 6 predicted `fx,fy,fz,tx,ty,tz` cols 29–34; `nCol + 6` in [simulator.cpp:22](NeuroBEM/code/simulator/src/simulator/simulator.cpp#L22)). The files in `processed_data/bem/` are **41 columns** = those 35 + 6 residuals (measured − predicted, cols 35–40) appended in a post-processing step (same append `make_nn_targets.py` does). `utils.py`'s `BEM_COLUMNS`/`VI`/`MU`/`AS` ([utils.py:32-36](NeuroBEM/analysis/utils.py#L32)) and `diagnostics()` assume the **47-col format** (35 above + 12 per-motor `vi,mu,alpha_s` diagnostics at cols 35–46, 0-indexed) — this is **not** what the default `bem-model` writes (35 cols), but it **is** exactly what `processed_data/bem-vi-baseline/` contains: those files were produced by the diagnostics-logging build (commit `d2fd23e`, `simulator.cpp` widened to `nCol + 18`; layout in `bem_output_columns.md`), and `utils.diagnostics`/`make_subset.py` read the `vi`/`mu` from them. So the `diagnostics` path is live for `bem-vi-baseline`, stale for `processed_data/bem/`. Regenerating `processed_data/bem/` with the default `bem-model` gives 35-col files; re-append the 6 residuals to get back to 41. `measured()`/`residuals()` compute force `= mass·acc` and torque `= I·ang_acc + ω×Iω` using the **simulator** mass/inertia. `FS = 400 Hz`. SNR uses a 4th-order **25 Hz Butterworth** low-pass.
- [NeuroBEM/analysis/measure_bem_RMSE.py](NeuroBEM/analysis/measure_bem_RMSE.py) — BEM baseline residual RMSE (measured − predicted) over the full set and the `testset.txt` hold-out. Run: `python3 analysis/measure_bem_RMSE.py` (defaults to `processed_data/bem` + root `testset.txt`; self-contained, hard-codes the simulator `mass = 0.752` / inertia).
- [NeuroBEM/make_nn_targets.py](NeuroBEM/make_nn_targets.py) — post-processes a `processed_data/bem-*` folder into NN residual targets: reads 35-col `bem_*.csv`, computes `residuals()` (force `= MASS·acc`, torque `= INERTIA·α + ω×INERTIA·ω` with `MASS = 0.772`, `INERTIA = [0.0025,0.0021,0.0043]` from `README.md`), writes `[first 35 cols, 6 residuals]`.
- **Re-identification / CMA-ES workflow (advisor ask; lives in the `cmaes` C++ tool).** The active re-identifier is `code/simulator/src/CMAES.cpp` → the **`cmaes`** executable (a second CMake target alongside `bem-model`). It reads one flat data CSV (the same 29+ col merged/bem layout, `load()` reads cols 1–23), builds a **fleet of `Quadcopter`s (one per OpenMP thread)** and parallelizes the fitness **across samples** ([CMAES.cpp:185](NeuroBEM/code/simulator/src/CMAES.cpp#L185); thread count is `--threads N`, default = `omp_get_num_procs()` = the cpuset's allocated cores — this deliberately **ignores a PBS-injected `OMP_NUM_THREADS=1`**, [CMAES.cpp:609](NeuroBEM/code/simulator/src/CMAES.cpp#L609); the run prints `openmp: N threads`), and via `Quadcopter::load` sweeps a **21-entry `REGISTRY`** ([CMAES.cpp:54](NeuroBEM/code/simulator/src/CMAES.cpp#L54)) of tunable params, each `{key, section, default, lo, hi}` (`section` = `bem`/`quad`/`body_drag`, used to group the emitted YAML), **ordered by optimization priority** — `lift_coefficient, drag_coefficient, hinge_spring_constant, lift_offset, hforce_scale, thrust_scale, pitch, twist, chord_inner, chord_outer, horizontal_drag_coefficient, vertical_drag_coefficient, radius, dx, dy, dz, frontarea_x/y/z`, then a **load-only tail** `num_blades, air_density` (kept at default). `--cma MASK` frees the entries whose bit is `1` in the binary `MASK` (one bit per REGISTRY entry, left-aligned; e.g. `111` = `cl,cd,k`, trailing/unset bits stay fixed at default). The objective is **residual MSE normalized by the baseline (defaults) residual MSE** — `fm.sum()/sf² + tm.sum()/st²` with `sf,st` = sqrt of the defaults' force/torque MSE ([CMAES.cpp:606](NeuroBEM/code/simulator/src/CMAES.cpp#L606)) — and **`--loss force|torque|both`** selects which terms are included (default `force`; replaces the old `--joint` flag; 100 gens, `MAXGEN` constant). Config is **YAML in and out**: `./cmaes data.csv` (report at defaults), `./cmaes data.csv config.yaml` (report at values loaded via `loadConfig` from [config.h](NeuroBEM/code/simulator/include/config.h)), `./cmaes data.csv --cma MASK [--loss force|torque|both]` (run the fit; prints baseline → CMA-ES → best, writes a per-run folder `CMAES-results/<ts>/` containing `best.yaml` (sectioned by `bem`/`quad`/`body_drag`), `convergence.csv` (trace), `coeff.txt` (mask line + objective line `force-only`/`torque-only`/`force+torque` + one freed-param name per line), and `metrics.csv` (**best** config's reported RMSE only: header `Fxy,Fz,F,Mxy,Mz,M` + one row)). The flat CSV `cmaes` consumes is built by [NeuroBEM/CMAES-dataset/make_subset.py](NeuroBEM/CMAES-dataset/make_subset.py) → `CMAES-dataset/subset_20k.csv`: pools all non-test flights from `processed_data/bem-vi-baseline/` (47-col), bins rows on a 6×6 quantile grid of per-row mean `(mu, vi)` (`utils.diagnostics`), and randomly samples ~equally per cell (`SEED=0`, `N_TARGET=20000`). The `processed_data/` `bem` / `2026-07-15-00-00-00` / `2026-07-21-14-54-19` / `bem-vi-baseline` / `bem-007` / `bem-agi` subfolders are outputs of different tunes (the two timestamped ones — formerly `bem-01`/`bem-02` — are named for their CMA-ES run) (see [notes/results.md](notes/results.md) for the exact cl/cd/k and corrections each uses).
- Notebooks: [NeuroBEM/analysis/feature_analyis.ipynb](NeuroBEM/analysis/feature_analyis.ipynb), [NeuroBEM/analysis/pred_analysis.ipynb](NeuroBEM/analysis/pred_analysis.ipynb), [NeuroBEM/analysis/vi_analysis.ipynb](NeuroBEM/analysis/vi_analysis.ipynb).
- **[NeuroBEM/CMAES-results-analysis/](NeuroBEM/CMAES-results-analysis/)** — cross-run comparison of every `CMAES-results/<ts>/` fit. `results_analysis.ipynb` loads each run's `best.yaml`/`coeff.txt`/`metrics.csv`, builds a coefficient table (freed params highlighted, defaults from `processed_data/bem/params.yaml`), a metrics table vs the **paper Table II** BEM baseline, and convergence-trace plots. `agi_coeffs.yaml` is the agilicious-specific coefficient set included as an extra column for comparison.

## Common commands

Python runs in the `neurobem` conda env: `conda run -n neurobem python ...`.

### Build the C++ simulator
Requires GSL and Eigen (`sudo apt-get install libgsl-dev libeigen3-dev`). The [NeuroBEM/build/](NeuroBEM/build/) directory is a pre-existing CMake configuration of the simulator (the VS Code CMake extension points at `NeuroBEM/code/simulator` via `.vscode/settings.json`). To build from scratch:
```
cd NeuroBEM/code/simulator
mkdir build && cd build
cmake ..
make            # produces the `bem-model` and `cmaes` executables
```

### Re-identify aero params with CMA-ES
```
cd NeuroBEM/code/simulator/build
./cmaes DATA.csv                                  # report force/torque RMSE at defaults
./cmaes DATA.csv config.yaml                      # report RMSE at params from a YAML file
./cmaes DATA.csv --cma 111 --loss both            # fit cl,cd,k (mask bits 0-2) on force+torque → best.yaml
./cmaes DATA.csv --cma 111 --loss both --threads 8  # cap OpenMP threads (default: omp_get_num_procs)
```
CMake options (default ON): `ENABLE_FAST`, `UNSAFE_MATH`, `ENABLE_PARALLEL` (OpenMP), `EIGEN_FROM_SYSTEM`. C++17, `-march=native`. Two executables share the `simulator` library: **`bem-model`** (`src/simulator_node.cpp`, pipeline stage 2) and **`cmaes`** (`src/CMAES.cpp`, CMA-ES aero re-identification — see re-identification note).

### Run CMA-ES on the MetaCentrum PBS cluster
The `cmaes` tool can be batch-submitted to MetaCentrum via [NeuroBEM/code/simulator/metacentrum/submit.py](NeuroBEM/code/simulator/metacentrum/submit.py) + `job.sh`; full walkthrough in [NeuroBEM/code/simulator/metacentrum/metacentrum.md](NeuroBEM/code/simulator/metacentrum/metacentrum.md). The binary is **built inside the job** on the compute node (because `-march=native` + heterogeneous nodes), so the job first `module add cmake gcc gsl eigen` — Eigen is required (no cmake download fallback). One `(mask, loss)` pair → one `qsub` (jobs run in parallel).
```
python3 metacentrum/submit.py --dry-run                       # preview qsub, submit nothing
python3 metacentrum/submit.py                                 # full 19-param free, loss=both, 12 CPUs
python3 metacentrum/submit.py --mask 111 --loss force         # cl,cd,k force-only
```
`submit.py` requests `ncpus=ompthreads=N` and passes `NCPUS` into the job; `job.sh` uses it for `make -j"$NCPUS"` and exports `OMP_NUM_THREADS="$NCPUS"` but **deliberately does not pass `--threads`** to `cmaes` — the binary defaults to `omp_get_num_procs()` (the allocated cores), which is what dodges the PBS `OMP_NUM_THREADS=1` override that caused the earlier nproc/stale-file bugs. `job.sh` copies each `CMAES-results/<ts>/` back to `$OUTDIR`. Ship source + `subset_20k.csv` to `~/cmaes/` first (rsync commands in `metacentrum.md`).

### Apply the base model to flight data
```
cd NeuroBEM/code/Scripts
./applyBM.sh ../ExampleData/OptiTrack/        # or pass a filelist as 2nd arg
```

### Prepare the NN dataset (train/val/test split)
```
cd NeuroBEM/code/Python/data
./get_datafiles.bash ../../ExampleData/OptiTrack/ bem
```
Splits are copied into `data/bem/{train,validation,test}`. `testset.txt` defines a manual hold-out set.

### Train / monitor / test the network
```
cd NeuroBEM/code/Python
conda run -n neurobem python train.py --settings_file config/bem_settings.yaml
tensorboard --logdir=train_logs            # checkpoints + best_model under train_logs/TIMESTAMP/
conda run -n neurobem python predict_from_pb.py --log_folder train_logs/TIMESTAMP/    # sanity-check (ones input)
conda run -n neurobem python generate_ablation_study.py --load_folder train_logs/TIMESTAMP --data_root data/bem/test --output_dir .
```

### BEM baseline RMSE
```
cd NeuroBEM && conda run -n neurobem python analysis/measure_bem_RMSE.py
```

### MATLAB
No CLI build/test harness — scripts are run inside MATLAB. Every function is documented; use `help functionName` (e.g. `help mergefile` prints the segment-CSV column order). Shared helpers live in `Matlab/Common/` and per-area `subroutines/` folders.

## Conventions & gotchas

- **MINIMAL COMMENTS AND DOCSTRINGS.** Keep comments and docstrings to an absolute minimum — code should be self-explanatory. Keep the code itself as simple as possible.
- **ALWAYS CHECK THE SOURCE, ALWAYS CITE IT.** Never answer factual questions about the code/data (what a value is, where a signal comes from, how something is computed) from memory, the summary, or inference — open the actual file and verify. Every claim must come with a source the user can open: `file:line` (e.g. [MergeAndPreprocessData.m:277](NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L277)), not a vague reference. Trace data provenance to the exact line that assigns/derives it.
- **Flight-file naming is load-bearing.** All artifacts for one flight share a base ID (typically a timestamp like `2021-02-03-13-43-38`) with different prefixes/suffixes/extensions: `merged_<ID>_seg_X.csv`, `bem_<ID>_seg_X.csv`, `<ID>_traj.csv`, `<ID>.BFL`, etc. Scripts glob on these patterns, so renames break the pipeline.
- BEM coning/flapping angles use the *linear* lift/drag coefficients (`param.a`/`param.d`); changing the *nonlinear* `param.cl`/`param.cd` will not move those angles. This is intentional (tractability).
- `Coning.m` is described in the README as fragile/"a hack" (FFT of audio to recover prop RPM) — handle with care.
- **Multiple mass/inertia values are in play — cite the right one.** The C++ simulator and `analysis/measure_bem_RMSE.py` use `mass = 0.752`, `I = [0.00254, 0.00214, 0.00436]` ([params.h:115-118](NeuroBEM/code/simulator/include/params.h#L115)). The dataset `README.md` (and `make_nn_targets.py`, and the paper) use `0.772`, `[0.0025, 0.0021, 0.0043]`. Check which a given script uses before computing forces/torques from `processed_data/bem/`.
- Gitignored outputs you should not commit: `Python/train_logs/`, `simulator/build/`, `Matlab/tmp/`, `*.trt`, `*.asv`, `NeuroBEM/processed_data/`, `NeuroBEM/raw_data/`.
