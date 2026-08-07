# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## IMPORTANT NOTE TO ALL AI CHATBOTS

When talking to me start the message with "TARS:". When writing answer always keep it concise and information dense. But do not skip important stuff. When I shout and curse at you DO NOT apologize, it wastes tokens, just follow orders. Next when writing code DO NOT write stupid comments and docstrings. Keep the code clean and high quality. When implementing stuff KEEP IT SIMPLE. This important. I do not want to read 1000 lines of diffs, I want to look at the change and know what it does.
Also IDE is wrongly configured, so it will report wrong include errors etc. So do not worry about it. Only investigate when neccessary. 

When listing commands to me, one command one line. If there is more than one command or it is super duper complex i will punish you. HARD.

**NO EXTREME RATIONALE COMMENTS. EVER.** Do not write comments that OVERexplain a design choice was made, justify a trade-off, cite a benchmark, reference a spec section, or narrate what the code is doing. I can read the code. Comments explaining intent belong in the commit message or in your reply to me, never in the source. The only comments allowed are: units, non-obvious external constraints (a data format, a hardware quirk), and a one-line file header. If you catch yourself writing "so that", "because", "this means", "note that", or "matches X" in a comment — delete it. Same rule for docstrings: a short one-liner or nothing.

## My goal, my rules and my story

This repo's goal is to recreate the NeuroBEM paper and port it into the **Agilicious** simulator for closed-loop evaluation. `NeuroBEM/analysis/` is my own workspace to play with and analyze the data and outputs of the BEM model. `agilicious/simulator/` is part of the simulator I received from my supervisor (from the same lab as the paper) that implements BEM inside — the deployment target in the future.

From the MRS web page:

> **Learned Hybrid Aerodynamic Modeling for Agile Multirotor Flight**
> While control, planning, and perception have already reached significant milestones in UAV research, modeling aerodynamics remains one of the most open-ended problems in the field. This is caused by the complex phenomena that emerge during agile motion of multirotor UAVs, where each propeller blade is itself a wing interacting with all others. Classical modeling and gray-box approaches quickly fall short, and for agile flight of larger drones, this becomes a bottleneck that must be addressed. This project focuses on hybrid aerodynamic modeling that combines first-principles theory with machine learning.
> [1] M. O'Connell et al. "Neural-Fly enables rapid learning for agile flight in strong winds." Science Robotics, 2022.
> [2] L. Bauersfeld et al. "NeuroBEM: Hybrid Aerodynamic Quadrotor Model." RSS, 2021.
>
> Advisor: Michal Pliska (pliskmic@fel.cvut.cz)

### Current direction

New work goes in **[MyBEM/](MyBEM/)**, not in `NeuroBEM/` — the latter is the frozen paper reference and cross-check.

Long-running steer: (1) reproduce the paper's results, (2) CMA-ES for BEM param tuning, (3) get the whole pipeline working, (4) ablations — leave out features, see how much the TCN degrades, (5) study polyfit and how it could be used here. Main task afterwards: **apply it to the data from the Eagle drone and then in a closed-loop simulation.**

## Repository layout

This repo is a workspace of four loosely-coupled parts:

- **[MyBEM/](MyBEM/)** — **the active rebuild.** A clean-break reimplementation of the whole pipeline as independent, composable stages with explicit artifacts. Spec in [MyBEM/DESIGN.md](MyBEM/DESIGN.md); C++ base model + PyTorch residual net both exist. See the MyBEM section below.
- **[NeuroBEM/](NeuroBEM/)** — the main working tree: the NeuroBEM framework ([NeuroBEM/code/](NeuroBEM/code/)), my exploratory analysis ([NeuroBEM/analysis/](NeuroBEM/analysis/)), the CMA-ES reporting notebooks ([NeuroBEM/CMAES-results-analysis/](NeuroBEM/CMAES-results-analysis/)), dataset docs ([NeuroBEM/README.md](NeuroBEM/README.md), [NeuroBEM/Flights.txt](NeuroBEM/Flights.txt), [NeuroBEM/testset.txt](NeuroBEM/testset.txt)), and gitignored flight data/outputs (`processed_data/`, `raw_data/`, `pdf/`, `bem+nn/`, `CMAES-results/`). [NeuroBEM/code/README.md](NeuroBEM/code/README.md) is the authoritative end-to-end tutorial for the *pipeline*.
- **[agilicious/simulator/](agilicious/simulator/)** — extracted Agilicious C++ simulator sources (models incl. `model_propeller_bem*.cpp` + `bem/`, the BetaFlight/simple low-level controllers, `quadrotor_simulator.cpp`), the **deployment target** for the final closed-loop sim. Reference source only — no build system or headers checked in here.
- **[research/](research/)** — reading material and write-ups. Two folders, nothing else: no index, no schema, no maintenance skills. Conceptual questions are answered from `sources/` + the code with a `file:line` citation.
  - **[research/sources/](research/sources/)** — inputs. **Never edit a file here**; adding a new one is fine. [papers/](research/sources/papers/) holds each paper as PDF + a grep-able PDF→text transcription: [RSS21_Bauersfeld.md](research/sources/papers/RSS21_Bauersfeld.md) (NeuroBEM) and [scirobotics.abm6597.md](research/sources/papers/scirobotics.abm6597.md) (Neural-Fly). The transcriptions have no headings — cite them by line number. [notes/](research/sources/notes/) is my own loose notes: explorations, open questions, notes on the code under `NeuroBEM/`.
  - **[research/reports/](research/reports/)** — findings: concise, information-dense, no fluff. **Write one only when I ask** — never file a report unprompted. Both current files (`CMAES-identification.md`, `Generalization.md`) are empty placeholders.

### Root-level dataset docs (under `NeuroBEM/`)

- [NeuroBEM/README.md](NeuroBEM/README.md) — dataset-level README: folder layout and exact CSV column orders (29 cols for processed data; +12 cols 30–41 for `predictions/` = predicted force/torque + residuals). Drone mass 0.772 kg, diagonal inertia [0.0025, 0.0021, 0.0043]. Its `predictions/` folder is the dataset name for the stage-2 output the pipeline docs call `MODEL/`. **`NeuroBEM/bem+nn/` is that folder as downloaded** — the paper authors' own BEM+NN predictions (41 cols), a reference to compare against, *not* something this repo regenerates.
- [NeuroBEM/Flights.txt](NeuroBEM/Flights.txt) — catalog of all 95 flights as MATLAB `dataset = "<timestamp>"` lines, each commented with its trajectory type.
- [NeuroBEM/testset.txt](NeuroBEM/testset.txt) — 13 held-out `<ID>_seg_X` segments (dataset-level hold-out). The pipeline's own `testset.txt` consumed by `get_datafiles.bash` lives under [NeuroBEM/code/Python/data/](NeuroBEM/code/Python/data/).

## MyBEM — the rebuild (active work)

A clean-break reimplementation of the pipeline. `NeuroBEM/` stays frozen as the paper reference and the fallback/cross-check; nothing is imported from it except the merged flight CSVs. **[MyBEM/DESIGN.md](MyBEM/DESIGN.md) is the spec** — §1 (four artifact kinds), §5 (CLI), §6 (C++ architecture), §10 (decisions/non-goals). §10 "OPEN" is empty: the spec is settled, don't relitigate it.

**What exists today: both halves.** C++ — two binaries (`mybem-apply`, `mybem-tune`) + the `mybem_sim` library, ~1500 lines under [MyBEM/cpp/](MyBEM/cpp/). Python — the `mybem/` package (~575 lines, **PyTorch**, not TF): [drone.py](MyBEM/mybem/drone.py), [data.py](MyBEM/mybem/data.py), [train.py](MyBEM/mybem/train.py), [eval.py](MyBEM/mybem/eval.py), [nets/](MyBEM/mybem/nets/). Still **not written**: the single `mybem` CLI of DESIGN §5 (run modules directly, `python -m mybem.train`), the `@hash6` in the `store/<kind>/<name>@<hash6>/` artifact layout of DESIGN §2.2 (dirs are flat `store/nets/<name>/`, `store/preds/<name>/`, `store/tune/<name>/`), and the sweep/run-table of §4.6/§7.7. `Component::diagnose()`/`diagnostics()` ([component.h:21-22](MyBEM/cpp/include/mybem/component.h#L21)) are declared and never called. **`MyBEM/DESIGN.md` is still untracked** (`??` in git status).

### The object model

Two type aliases are the whole trick that killed the `#define`s ([types.h:13-15](MyBEM/cpp/include/mybem/types.h#L13)): `Params = map<string,double>` (everything CMA-ES can move) and `Options = map<string,string>` (structural — polar form, chord law, on/off — never tuned).

- **`Component`** ([component.h:10](MyBEM/cpp/include/mybem/component.h#L10)) — an additive `+=` contributor, shaped like `agi::ModelBase`: `add(State, Airframe, Wrench&)`, `load(Params, Options)`, `params()`, `tunables()`. `tunables()` returns `{key, def, lo, hi}` and is the **per-component replacement for the old global 21-entry `REGISTRY`**.
- **`Model`** ([model.cpp](MyBEM/cpp/src/model.cpp)) — an `Airframe` + an ordered `vector<ComponentPtr>`; `evaluate()` is a three-line sum ([model.cpp:108](MyBEM/cpp/src/model.cpp#L108)). Loading is **strict**: any unknown key `exit(1)`s ([model.cpp:71](MyBEM/cpp/src/model.cpp#L71)). Numeric YAML values route to `Params`, non-numeric to `Options`. Duplicate component types get suffixed prefixes (`bem`, `bem_2`), and `tunables()`/`values()`/`set()` namespace every key as `bem.lift_coefficient`, `airframe.dx` ([model.cpp:114-160](MyBEM/cpp/src/model.cpp#L114)).
- **`Airframe`** ([types.h:56](MyBEM/cpp/include/mybem/types.h#L56)) — `dx,dy,dz,thrust_scale` + `offsets()` and the hardcoded spin pattern `{CW,CCW,CCW,CW}`. Not a component; it is what components act on. Its params are tunable as `airframe.*`.
- **`Drone`** ([drone.h:9](MyBEM/cpp/include/mybem/drone.h#L9)) — mass + inertia, **the only place they are defined**, loaded from `configs/drones/*.yaml`. `Drone::measured()` is the sole measured-force/torque conversion. This kills the three-inconsistent-mass/inertia-sets problem of the old stack (see the gotcha table below).
- **`PropellerModel`** ([propeller_model.cpp](MyBEM/cpp/src/models/propeller_model.cpp)) — base for anything per-rotor; owns the rotor loop, kinematics (`vel = v_body + ω × offset`), and force/torque assembly. Subclasses supply only `thrust`/`torque`/`hforce`/`inducedVelocity`/`hasFlapping`. `add()` scales **only `force[2]`** by `thrust_scale`, *after* the moment arms used unscaled thrust ([propeller_model.cpp:40](MyBEM/cpp/src/models/propeller_model.cpp#L40)) — matches the original ordering.
- **Component registry** — five types, one `if` each ([registry.cpp:8](MyBEM/cpp/src/registry.cpp#L8)): `bem`, `quadratic`, `none`, `body_drag`, `motor_reaction`. Adding `polyfit` = one file + one line.
- `BEMModel` resolves `polar`/`chord`/`distortion` strings to function pointers in `load()` ([bem.cpp:56-75](MyBEM/cpp/src/models/bem.cpp#L56)); `GSLParams` carries them as plain fn pointers so the hot path inside the nested `qags` gets an indirect call, not a virtual one. One `GSLHelper` **per rotor**, lazily ([bem.h:29-31](MyBEM/cpp/include/mybem/models/bem.h#L29)) — deliberate, since the helper permanently widens its bracket window after a failed bracketing and a shared one would couple the rotors.
- BEM numerics (`gsl_helper.cpp`, `integrands.cpp`) and the three flapping polynomials (`coning.cpp`, `longitudinal_flapping.cpp`, `lateral_flapping.cpp`) are the **unchanged** Maple-generated math, restructured into free functions over `PropState`. The `cl`/`cd`-baked-into-the-constants limitation is stated at [flapping.h:7-9](MyBEM/cpp/include/mybem/bem/flapping.h#L7).

### Config = the model

`configs/models/bem_default.yaml`: root scalars (`name`, `drone`), an ordered `models:` list of additive components, and an `airframe:` block. `models: [bem]`, `[quadratic, body_drag]`, `[bem, motor_reaction, body_drag]` — all config lines, **no rebuild**. `yaml.cpp` is a hand-rolled ~90-line parser accepting exactly that one schema; anything else is a hard error with `file:line`. `Model::save` round-trips losslessly, emitting the fully-resolved config including defaults you never wrote.

### Commands

Build (needs GSL + Eigen + OpenMP; on macOS the CMakeLists auto-points at `brew --prefix libomp` since AppleClang ships no runtime, and a missing OpenMP is a **fatal error** unless you pass `-DENABLE_PARALLEL=OFF`. Options: `ENABLE_FAST`, `UNSAFE_MATH`, `ENABLE_PARALLEL`, `ENABLE_NATIVE`, `EIGEN_FROM_SYSTEM`/`EIGEN_ALTERNATIVE`, all ON by default):
```
cmake -S MyBEM/cpp -B MyBEM/cpp/build && cmake --build MyBEM/cpp/build
```
Apply a model to one segment (7-col output + `params.yaml` written next to it):
```
MyBEM/cpp/build/mybem-apply configs/models/bem_default.yaml INPUT.csv OUTPUT.csv
```
Batch every segment in a folder ([scripts/apply.sh](MyBEM/scripts/apply.sh) `BASEPATH MODEL CONFIG [filelist]` → `MyBEM/store/preds/MODEL/<id>.csv`; existing outputs are skipped, so it is safe to re-run):
```
MyBEM/scripts/apply.sh data/processed_data bem_default MyBEM/configs/models/bem_default.yaml
```
List a model's tunable names, report RMSE at the loaded values, and fit ([tune.cpp:320](MyBEM/cpp/src/apps/tune.cpp#L320)):
```
MyBEM/cpp/build/mybem-tune MODEL.yaml DATA.csv --list
MyBEM/cpp/build/mybem-tune MODEL.yaml DATA.csv --drone configs/drones/paper_quad.yaml
MyBEM/cpp/build/mybem-tune MODEL.yaml DATA.csv --drone configs/drones/paper_quad.yaml --free lift_coefficient,drag_coefficient,hinge_spring_constant --loss both --out RUNDIR
```
`--free NAMES|all` replaces the old `--cma MASK` bit string; a bare name is accepted when exactly one component offers it, otherwise write `bem.lift_coefficient` ([tune.cpp:263](MyBEM/cpp/src/apps/tune.cpp#L263)). `--free` requires `--out`, `--drone` is always required. Other flags: `--gens` (100), `--seed` (0), `--threads`, `--loss force|torque|both` (**default `force`**, [tune.cpp:237](MyBEM/cpp/src/apps/tune.cpp#L237)). Output dir gets `model.yaml` (a full config — feed it straight back to `mybem-apply` or resume a tune from it), `convergence.csv`, `metrics.csv` (baseline + best rows), `tune.yaml` (run record).

### mybem-apply / mybem-tune I/O

Both read a `merged_*_seg_X.csv` (≥29 cols, header, FLU) and take exactly the slices they need — `mybem-apply` reads angvel@4, linvel@14, motors@20, motor-accel@24 plus col 0 ([apply.cpp:13-17](MyBEM/cpp/src/apps/apply.cpp#L13)); `mybem-tune` additionally reads angacc@1 and acc@11 to build the measured wrench ([tune.cpp:28-34](MyBEM/cpp/src/apps/tune.cpp#L28)). Position and attitude are never read, because no component uses them. Internals are **FRD**; `flu2frd`/`frd2flu` convert at the boundary. Output is 7 columns, no header, `%.12g`: `t, fx, fy, fz, tx, ty, tz` (FLU).

CMA-ES is a from-scratch (mu/mu_w, lambda) implementation ([cma.cpp](MyBEM/cpp/src/tune/cma.cpp)) that knows nothing about the objective — bounds and scaling live in `SearchSpace` ([tune.cpp:147](MyBEM/cpp/src/apps/tune.cpp#L147)), which centres x-space on the **loaded model's current values**, not the hardcoded defaults, so a tune resumes from its own output. Loss is per-term MSE normalized by the baseline MSE, so the starting objective is exactly 1 per active term ([tune.cpp:386](MyBEM/cpp/src/apps/tune.cpp#L386)).

### Behavioral deltas vs. the old `bem-model` — do not treat these as bugs

- **Output shape**: 7 cols (t + 6), not the old 35 (29 passthrough + 6). There is no 41-col file and no `make_nn_targets.py` equivalent — [data.py:58-69](MyBEM/mybem/data.py#L58) rejoins prediction to merged CSV positionally (row-count + `t`-column check) and computes residuals **in RAM, never written**.
- **Precision**: `%lf` (6 decimal places) → `%.12g`. On ~1e-3 Nm torques the old writer kept ~3 significant digits. Numbers will differ from `processed_data/bem/`, and **the new ones are correct** — the acceptance-test caveat in DESIGN §9.4.
- **`none` is now genuinely zero.** Old `MODEL -1` skipped thrust but still ran the flapping angles, emitting a hinge-spring torque at zero thrust. `NoneModel::hasFlapping()` returns false ([simple.h:21](MyBEM/cpp/include/mybem/models/simple.h#L21)). The None-base generalization arm changes.
- **`motor_reaction` can actually fire.** The original read `domega` but never called `setMotorAcceleration`, so the term was dead; `apply.cpp:56` fills `s.dmot` for real. It is absent from `bem_default.yaml`, so the default stays faithful — add it and you get a term the old pipeline never had.
- **`body_drag` is opt-in**; the old code had no off switch.
- **No caching.** The `_valid`/`_validv1` dirty flags are gone; every `evaluate` recomputes.
- **Parallelism moved.** The rotor loop is `omp parallel for` ([propeller_model.cpp:26](MyBEM/cpp/src/models/propeller_model.cpp#L26)); `mybem-tune` parallelizes across *samples* with one `Model` per thread and `omp_set_max_active_levels(1)`, which collapses the inner rotor loop to serial ([tune.cpp:354](MyBEM/cpp/src/apps/tune.cpp#L354)). The row loop in `mybem-apply` is sequential **on purpose** — `GSLHelper` carries its bracket window across rows ([apply.cpp:49](MyBEM/cpp/src/apps/apply.cpp#L49)).

### The Python half (`MyBEM/mybem/`, PyTorch)

Four modules, no CLI wrapper, no packaging (`python -m mybem.<mod>` from `MyBEM/`). Env is **`mybem`**, not `neurobem` — [environment.yml](MyBEM/environment.yml), python 3.14, **everything from pip on purpose**: conda-forge numpy links llvm-openmp, the torch wheel bundles its own libomp, and two OpenMP runtimes in one process abort on import.

- **[drone.py](MyBEM/mybem/drone.py)** — `Drone.load(name)` from `configs/drones/*.yaml` + the merged-CSV column indices (`ANGACC`, `ANGVEL`, `ATT` reordered to w,x,y,z, `ACC`, `LINVEL`, `POS`, `MOTORS`, `VBAT`). `Drone.measured()` is the only measured-wrench conversion on the Python side; mass/inertia live nowhere else.
- **[data.py](MyBEM/mybem/data.py)** — `DATA = <repo>/data/processed_data`, `PREDS = MyBEM/store/preds` ([data.py:12-13](MyBEM/mybem/data.py#L12)). `split(name, ids)` reads `configs/splits/<name>.yaml`: pinned id lists come out first, the rest is shuffled once with `seed` and sized folds (`{n:}` or `{frac:}`) are cut off it; **train is whatever is left**, and an unknown pinned id is a hard error. `Data` loads all selected segments into one array plus per-segment `bounds`; `windows(history, max_speed)` returns start indices that never cross a segment boundary and (when filtering) whose *full span* is under the speed cut; `normalization(max_speed)` excludes masked rows.
- **[nets/](MyBEM/mybem/nets/)** — registry `{tcn, mlp}` ([nets/__init__.py:9](MyBEM/mybem/nets/__init__.py#L9)); one file + one line adds an architecture. Contract: `(batch, history, features) -> (batch, 6)`, force then torque. `two_heads: true` = the paper's two independent stacks. **`TimeConv`** ([tcn.py:13-27](MyBEM/mybem/nets/tcn.py#L13)) is `unfold + Linear`, not `nn.Conv1d` — identical arithmetic and param count, but ~0.01 ms vs ~2 ms per call on CPU at these shapes. Don't "simplify" it back.
- **[train.py](MyBEM/mybem/train.py)** — one experiment yaml in, `store/nets/<name>/{model.pt, config.yaml, normalization.yaml, tb/}` out. `config.yaml` is the experiment **plus the resolved `segments:` fold lists**, so a run records its own split. Seeded end to end (`cfg["seed"]` → `torch.manual_seed` + the batch-shuffle generator) — unlike the old TF stack. `cosine_restarts` ([train.py:20-28](MyBEM/mybem/train.py#L20)) reimplements `tf.keras.experimental.CosineDecayRestarts`. Loss = per-axis weighted MSE on **normalized** residuals. Best-val-loss checkpoint only.
- **[eval.py](MyBEM/mybem/eval.py)** — prints BEM vs BEM+NN in paper Table II format (`Fxy Fz F Mxy Mz M`) with the paper's numbers ([RSS21_Bauersfeld.md:437-443](research/sources/papers/RSS21_Bauersfeld.md#L437)) hardcoded underneath. Evaluation always uses the full speed envelope ([eval.py:42](MyBEM/mybem/eval.py#L42)).

Configs that exist: `models/{bem_default,none}.yaml`, `drones/paper_quad.yaml`, `splits/paper.yaml` (13 pinned test ids + `val: {frac: 0.21}`), `experiments/tcn_baseline.yaml`.

Commands (from `MyBEM/`):
```
conda run -n mybem python -m mybem.train configs/experiments/tcn_baseline.yaml
```
```
conda run -n mybem python -m mybem.eval tcn_baseline --on test
```
`train.py` flags override the yaml: `--seed`, `--name`, `--epochs`, `--limit N` (first N segments per fold — the smoke test).

### Data lives at the repo root

`/data/` (gitignored) is the single data root: `data/processed_data/` = the 247 `merged_*_seg_*.csv` **hard-linked** from `NeuroBEM/processed_data/` (same inodes), `data/CMAES-subsets/subset_20k.csv`. `MyBEM/store/`, `/data/` and `**/__pycache__/` are all gitignored now — never commit predictions, checkpoints or tb events.

### Map to `NeuroBEM/code/simulator`

| MyBEM | original |
|---|---|
| `apps/apply.cpp` | `simulator_node.cpp` + `simulator.cpp` (the `Simulator` class dissolved into `main`) |
| `model.cpp` `Model` | `quadcopter.cpp` `Quadcopter` |
| `types.h:56` `Airframe` | `Quadcopter::_placeMotors` + `Quadcopter_s` in `params.h:144` |
| `propeller_model.cpp` `add`/`evaluate` | `_calculateThrust`/`_calculateTorque` + `Motor::_update` + `Propeller::_update` (the `Motor` class is gone) |
| `models/bem.cpp`, `models/simple.h` | the `#if MODEL==1 / ==0 / ==-1` branches of `Propeller::_calculate*` |
| `gsl_helper.cpp` + `integrands.cpp` | `gslHelper.cpp` (solver split from integrands; `POLAR`/`CHORD` `#if` → fn pointers) |
| `csv.cpp`, `yaml.cpp` | `csvReader/csvWriter.cpp`, `config.h loadConfig` + `log_params` |
| `Component::tunables()` | `REGISTRY[21]` at `CMAES.cpp:54` |
| `apps/tune.cpp` + `tune/cma.cpp` | `CMAES.cpp` (both self-contained CMA-ES; the optimizer is now split out of the driver) |
| — | `params.h` `#define MODEL/POLAR/CHORD/DIST`, `Propeller_s::field()` reflection, `timeit.h` — all deleted |

## Architecture: the three-stage pipeline (`NeuroBEM/`, frozen reference)

Data flows through three loosely-coupled stages that communicate via CSV files on disk. There is no single orchestrator — each stage is run manually and writes files the next stage reads.

1. **BEM identification & physical model (MATLAB + C++)** — Identify propeller physical parameters from thrust-test-stand data, then encode them into a C++ simulator.
   - MATLAB scripts under [NeuroBEM/code/Matlab/BEM/](NeuroBEM/code/Matlab/BEM/) fit lift/drag coefficients and the hinge-spring constant (`ParameterID.m`, `QuadraticFit.m`, `Coning.m`). Geometry/identified params are hand-edited into `Matlab/BEM/subroutines/setParam.m`.
   - The flapping/coning equations are derived symbolically in `Maple/BEM_Derivation.mw` and the generated formulae are implemented in both MATLAB subroutines and the C++ simulator (`calc_a0`/`calc_a1s`/`calc_b1s` ↔ `calculateConing.cpp` etc.). MATLAB is only a tool to produce values for C++; it is intentionally not optimized.

2. **Flight-data processing (MATLAB) → base-model application (C++)** — Merge sensor sources, then run the base model over them to produce training data.
   - `Scripts/blackboxToCSV.sh PATH` first decodes every `<ID>.BFL` in a folder to `<ID>.01.csv` (needs the external *blackbox-tools* `blackbox_decode` on `PATH`; existing CSVs are skipped, so it is safe to re-run — [code/README.md:128-139](NeuroBEM/code/README.md#L128)).
   - `Matlab/OptiTrack/MergeAndPreprocessData.m` merges Rosbag (OptiTrack pose) + Betaflight motor-speed logs + trajectory into `merged_*` and `merged_*_seg_X.csv` files. The `_seg_X` segment files (airborne portions) are the ones used downstream.
   - The C++ `bem-model` executable ([NeuroBEM/code/simulator/](NeuroBEM/code/simulator/)) reads each `merged_*_seg_*.csv`, predicts forces/torques, and writes `<MODEL>/<MODEL>_<flight>_seg_X.csv`. Run via `Scripts/applyBM.sh BASEPATH MODEL CONFIG`. It also dumps the effective aero params as `params.yaml` next to the output file ([simulator.cpp:31](NeuroBEM/code/simulator/src/simulator/simulator.cpp#L31) → `Quadcopter::log_params` → `Propeller_s/Quadcopter_s::log_params` in [params.h:132](NeuroBEM/code/simulator/include/params.h#L132)), so the run records exactly which coefficients produced it.

3. **Neural network (Python/TensorFlow)** — Train a network on the residuals. Code under [NeuroBEM/code/Python/](NeuroBEM/code/Python/).

**Python env:** all Python (`NeuroBEM/analysis/` + `NeuroBEM/code/Python/`) runs in the `neurobem` conda env (py3.11, pinned deps in [NeuroBEM/requirements.txt](NeuroBEM/requirements.txt)) — `conda run -n neurobem python ...`.

### Key cross-stage coupling to be aware of

- **`MODEL` is a compile-time choice.** The base model is selected by `#define MODEL` at [params.h:10](NeuroBEM/code/simulator/include/params.h#L10): `1` = BEM, `0` = quadratic fit, `-1` = none. Changing it **requires rebuilding the simulator**. `MODEL` also drives the `CHORD`/`POLAR`/`DIST` sub-defines and which `cl`/`cd` defaults compile in ([params.h:93-115](NeuroBEM/code/simulator/include/params.h#L93)); `MODEL == -1` compiles `cl = cd = 0`, i.e. the model predicts zeros.
  **As of now `params.h` has `MODEL -1`**, so the binaries in `simulator/build/` are a *none* build — rebuild with `MODEL 1` before regenerating any BEM predictions.
- `applyBM.sh` no longer hardcodes the model: the **output subfolder name is argv[2]** and the params YAML argv[3]. The Python `base_type` setting must match that subfolder name (it is also the file prefix the loader matches on).
- **Identified parameters are duplicated by hand**, not shared via a file. Values from MATLAB land in `setParam.m`, in `params.h` for the simulator, and ultimately in agilicious `sim_*.yaml`/`.hpp` files. When a physical parameter changes, all copies must change.
- **Aero params are runtime-overridable (per instance).** The `params.h` `#define`s are the compile-time *defaults*, but `param` is a plain mutable instance member — `Propeller_s param = Propeller_s()` ([propeller.h:85](NeuroBEM/code/simulator/include/propeller.h#L85)), `Quadcopter_s param` ([quadcopter.h:47](NeuroBEM/code/simulator/include/quadcopter.h#L47)) — so any param can be changed at runtime without a rebuild. The plumbing: `Propeller_s::field(name)`/`Quadcopter_s::field(name)` ([params.h:112](NeuroBEM/code/simulator/include/params.h#L112), [params.h:150](NeuroBEM/code/simulator/include/params.h#L150)) return a `double*` to the named member (poor-man's reflection; unknown name → `nullptr`), and `Propeller::setParams`/`Quadcopter::setParams` ([propeller.cpp:148](NeuroBEM/code/simulator/src/simulator/propeller.cpp#L148), [quadcopter.cpp:29](NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L29)) write through them by name — `Propeller::setParams` also copies into `solver->p` (the GSL helper's `Propeller_s`) and invalidates caches; `Quadcopter::setParams` re-places motors and forwards leftover names to each `Motor`. YAML → map → `Quadcopter::load` ([quadcopter.cpp:31](NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L31)) via `loadConfig` ([config.h:11](NeuroBEM/code/simulator/include/config.h#L11)) is what both `bem-model` (CONFIG arg) and `cmaes` drive. Only structural/`#define`-gated choices (`MODEL`/`POLAR`/`CHORD`, which fields even exist) still need a rebuild.

### Python NN internals

- Entry points: `train.py` and `test.py`, both `--settings_file config/bem_settings.yaml`. `Learner` ([NeuroBEM/code/Python/learner.py](NeuroBEM/code/Python/learner.py)) owns the full TF training/eval loop; `config/settings.py` parses and validates the YAML (asserts data dirs exist, sets `CUDA_VISIBLE_DEVICES`, copies `settings.yaml` + `nets.py` into the new `train_logs/<ts2>/` — that copy is how you recover what a past run was).
- `utils/` holds the pieces wired together by the learner: `nets.py` (MLP / TCN / TCN_ORIG / RNN, selected by `network.architecture`), `loader.py` + `window_generator.py` (sliding-window dataset), `loss.py` (separate `ForceLoss`/`TorqueLoss` with per-axis weights), `normalization.py`, `visualization.py`.
- **The loader matches files by prefix**: `QuadDataset` walks `root_path` and keeps files starting with `<base_type>_` ([loader.py:41-49](NeuroBEM/code/Python/utils/loader.py#L41)). This is why every derived dataset folder must have its files renamed to its own prefix — hence `bem-slow_*.csv`, `none_*.csv`, `<ts>_*.csv`.
- **Feature set** is the `dataloading.use_*` flags: `use_pos`(3) / `use_att`(4) / `use_angvel`(3) / `use_linvel`(3) / `use_motors`(4) / `use_base_force`(3) / `use_base_torque`(3), summed into `feature_dim` at [settings.py:105-119](NeuroBEM/code/Python/config/settings.py#L105). The paper default (angvel+linvel+motors) gives `FL = 10`; the deployed ONNX/TensorRT shape is `(1, H, FL) = (1, 20, 10)`.
- **`dataloading.max_speed`** (new): `0` = off, otherwise drop every window whose full 50 ms span is not entirely below that body speed. Implemented as a `valid_mask` built in [loader.py:206-209](NeuroBEM/code/Python/utils/loader.py#L206) and applied to the start-index list in [window_generator.py:135-139](NeuroBEM/code/Python/utils/window_generator.py#L135) (no cross-gap splicing). Masked rows are also excluded from the normalization constants ([loader.py:257-260](NeuroBEM/code/Python/utils/loader.py#L257)). `run_inference` forces `max_speed = 0` so evaluation always sees the full envelope ([generate_ablation_study.py:20](NeuroBEM/code/Python/generate_ablation_study.py#L20)).
- **`hardware.use_gpu`** (new, default `True` when absent): `False` calls `tf.config.set_visible_devices([], 'GPU')` ([learner.py:26-28](NeuroBEM/code/Python/learner.py#L26)). On this Mac keep it **`False`** — Metal is ~2.7× slower than the CPU for this 28k-param model. Set it `True` for a CUDA node.
- **`window_generator.make_dataset` is a hand-rolled replacement for `keras.utils.timeseries_dataset_from_array`** ([window_generator.py:128](NeuroBEM/code/Python/utils/window_generator.py#L128)): one `tf.gather` per *batch* instead of per sample, columns pre-gathered into `[features | labels | infos]` order. Verified bitwise-identical to the keras path; do not "simplify" it back.
- **Nothing is seeded yet.** `make_dataset` draws its own shuffle seed ([window_generator.py:142](NeuroBEM/code/Python/utils/window_generator.py#L142)), so two runs of the same config differ by a few percent. Any n=1 comparison between runs is currently unfalsifiable — this is Phase 1 item 1.
- **`RNN` has no config block**: [nets.py:180](NeuroBEM/code/Python/utils/nets.py#L180) hardcodes `LSTM(12)`, and `settings.py` only parses `tcn`/`mlp`.
- **Evaluation**: [generate_ablation_study.py](NeuroBEM/code/Python/generate_ablation_study.py) loads `train_logs/<ts2>/best_model`, runs it over a folder **or a single CSV**, prints per-axis force/torque RMSE and writes per-signal `.png`/`.csv` to `--output_dir`. Its `run_inference(load_folder, data_root)` returns `(preds, labels, infos)` and is the shared engine of every evaluation notebook.
- **Deployment path** (trained `.pb` → ONNX): `python3 -m tf2onnx.convert --saved-model best_model/ --output best_model/network.onnx --inputs=input_1:0[BS,H,FL]` ([code/README.md:268](NeuroBEM/code/README.md#L268)); [verify_onnx.py](NeuroBEM/code/Python/verify_onnx.py) checks ONNX == TF on a ones-input `(1,20,10)`. Agilicious runs it via **ONNX Runtime**; a TensorRT path also exists (`.trt` engine, C++ check under [NeuroBEM/code/Python/trt/](NeuroBEM/code/Python/trt/)). ONNX/engines are machine-specific and gitignored.

## Model background

The whole framework computes one equation:

```
f = f_prop + f_res        τ = τ_prop + τ_res
```

A **rotor model** predicts `f_prop`/`τ_prop` (stages 1–2, C++); a **neural network** predicts the residuals (stage 3, Python). The *why* — BEM derivation, induced velocity, coning/flapping, the rotor-model variants and Table II — is in [research/sources/papers/RSS21_Bauersfeld.md](research/sources/papers/RSS21_Bauersfeld.md) and the code itself.

## Exploratory analysis (`NeuroBEM/analysis/`)

A separate Python workspace for data-quality analysis and BEM-baseline evaluation. Unlike the pipeline, these scripts read the **local, gitignored** `NeuroBEM/processed_data/` directly, so they only run where that data is present.

- [NeuroBEM/analysis/utils.py](NeuroBEM/analysis/utils.py) — shared loaders, noise metrics, and the **canonical column layout**.
  - `load_flight`/`load_largest_segment` read `processed_data/merged_*_seg_*.csv` (29-column merged order hard-coded in `COLUMNS`).
  - `BEM_DIR`/`BEM_PREFIX` ([utils.py:12-13](NeuroBEM/analysis/utils.py#L12)) currently point at **`processed_data/2026-07-15-00-00-00`** with prefix **`bem-01`**, not at `processed_data/bem` — `bem_flight_ids`/`load_bem_flight` follow them. **Gotcha:** `load_bem(bem_dir)` ([utils.py:95](NeuroBEM/analysis/utils.py#L95)) still globs a literal `bem_*.csv`, so it returns nothing for a folder whose prefix isn't `bem`. Check both before trusting a notebook that calls them.
  - `measured()`/`residuals()` compute force `= MASS·acc` and torque `= INERTIA·α + ω×INERTIA·ω` with `MASS = 0.772` (dataset README) and `INERTIA = [0.00254, 0.00214, 0.00436]` (`params.h`) — a deliberate mix, see the mass/inertia gotcha below. `FS = 400 Hz`; SNR uses a 4th-order **25 Hz Butterworth** low-pass.
  - Column layout: the current `bem-model` writes **35 columns** (29 merged + 6 predicted `fx,fy,fz,tx,ty,tz` at cols 29–34; `nCol + 6` in [simulator.cpp:22](NeuroBEM/code/simulator/src/simulator/simulator.cpp#L22)); `make_nn_targets.py` appends 6 residuals → **41 columns**, which is what the NN loader expects. `BEM_COLUMNS` names the 35-col layout, while `VI`/`MU`/`AS` ([utils.py:55-57](NeuroBEM/analysis/utils.py#L55)) and `diagnostics()` index a **47-col** layout (35 + 12 per-motor `vi,mu,alpha_s` at cols 35–46). Only `processed_data/bem-vi-baseline/` is 47-col — produced by the diagnostics-logging build (commit `d2fd23e`, `simulator.cpp` widened to `nCol + 18`). So `diagnostics` is live for `bem-vi-baseline`, stale for everything else.
- [NeuroBEM/analysis/measure_bem_RMSE.py](NeuroBEM/analysis/measure_bem_RMSE.py) — base-model residual RMSE (measured − predicted) over the full set and the `testset.txt` hold-out, in paper Table II format. Self-contained: `MASS = 0.772`, `INERTIA = params.h` values, args `--bem_dir` (default `processed_data/bem`) and `--testset` (default root `testset.txt`). Its `metrics()` ([measure_bem_RMSE.py:28](NeuroBEM/analysis/measure_bem_RMSE.py#L28)) is the shared `Fxy/Fz/F` formula that every notebook imports by file path.
- [NeuroBEM/analysis/speed_envelope.py](NeuroBEM/analysis/speed_envelope.py) — per-segment speed envelope, and builder of the two reduced prediction folders used by the generalization experiment: **`bem-slow`** (train segments with max body speed ≤ CUT, default 5 m/s — the paper's reduced set) and **`bem-ctrl`** (random full-envelope segments, row-count-matched to `bem-slow`, a control the paper never ran). Both also get the 13 `testset.txt` segments. Hard links from `processed_data/bem`, renamed to the `<base_type>_` prefix the loader needs. Run: `python3 analysis/speed_envelope.py [CUT]`.
- [NeuroBEM/make_nn_targets.py](NeuroBEM/make_nn_targets.py) — post-processes a `processed_data/<folder>` into NN residual targets: reads 35-col files, computes residuals with `MASS = 0.772`, `INERTIA = [0.0025,0.0021,0.0043]` (**README values, not `params.h`**), writes `[first 35 cols, 6 residuals]`. Idempotent (re-slices `d[:, :35]`).
- Notebooks: [feature_analyis.ipynb](NeuroBEM/analysis/feature_analyis.ipynb) (writes `noise_corpus.csv`), [pred_analysis.ipynb](NeuroBEM/analysis/pred_analysis.ipynb) (writes `pred_rmse.csv` / `pred_snr.csv` — per-flight BEM RMSE and SNR, committed so they can be diffed across tunes), [vi_analysis.ipynb](NeuroBEM/analysis/vi_analysis.ipynb), and **[generalization.ipynb](NeuroBEM/analysis/generalization.ipynb)** — the paper's `*` (reduced-training-set) experiment, described below.
- **Re-identification / CMA-ES workflow (advisor ask; lives in the `cmaes` C++ tool).** The active re-identifier is `code/simulator/src/CMAES.cpp` → the **`cmaes`** executable (a second CMake target alongside `bem-model`). It reads one flat data CSV (the same 29+ col merged/bem layout, `load()` reads cols 1–23), builds a **fleet of `Quadcopter`s (one per OpenMP thread)** and parallelizes the fitness **across samples** ([CMAES.cpp:185](NeuroBEM/code/simulator/src/CMAES.cpp#L185); thread count defaults to `omp_get_num_procs()`, overridable with `--threads N`), and via `Quadcopter::load` sweeps a **21-entry `REGISTRY`** ([CMAES.cpp:54](NeuroBEM/code/simulator/src/CMAES.cpp#L54)) of tunable params, each `{key, section, default, lo, hi}` (`section` = `bem`/`quad`/`body_drag`, used to group the emitted YAML), **ordered by optimization priority** — `lift_coefficient, drag_coefficient, hinge_spring_constant, lift_offset, hforce_scale, thrust_scale, pitch, twist, chord_inner, chord_outer, horizontal_drag_coefficient, vertical_drag_coefficient, radius, dx, dy, dz, frontarea_x/y/z`, then a **load-only tail** `num_blades, air_density` (kept at default → 19 actually tunable). `--cma MASK` frees the entries whose bit is `1` in the binary `MASK` (one bit per REGISTRY entry, left-aligned; e.g. `111` = `cl,cd,k`, trailing/unset bits stay fixed at default). The objective is **residual MSE normalized by the baseline (defaults) residual MSE** — `fm.sum()/sf² + tm.sum()/st²` ([CMAES.cpp:606](NeuroBEM/code/simulator/src/CMAES.cpp#L606)) — with `MASS = 0.772`, `INERTIA = params.h` values, and **`--loss force|torque|both`** selecting which terms are included (default `force`; 100 gens, `MAXGEN`). Config is **YAML in and out**: `./cmaes data.csv` (report at defaults), `./cmaes data.csv config.yaml` (report at loaded values), `./cmaes data.csv --cma MASK [--loss ...]` (run the fit; prints baseline → CMA-ES → best, writes `CMAES-results/<ts>/` containing `best.yaml` (sectioned), `convergence.csv`, `coeff.txt` (mask line + objective line + one freed-param name per line), and `metrics.csv` (best config's RMSE: `Fxy,Fz,F,Mxy,Mz,M` + one row)). The flat CSV it consumes is built by [NeuroBEM/CMAES-dataset/make_subset.py](NeuroBEM/CMAES-dataset/make_subset.py) → `CMAES-dataset/subset_20k.csv` (**committed**, 9 MB): pools all non-test flights from `processed_data/bem-vi-baseline/` (47-col), bins rows on a 6×6 quantile grid of per-row mean `(mu, vi)`, samples ~equally per cell (`SEED=0`, `N_TARGET=20000`).
- **[NeuroBEM/CMAES-results-analysis/](NeuroBEM/CMAES-results-analysis/)** — the reporting layer; three notebooks, all producing tables in **paper Table II** format (`Fxy, Fz, Mxy, Mz, F, M`) with the paper's numbers hardcoded for comparison.
  - `results_analysis.ipynb` — cross-run comparison of every `CMAES-results/<ts>/` fit: coefficient table (freed params highlighted, defaults from `processed_data/bem/params.yaml`), metrics table, convergence traces. `agi_coeffs.yaml` is the agilicious coefficient set, included as an extra column.
  - `table_testset_rmse.ipynb` — **base model only, no NN**: recomputes hold-out RMSE per `processed_data/` tune folder with the `measure_bem_RMSE` stack, plus best/worst-segment measured-vs-modeled plots.
  - `table2.ipynb` — **BEM vs BEM+NN**: the paper-Table-II reproduction. Its `MODELS` dict maps each label to a `(train_logs/<ts2>, data/<ts>/test)` pair — **add a row here after each training run**; that dict and the one in `generalization.ipynb` are the only record of which trained model came from which dataset.

### The generalization experiment (`analysis/generalization.ipynb`)

Reproduces the paper's `*` result (train on ≤ 5 m/s only, test on everything). Four arms — `full` (`data/bem`), `slow` and `ctrl` (built by `speed_envelope.py` → `bem-slow` / `bem-ctrl`), `win` (`dataloading.max_speed: 5` on `data/bem`) — plus a `None`-base arm from a `MODEL -1` build. `slow` vs `win` (reduced dataset vs. windowed filtering of the full one) is the open question.

## The main loop: tune → predictions → NN targets → split → train → tables

Everything below is keyed on one **run timestamp** `<ts>` (e.g. `2026-07-23-15-54-29`), which is simultaneously the `CMAES-results/<ts>/` folder, the `processed_data/<ts>/` prediction folder, the `data/<ts>/` split folder, and the file prefix `<ts>_<flight>_seg_X.csv`. Keep them identical — every step globs on it. The NN `train_logs/<ts2>` timestamp is a *different*, TF-generated one; the mapping run↔model lives only in the `MODELS`/`ARMS` dicts of the notebooks (and in the `settings.yaml` copied into each `train_logs/<ts2>/`).

1. **CMA-ES fit** (`cmaes`, optionally on MetaCentrum) → `CMAES-results/<ts>/{best.yaml,coeff.txt,metrics.csv,convergence.csv}`.
2. **Predictions**: `applyBM.sh processed_data/ <ts> CMAES-results/<ts>/best.yaml` → `processed_data/<ts>/<ts>_*_seg_*.csv`, 35 cols + `params.yaml`.
3. **NN targets**: `make_nn_targets.py processed_data/<ts>` → same files, 41 cols.
4. **Split**: `mkdir -p code/Python/data/<ts>/{train,validation,test}`, then `get_datafiles.bash ../../../processed_data <ts>` (hard links; `data/testset.txt` pins the 13-segment hold-out).
5. **Train**: set `base_type: "<ts>"` in `config/bem_settings.yaml`, run `train.py` → `train_logs/<ts2>/` with `best_model/`, `settings.yaml`, `nets.py`, `all.yaml` (normalization constants).
6. **Evaluate**: `generate_ablation_study.py` for per-signal plots/CSVs into `eval_out/<ts>_{train,test}`, and the notebooks for the tables.

`run_inference(load_folder, data_root)` returns `(preds, labels, infos)` where `labels` = the base-model residual (→ the **BEM** row) and `labels - preds` = the residual left after the NN (→ the **BEM+NN** row); `infos[:, 2:8]` is the base-model force/torque, so `labels + infos[:, 2:8]` reconstructs the measurement. `data_root` may be a folder **or a single CSV** — that is how per-segment plots are made.

## Common commands

Python runs in the `neurobem` conda env: `conda run -n neurobem python ...`.

### Build the C++ simulator
Requires GSL and Eigen (`sudo apt-get install libgsl-dev libeigen3-dev`). [NeuroBEM/code/simulator/build/](NeuroBEM/code/simulator/build/) is a pre-existing CMake configuration (the VS Code CMake extension points at `NeuroBEM/code/simulator` via `.vscode/settings.json`). To build from scratch:
```
cd NeuroBEM/code/simulator
mkdir build && cd build
cmake ..
make            # produces the `bem-model` and `cmaes` executables
```
CMake options (default ON): `ENABLE_FAST`, `UNSAFE_MATH`, `ENABLE_PARALLEL` (OpenMP), `EIGEN_FROM_SYSTEM`. C++17, `-march=native`. Two executables share the `simulator` library: **`bem-model`** ([src/simulator_node.cpp](NeuroBEM/code/simulator/src/simulator_node.cpp), pipeline stage 2) and **`cmaes`** ([src/CMAES.cpp](NeuroBEM/code/simulator/src/CMAES.cpp)). (`build/identify` is a stale binary from an older target — ignore it.)

### Re-identify aero params with CMA-ES
```
cd NeuroBEM/code/simulator/build
./cmaes DATA.csv                                  # report force/torque RMSE at defaults
./cmaes DATA.csv config.yaml                      # report RMSE at params from a YAML file
./cmaes DATA.csv --cma 111 --loss both            # fit cl,cd,k (mask bits 0-2) on force+torque → best.yaml
./cmaes DATA.csv --cma 111 --loss both --threads 8  # cap OpenMP threads (default: omp_get_num_procs)
```

### Run CMA-ES on the MetaCentrum PBS cluster
Batch-submit via [metacentrum/submit.py](NeuroBEM/code/simulator/metacentrum/submit.py) + `job.sh`; full walkthrough in [metacentrum/metacentrum.md](NeuroBEM/code/simulator/metacentrum/metacentrum.md) (the single authoritative guide). The binary is **built inside the job** on the compute node (`-march=native` + heterogeneous nodes), so the job first `module add cmake gcc gsl eigen` — Eigen is required (no cmake download fallback). One `(mask, loss)` pair → one `qsub` (jobs run in parallel).
```
python3 metacentrum/submit.py --dry-run                       # preview qsub, submit nothing
python3 metacentrum/submit.py                                 # full 19-param free, loss=both, 12 CPUs
python3 metacentrum/submit.py --mask 111 --loss force         # cl,cd,k force-only
```
`submit.py` requests `ncpus=ompthreads=N` (default 12, `--mem 8gb`, `--walltime 4:00:00`) and passes `NCPUS` into the job (used for `make -j`); `job.sh` does **not** pass `--threads`, so `cmaes` uses all allocated cores. `job.sh` copies each `CMAES-results/<ts>/` back to `$OUTDIR`. The full repo clone (`mrs-BEM`) and `subset_20k.csv` are already on MetaCentrum — no uploads needed.

### Apply the base model to flight data
```
cd NeuroBEM/code/Scripts
./applyBM.sh ../../processed_data/ 2026-07-23-15-54-29 ../../CMAES-results/2026-07-23-15-54-29/best.yaml
```
Signature is `./applyBM.sh BASEPATH MODEL CONFIG [filelist]` ([applyBM.sh:3](NeuroBEM/code/Scripts/applyBM.sh#L3)): `MODEL` is the **output subfolder name and file prefix** (convention: the CMA-ES run timestamp), `CONFIG` the params YAML. Writes `BASEPATH/MODEL/MODEL_<flight>_seg_X.csv` (35 cols) for every `BASEPATH/merged_*seg*.csv`, plus `params.yaml`. Optional 4th arg is a file listing flight IDs instead of globbing. **The binary must have been built with the right `#define MODEL`** — the CONFIG yaml cannot switch base model.

### Turn predictions into NN targets
```
cd NeuroBEM && conda run -n neurobem python make_nn_targets.py processed_data/2026-07-23-15-54-29
```
In-place 35 → 41 cols. Must run before the split — the loader expects the residual columns.

### Prepare the NN dataset (train/val/test split)
**This runs fine on this Mac** — the brew GNU tools are already installed, they just need to be first on `PATH`:
```
export PATH="/opt/homebrew/opt/coreutils/libexec/gnubin:/opt/homebrew/opt/findutils/libexec/gnubin:$PATH"
```
```
cd NeuroBEM/code/Python/data && ./get_datafiles.bash ../../../processed_data 2026-07-23-15-54-29
```
The script is GNU-only (`sort --random-source -R`, `head/tail --lines=-N`, bare `find -type f`, `mv -t`); BSD versions fail. Never claim it can't run on macOS — export the PATH above and run it.

`./get_datafiles.bash DATAPATH DATASET [NVAL]`, where `DATASET` is the subfolder under `DATAPATH` **and** the local split folder name — `data/DATASET/{train,validation,test}` must already exist (the script only wipes + refills them). Files are **hard-linked**, not copied ([get_datafiles.bash:47](NeuroBEM/code/Python/data/get_datafiles.bash#L47)). Split: seeded (`seed=42`) random shuffle, `NVAL` validation segments (default 49) + 1 test, rest train; then, if `data/testset.txt` exists, the whole test folder is moved back to train and only the 13 IDs listed there become the test set ([get_datafiles.bash:59](NeuroBEM/code/Python/data/get_datafiles.bash#L59)).

### Train / monitor / test the network
```
cd NeuroBEM/code/Python
conda run -n neurobem python train.py --settings_file config/bem_settings.yaml
tensorboard --logdir=train_logs            # checkpoints + best_model under train_logs/TIMESTAMP/
conda run -n neurobem python predict_from_pb.py --log_folder train_logs/TIMESTAMP/    # sanity-check (ones input)
conda run -n neurobem python generate_ablation_study.py --load_folder train_logs/TIMESTAMP --data_root data/DATASET/test --output_dir eval_out/DATASET_test    # per-axis RMSE vs paper + plots
conda run -n neurobem python verify_onnx.py --log_folder train_logs/TIMESTAMP/        # ONNX == TF on ones-input
```
`train.py` reads `base_type` from the settings YAML to pick `data/<base_type>/{train,validation,test}` — for a CMA-ES tune set it to the timestamp (`base_type: "2026-07-23-15-54-29"`), not `"bem"`. `--output_dir` must exist; convention is `eval_out/<DATASET>_test` / `_train`.

**Timing:** ~31 s/epoch × 120 epochs ≈ 70 min with `use_gpu: False` on this Mac. Run long trainings under `caffeinate -is` — a locked macOS demotes the process to efficiency cores and it drops ~30×.

Per-batch profiling (producer vs normalization vs `train_step`):
```
conda run -n neurobem python profile_train_step.py data/bem/train/SOME_SEG.csv cpu
```

### Base-model baseline RMSE
```
cd NeuroBEM && conda run -n neurobem python analysis/measure_bem_RMSE.py --bem_dir processed_data/bem
```

### MATLAB
No CLI build/test harness — scripts are run inside MATLAB. Every function is documented; use `help functionName` (e.g. `help mergefile` prints the segment-CSV column order). Shared helpers live in `Matlab/Common/` and per-area `subroutines/` folders.

## Conventions & gotchas

- **There is no test suite anywhere** — no pytest, no CTest, no MATLAB test harness. Verification is: `predict_from_pb.py` / `verify_onnx.py` (ones-input sanity checks), `generate_ablation_study.py` (RMSE vs the paper), and the committed notebook tables. Do not go looking for tests to run; say so instead of inventing a command.
- **NO TEMP DIRECTORIES, NO SCRATCH FILES.** Never write to `/tmp`, `/private/tmp/claude-*`, or any scratchpad. Everything must be reproducible: either put the script in the repo where it belongs, or don't write it at all and just describe what you would run.
- **MINIMAL COMMENTS AND DOCSTRINGS.** Keep comments and docstrings to an absolute minimum — code should be self-explanatory. Keep the code itself as simple as possible.
- **ALWAYS CHECK THE SOURCE, ALWAYS CITE IT.** Never answer factual questions about the code/data (what a value is, where a signal comes from, how something is computed) from memory, the summary, or inference — open the actual file and verify. Every claim must come with a source the user can open: `file:line` (e.g. [MergeAndPreprocessData.m:277](NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L277)), not a vague reference. Trace data provenance to the exact line that assigns/derives it.
- **Flight-file naming is load-bearing.** All artifacts for one flight share a base ID (typically a timestamp like `2021-02-03-13-43-38`) with different prefixes/suffixes/extensions: `merged_<ID>_seg_X.csv`, `bem_<ID>_seg_X.csv`, `<ID>_traj.csv`, `<ID>.BFL`, etc. Scripts glob on these patterns and the NN loader matches on the `<base_type>_` prefix, so renames break the pipeline.
- **Multiple mass/inertia values are in play — cite the right one.**
  | where | mass | inertia |
  |---|---|---|
  | C++ simulator ([params.h:169](NeuroBEM/code/simulator/include/params.h#L169), `:164`) | 0.752 | [0.00254, 0.00214, 0.00436] |
  | `CMAES.cpp` + `analysis/utils.py` + `measure_bem_RMSE.py` | 0.772 | [0.00254, 0.00214, 0.00436] |
  | dataset `README.md` / paper / `make_nn_targets.py` / `bem_settings.yaml` | 0.772 | [0.0025, 0.0021, 0.0043] |
  | **MyBEM** — [configs/drones/paper_quad.yaml](MyBEM/configs/drones/paper_quad.yaml), the only place it is defined (C++ `Drone` **and** `mybem/drone.py` read it) | 0.772 | [0.00254, 0.00214, 0.00436] |

  So the NN **labels** are computed with the README inertia while the **RMSE reports** use the `params.h` inertia. Small, but it means a number from `measure_bem_RMSE.py` is not bitwise the same quantity as the label the NN fits. Check which a given script uses before comparing.
- BEM coning/flapping angles use the *linear* lift/drag coefficients (`param.a`/`param.d`); changing the *nonlinear* `param.cl`/`param.cd` will not move those angles. This is intentional (tractability), and it means the CMA-ES retunes never reach the flapping/`kβ` torque path.
- `Coning.m` is described in the README as fragile/"a hack" (FFT of audio to recover prop RPM) — handle with care.
- **Batches never mix flights.** The dataset is ~170 per-file pipelines `concatenate`d ([loader.py:63-67](NeuroBEM/code/Python/utils/loader.py#L63)) with shuffling only *within* each file, so every batch comes from one flight. Known real issue, deliberately deferred (fixing it changes the gradient sequence → separate experiment).
- Gitignored outputs you should not commit: `NeuroBEM/processed_data/`, `NeuroBEM/raw_data/`, `NeuroBEM/bem+nn/`, `NeuroBEM/pdf/`, **`NeuroBEM/CMAES-results/`**, `Python/train_logs/`, `Python/data/2026-*/`, `Python/data/bem-*/`, `Python/data/none/`, `Python/eval_out/`, `simulator/build/`, `Matlab/tmp/`, `*.trt`, `*.asv`, plus (root `.gitignore`) `/data/`, `MyBEM/store/`, `MyBEM/cpp/build/`, `**/__pycache__/`. Note `CMAES-dataset/subset_20k.csv` **is** committed. The notebooks under `CMAES-results-analysis/` and `analysis/` **are** committed with their outputs — that is where results are recorded, since the raw `CMAES-results/` folders are not.
