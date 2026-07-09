# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## My goal, my rules and my story

This repo goal is to recreate the NeuroBEM paper based on its attachements. My current additions are analysis/ which is for me to play around and analyze the data and outputs of the BEM model. Next, i recieved from my supervisor part of the simulator (in folder my_bem/), that is from the same lab as the paper, that implements BEM inside.

From the MRS web page:

Learned Hybrid Aerodynamic Modeling for Agile Multirotor Flight
While control, planning, and perception have already reached significant milestones in UAV research, modeling aerodynamics remains one of the most open-ended problems in the field. This is caused by the complex phenomena that emerge during agile motion of multirotor UAVs, where each propeller blade is itself a wing interacting with all others. Classical modeling and gray-box approaches quickly fall short, and for agile flight of larger drones, this becomes a bottleneck that must be addressed. This project focuses on hybrid aerodynamic modeling that combines first-principles theory with machine learning. The work compares different hybrid modeling strategies and evaluates their tracking and disturbance-rejection performance in simulation and in real flight on agile platforms.
[1] M. O'Connell, G. Shi, X. Shi, K. Azizzadenesheli, A. Anandkumar, Y. Yue, S.-J. Chung. "Neural-Fly enables rapid learning for agile flight in strong winds." Science Robotics, vol. 7, no. 66, eabm6597, 2022.
[2] L. Bauersfeld, E. Kaufmann, P. Foehn, S. Sun, D. Scaramuzza. "NeuroBEM: Hybrid Aerodynamic Quadrotor Model." Robotics: Science and Systems, 2021.

Advisor: Michal Pliska
Email: pliskmic@fel.cvut.cz

After conversation with AI chatbot I ended up on file current_goal.md

### Current direction ([current_goal.md](current_goal.md))

The active objective (sharpening the vaguer "recreate the paper" framing above): **port NeuroBEM into the Agilicious simulator and reproduce the paper's closed-loop tracking (Table III) entirely in sim.** Agilicious already ships the BEM rotor model; the work is to port the NN residual, wire it into the sim loop, and build the evaluation pipeline. No mocap access — all data and ground truth come from the public NeuroBEM dataset; we never record our own flights.

- **NN residual (fixed interface):** input `20×10` (3 body linvel, 3 body rates, 4 motor speeds) @ 2.5 ms = 50 ms history; output 6 = residual force+torque added to BEM. Two heads (force/torque), strictly causal, **bounded output** (an unbounded torque residual crashed the paper's ablation), exported to C++ (ONNX Runtime / libtorch) at 1 kHz with the training normalization constants shipped alongside.
- **Sim loop (per 1 ms tick):** low-level controller → first-order motor model → BEM+NN → rigid-body integrate. Use **symplectic Euler @ 1 ms** (Agilicious defaults to RK4 — switch it) and a BetaFlight-style rate loop, or comparisons to the paper are invalid.
- **Evaluation ladder (in order):** (1) single-step RMSE on the held-out split (Table I target ≈ 0.352 N / 5.3e-3 Nm), (2) open-loop motor-replay rollout (position drift over ~1 s windows), (3) closed-loop MPC flying the reference trajectories vs. the recorded flights (Table III). Model selection uses trajectory drift, not per-step RMSE.
- **Out of scope until the ladder reproduces the paper:** architecture swaps, multi-step/rollout training, BEM-solver improvements. `current_goal.md` is the full spec.


### IMPORTANT NOTE TO ALL AI CHATBOTS

When talking to me start the message with "TARS:". When writing answer always keep it concise and information dense. But do not skip important stuff. When I tell you to fuck off or shout and curse at you DO NOT apologize, it wastes tokens, just follow orders. Next when writing code DO NOT write stupid comments and docstrings. Keep the code clean and high quality. When implementing stuff KEEP IT SIMPLE. This important. I do not want to read 1000 lines of diffs, I want to look at the change and know what it does. As one artist said "One good girl is worth a thousand bitches.".


## What this is

NeuroBEM is a hybrid aerodynamic quadrotor model that augments a first-principle Blade Element Model (BEM) with a neural network that learns the residual forces/torques the BEM cannot predict. Reference: *"NeuroBEM: Hybrid Aerodynamic Quadrotor Model", 2021, L. Bauersfeld et al.* (see `RSS21_Bauersfeld.pdf`) and ALSO .md file - same name.

The actual framework lives in [code/](code/). The repo root also holds working data/output produced by running the pipeline (`bem+nn/`, `processed_data/`, `pdf/`) — these are flight-data CSVs and per-flight visualization PDFs, not source code. [code/README.md](code/README.md) is the authoritative end-to-end tutorial for the *pipeline*; consult it for the full workflow and per-parameter details.

### Root-level dataset docs

Three root-level files document the released *dataset* (1h15min of flights, distinct from the code tutorial):
- [Readme.md](Readme.md) — dataset-level README: folder layout (`processed_data/`, `raw_data/`, `pdf/`, `predictions/`, `code/`) and the exact CSV column orders (29 cols for processed data; +12 cols 30–41 for `predictions/` = predicted force/torque + residuals). Drone mass 0.772 kg, diagonal inertia [0.0025, 0.0021, 0.0043]. Note: this README's `predictions/` folder is the dataset-distribution name for the same stage-2 output the pipeline docs call `MODEL/`/`bem+nn/`.
- [Flights.txt](Flights.txt) — catalog of all 95 flights as MATLAB `dataset = "<timestamp>"` lines, each commented with its trajectory type (circles, lemniscates, linear/vertical oscillations, cpc, random points, satellites, ellipse).
- [testset.txt](testset.txt) — 13 held-out `<ID>_seg_X` segments defining the manual test hold-out (all from the 02-18 and 02-23 sessions). This is the dataset-level hold-out list; the pipeline's own `testset.txt` consumed by `get_datafiles.bash` lives under [code/Python/data/](code/Python/data/).

## Architecture: the three-stage pipeline

Data flows through three loosely-coupled stages that communicate via CSV files on disk. There is no single orchestrator — each stage is run manually and writes files the next stage reads.

1. **BEM identification & physical model (MATLAB + C++)** — Identify propeller physical parameters from thrust-test-stand data, then encode them into a C++ simulator.
   - MATLAB scripts under [code/Matlab/BEM/](code/Matlab/BEM/) fit lift/drag coefficients and the hinge-spring constant (`ParameterID.m`, `QuadraticFit.m`, `Coning.m`). Geometry/identified params are hand-edited into `Matlab/BEM/subroutines/setParam.m`.
   - The flapping/coning equations are derived symbolically in `Maple/BEM_Derivation.mw` and the generated formulae are implemented in both MATLAB subroutines and the C++ simulator (`calc_a0`/`calc_a1s`/`calc_b1s` ↔ `calculateConing.cpp` etc.). MATLAB is only a tool to produce values for C++; it is intentionally not optimized.

2. **Flight-data processing (MATLAB) → base-model application (C++)** — Merge sensor sources, then run the BEM over them to produce training data.
   - `Matlab/OptiTrack/MergeAndProcessData.m` merges Rosbag (OptiTrack pose) + Betaflight motor-speed logs + trajectory into `merged_*` and `merged_*_seg_X.csv` files. The `_seg_X` segment files (airborne portions) are the ones used downstream.
   - The C++ `bem-model` executable ([code/simulator/](code/simulator/)) reads each `merged_*_seg_*.csv`, predicts forces/torques, and writes `MODEL/MODEL_<flight>_seg_X.csv` (the NN training data = measured minus predicted). Run via `Scripts/applyBM.sh DATAFOLDER`.

3. **Neural network (Python/TensorFlow)** — Train a network on the residuals. Code under [code/Python/](code/Python/).

**Python env:** all Python (`analysis/` + `code/Python/`) runs in the `neurobem` conda env — `conda run -n neurobem python ...`.

### Key cross-stage coupling to be aware of

- **`MODEL` is a compile-time choice.** The base model is selected by `#define MODEL` in [code/simulator/include/params.h](code/simulator/include/params.h): `1` = BEM, `0` = quadratic fit, `-1` = none. Changing it **requires rebuilding the simulator**, and the `model` variable in `Scripts/applyBM.sh` (and the `base_type` in the Python settings) must be changed to match. `MODEL` also drives the `CHORD`/`POLAR`/`DIST` sub-defines.
- **Identified parameters are duplicated by hand**, not shared via a file. Values from MATLAB land in `setParam.m`, in `params.h` for the simulator, and ultimately in agilicious `sim_*.yaml`/`.hpp` files. When a physical parameter changes, all copies must change.
- **Aero params are compile-time constants.** The propeller `param` is `static constexpr Propeller_s` ([propeller.h:82](code/simulator/include/propeller.h#L82)) with the `params.h` defaults as the single source of truth; changing `cl`/`cd`/`k` needs a rebuild. (An earlier experiment made `param` an instance member with a `setAero()` runtime override so the `identify` tool could re-fit without recompiling; that was reverted in the working tree along with `identify.cpp` — see the re-identification note below.)
- The Python `base_type` setting (`"bem"`/`"fit"`/`"none"`) selects which `MODEL`-output subfolder to train on.

### Python NN internals

- Entry points: `train.py` and `test.py`, both `--settings_file config/bem_settings.yaml`. `Learner` ([code/Python/learner.py](code/Python/learner.py)) owns the full TF training/eval loop; `config/settings.py` parses and validates the YAML (asserts data dirs exist, sets `CUDA_VISIBLE_DEVICES`, etc.).
- `utils/` holds the pieces wired together by the learner: `nets.py` (MLP / TCN / RNN architectures, selected by `network.architecture`), `loader.py` + `window_generator.py` (sliding-window dataset, `history_len`), `loss.py` (separate `ForceLoss`/`TorqueLoss` with per-axis weights), `normalization.py`, `visualization.py`.
- The network input feature set is configured by the `dataloading.use_*` flags in the YAML (linvel/angvel/motors etc.), so feature length depends on config and must stay consistent with the deployed ONNX/TensorRT shape (`H`=history_len, `FL`=feature length).
- Deployment path for agilicious is TensorRT: trained `.pb` → ONNX (`tf2onnx`) → serialized `.trt` engine, verified with [code/Python/trt/](code/Python/trt/) against `predict_from_pb.py` output. Engines are machine-specific and gitignored.

## Model background (from the NeuroBEM paper)

`RSS21_Bauersfeld.pdf` (with a grep-able markdown transcription at [RSS21_Bauersfeld.md](RSS21_Bauersfeld.md)) is the source of truth for *why* the code is shaped this way. The whole framework computes one equation:

```
f = f_prop + f_res        τ = τ_prop + τ_res
```

A **rotor model** (first principles) predicts `f_prop`/`τ_prop`; a **neural network** predicts the residuals `f_res`/`τ_res` (body/frame aero + rotor-to-rotor interactions the rotor model ignores). This split is exactly stages 1–2 (rotor model in C++) vs stage 3 (NN in Python) above.

- **The `MODEL` define maps to the paper's three rotor models:** `-1` None (predict zeros, a naive baseline), `0` Quadratic/"Fit" (thrust & torque ∝ Ω², coefficients from a static test stand — good only near hover), `1` BEM (the accurate model). Paper Table II/III compares None / Fit / BEM each ±NN; "BEM+NN" is the proposed method.
- **BEM internals** (the C++ `simulator/src/simulator/` files): blade-element-momentum theory needs the **induced velocity `vi`**, which has no closed form — it's solved numerically via GSL (`gslHelper.cpp` / `propeller.cpp::_calculateInducedVelocity`, the runtime-dominant step, ~100 µs). With `vi` known, the **coning angle `a0`** and **flapping angles `a1`/`b1`** are evaluated (`calculateConing.cpp`, `calculateLongitudinalFlapping.cpp`, `calculateLateralFlapping.cpp` — these are the long auto-generated expressions from the Maple worksheet). There is special handling for **vortex-ring state** (descending into own downwash) where momentum theory breaks down and an empirical quartic fit for `vi` is used.
- **Lift/drag polar:** `cl(α)=cl,0·sinα·cosα`, `cd(α)=cd,0·sin²α` (the `POLAR` setting; `param.cl`/`param.cd`). The coning/flapping use the separate *linear* coefficients (`param.a`/`param.d`) for tractability — this is why the README warns that changing `param.cl`/`param.cd` doesn't move the angles. `kβ` is the hinge-spring stiffness; it appears directly in the final propeller torque `τ_P`.
- **The NN feature set is dictated by the paper:** inputs are linear velocity, angular velocity, and motor speeds over a history of **h = 20** samples at **δt = 2.5 ms** (= 50 ms of context). This is exactly `dataloading.history_len: 20` + `use_linvel/use_angvel/use_motors: True` in `bem_settings.yaml`. The paper's ablation (Table I) selected **TCN-medium**, hence `network.architecture: "TCN"` is the intended default; MLP/RNN exist for comparison.
- **Quadrotor & sim constants** that appear in code: motor first-order dynamics with time constant **τΩ = 33 ms** (`params.h: tau = 0.033`); the closed-loop simulator integrates with a **symplectic Euler** scheme at 1 ms (chosen for energy conservation); platform mass ≈ 0.772 kg with diagonal inertia (the `quadrotor:` block in the YAML).
- **Data-processing rationale** (stage 2 MATLAB): Vicon pose at 400 Hz and onboard IMU + motor speeds at 1 kHz are asynchronous, so `MergeAndProcessData` fits **cubic splines** to fuse them and differentiates the splines to get low-noise linear velocity / angular acceleration. Time sync (offset + ~2.4% clock skew) is recovered by correlating gyro rates against the spline (the `align_data` subroutine); motor speeds get a 4th-order Butterworth low-pass. Full dataset in the paper: 96 flights / 1.8 M points, split 70/20/10 — matching the train/val/test counts produced by `get_datafiles.bash`.

## Exploratory analysis (`analysis/`)

A separate Python workspace (added after the pipeline docs above; **formerly `EDA/`** — that folder no longer exists, its files moved into `analysis/` and `utility.py` was renamed `utils.py`). Used for data-quality analysis and BEM-baseline evaluation (and, historically, propeller aero re-identification — see the note below). Unlike the pipeline, these scripts read the **committed-locally, gitignored** `processed_data/` directly (repo-root, not `code/ExampleData/`), so they only run where that data is present. Deps: [analysis/requirements.txt](analysis/requirements.txt) (numpy/pandas/scipy/matplotlib/jupyter).

- [analysis/utils.py](analysis/utils.py) — shared loaders, noise metrics, and the **canonical column layout**. `load_flight`/`load_largest_segment` read `processed_data/merged_*_seg_*.csv` (29-column merged order hard-coded in `COLUMNS`, [utils.py:18](analysis/utils.py#L18)). `load_bem_flight`/`load_bem` read `processed_data/bem/bem_*_seg_*.csv`. The files **currently on disk are 47 columns**: the 29 merged cols **+ 6 predicted** `fx,fy,fz,tx,ty,tz` (29–34) **+ 12 per-motor diagnostics** `vi,mu,as` for motors 1–4 (35–46, indexed via `VI`/`MU`/`AS` at [utils.py:34](analysis/utils.py#L34)). ⚠ Those 12 diagnostics came from an earlier simulator build; the **current `bem-model` emits only the 6 predicted cols (35 total)**, so regenerating `processed_data/bem/` will break `load_bem`'s `VI`/`MU`/`AS` indexing until the diagnostics path is restored. `measured()`/`residuals()` compute force `= mass·acc` (acc already includes gravity) and torque `= I·ang_acc + ω×Iω` using the **simulator** mass/inertia ([utils.py:14](analysis/utils.py#L14)). `FS = 400 Hz`. SNR/noise split signal from noise with a 4th-order **25 Hz Butterworth** low-pass ([utils.py:89-102](analysis/utils.py#L89)); `noise_corpus(cache=…)` builds/caches the per-channel SNR table.
- [analysis/measure_bem_RMSE.py](analysis/measure_bem_RMSE.py) — BEM baseline residual RMSE (measured − predicted) over the full set and the `testset.txt` hold-out. Run: `python3 analysis/measure_bem_RMSE.py` (defaults to `processed_data/bem` + root `testset.txt`; self-contained, hard-codes the simulator `mass = 0.752` / inertia).
- **Re-identification workflow (historical — the C++ side was removed).** [analysis/make_subset.py](analysis/make_subset.py) pools all non-test `bem_*.csv`, bins rows on a `(mu, vi)` grid, and samples ~equally per cell so aggressive regimes aren't drowned out by hover — writing the fixed `analysis/subset_20k.csv` (47-col format), which is still present. The C++ `identify` tool that consumed it (a from-scratch **CMA-ES** re-fitting `(cl, cd, k)` in normalized coords, minimizing combined force+torque RMSE) has been **deleted from the working tree** along with its `setAero`/`getDiagnostics` plumbing; restore `code/simulator/src/identify.cpp` from git history to re-run it.
- The 12 per-motor diagnostics were emitted by a now-reverted simulator path (`Propeller::getDiagnostics()` → `Quadcopter::getDiagnostics()`). Notebooks: [analysis/feature_analyis.ipynb](analysis/feature_analyis.ipynb), [analysis/pred_analysis.ipynb](analysis/pred_analysis.ipynb), [analysis/vi_analysis.ipynb](analysis/vi_analysis.ipynb).
- `processed_data/bem/` is the local equivalent of the pipeline's stage-2 `MODEL/`/`bem+nn/` output that these scripts consume. `my_bem/simulator/` is a separate (agilicious-style) C++ model source tree, distinct from `code/simulator/`.

## Common commands

All paths below are relative to [code/](code/).

### Build the C++ simulator
Requires GSL and Eigen (`sudo apt-get install libgsl-dev libeigen3-dev`). The repo-root [build/](build/) directory is a pre-existing CMake configuration of the simulator (the VS Code CMake extension points at `code/simulator` via `.vscode/settings.json`). To build from scratch:
```
cd code/simulator
mkdir build && cd build
cmake ..
make            # produces the `bem-model` executable
```
CMake options (default ON): `ENABLE_FAST`, `UNSAFE_MATH`, `ENABLE_PARALLEL` (OpenMP), `EIGEN_FROM_SYSTEM`. C++17, `-march=native`. One target: `bem-model` (pipeline stage 2). An `identify` target (CMA-ES aero re-identification) used to build here but its source `identify.cpp` was removed from the working tree.

### Apply the base model to flight data
```
cd code/Scripts
./applyBM.sh ../ExampleData/OptiTrack/        # or pass a filelist as 2nd arg
```

### Prepare the NN dataset (train/val/test split)
```
cd code/Python/data
./get_datafiles.bash ../../ExampleData/OptiTrack/ bem
```
Splits are copied into `data/bem/{train,validation,test}`. `testset.txt` defines a manual hold-out set.

### Train / monitor / test the network
```
cd code/Python
python3 train.py --settings_file config/bem_settings.yaml
tensorboard --logdir=train_logs            # checkpoints + best_model under train_logs/TIMESTAMP/
python3 predict_from_pb.py --log_folder train_logs/TIMESTAMP/    # sanity-check (ones input)
python3 generate_ablation_study.py --load_folder train_logs/TIMESTAMP --data_root data/bem/test --output_dir .
```

### MATLAB
No CLI build/test harness — scripts are run inside MATLAB. Every function is documented; use `help functionName` (e.g. `help mergefile` prints the segment-CSV column order). Shared helpers live in `Matlab/Common/` and per-area `subroutines/` folders.

## Conventions & gotchas

- **MINIMAL COMMENTS AND DOCSTRINGS.** Keep comments and docstrings to an absolute minimum — code should be self-explanatory. Keep the code itself as simple as possible.
- **ALWAYS CHECK THE SOURCE, ALWAYS CITE IT.** Never answer factual questions about the code/data (what a value is, where a signal comes from, how something is computed) from memory, the summary, or inference — open the actual file and verify. Every claim must come with a source the user can open: `file:line` (e.g. [MergeAndPreprocessData.m:277](code/Matlab/OptiTrack/MergeAndPreprocessData.m#L277)), not a vague reference. Especially trace data provenance to the exact line that assigns/derives it (e.g. which columns are Betaflight vs OptiTrack in the merge).
- **Flight-file naming is load-bearing.** All artifacts for one flight share a base ID (typically a timestamp like `2021-02-03-13-43-38`) with different prefixes/suffixes/extensions: `merged_<ID>_seg_X.csv`, `bem_<ID>_seg_X.csv`, `<ID>_traj.csv`, `<ID>.BFL`, etc. Scripts glob on these patterns, so renames break the pipeline.
- BEM coning/flapping angles use the *linear* lift/drag coefficients (`param.a`/`param.d`); changing the *nonlinear* `param.cl`/`param.cd` will not move those angles. This is intentional (tractability).
- `Coning.m` is described in the README as fragile/"a hack" (FFT of audio to recover prop RPM) — handle with care.
- **Two different mass/inertia values are in play — cite the right one.** The C++ simulator uses `mass = 0.752`, `I = [0.00254, 0.00214, 0.00436]` ([params.h:115-118](code/simulator/include/params.h#L115)), and `analysis/utils.py` / `measure_bem_RMSE.py` follow it. The dataset `Readme.md` (and the paper-derived numbers earlier in this file) quote `0.772` and `[0.0025, 0.0021, 0.0043]`. Use the simulator values for anything computing forces/torques from `processed_data/bem/`.
- Gitignored outputs you should not commit: `Python/train_logs/`, `simulator/build/`, `Matlab/tmp/`, `*.trt`, `*.asv`.
