# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

NeuroBEM is a hybrid aerodynamic quadrotor model that augments a first-principle Blade Element Model (BEM) with a neural network that learns the residual forces/torques the BEM cannot predict. Reference: *"NeuroBEM: Hybrid Aerodynamic Quadrotor Model", 2021, L. Bauersfeld et al.* (see `RSS21_Bauersfeld.pdf`).

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

### Key cross-stage coupling to be aware of

- **`MODEL` is a compile-time choice.** The base model is selected by `#define MODEL` in [code/simulator/include/params.h](code/simulator/include/params.h): `1` = BEM, `0` = quadratic fit, `-1` = none. Changing it **requires rebuilding the simulator**, and the `model` variable in `Scripts/applyBM.sh` (and the `base_type` in the Python settings) must be changed to match. `MODEL` also drives the `CHORD`/`POLAR`/`DIST` sub-defines.
- **Identified parameters are duplicated by hand**, not shared via a file. Values from MATLAB land in `setParam.m`, in `params.h` for the simulator, and ultimately in agilicious `sim_*.yaml`/`.hpp` files. When a physical parameter changes, all copies must change.
- The Python `base_type` setting (`"bem"`/`"fit"`/`"none"`) selects which `MODEL`-output subfolder to train on.

### Python NN internals

- Entry points: `train.py` and `test.py`, both `--settings_file config/bem_settings.yaml`. `Learner` ([code/Python/learner.py](code/Python/learner.py)) owns the full TF training/eval loop; `config/settings.py` parses and validates the YAML (asserts data dirs exist, sets `CUDA_VISIBLE_DEVICES`, etc.).
- `utils/` holds the pieces wired together by the learner: `nets.py` (MLP / TCN / RNN architectures, selected by `network.architecture`), `loader.py` + `window_generator.py` (sliding-window dataset, `history_len`), `loss.py` (separate `ForceLoss`/`TorqueLoss` with per-axis weights), `normalization.py`, `visualization.py`.
- The network input feature set is configured by the `dataloading.use_*` flags in the YAML (linvel/angvel/motors etc.), so feature length depends on config and must stay consistent with the deployed ONNX/TensorRT shape (`H`=history_len, `FL`=feature length).
- Deployment path for agilicious is TensorRT: trained `.pb` → ONNX (`tf2onnx`) → serialized `.trt` engine, verified with [code/Python/trt/](code/Python/trt/) against `predict_from_pb.py` output. Engines are machine-specific and gitignored.

## Model background (from the NeuroBEM paper)

`RSS21_Bauersfeld.pdf` is the source of truth for *why* the code is shaped this way. The whole framework computes one equation:

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
CMake options (default ON): `ENABLE_FAST`, `UNSAFE_MATH`, `ENABLE_PARALLEL` (OpenMP), `EIGEN_FROM_SYSTEM`. C++17, `-march=native`.

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

- **Flight-file naming is load-bearing.** All artifacts for one flight share a base ID (typically a timestamp like `2021-02-03-13-43-38`) with different prefixes/suffixes/extensions: `merged_<ID>_seg_X.csv`, `bem_<ID>_seg_X.csv`, `<ID>_traj.csv`, `<ID>.BFL`, etc. Scripts glob on these patterns, so renames break the pipeline.
- BEM coning/flapping angles use the *linear* lift/drag coefficients (`param.a`/`param.d`); changing the *nonlinear* `param.cl`/`param.cd` will not move those angles. This is intentional (tractability).
- `Coning.m` is described in the README as fragile/"a hack" (FFT of audio to recover prop RPM) — handle with care.
- Gitignored outputs you should not commit: `Python/train_logs/`, `simulator/build/`, `Matlab/tmp/`, `*.trt`, `*.asv`.
