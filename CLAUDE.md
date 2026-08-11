# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## IMPORTANT NOTE TO ALL AI CHATBOTS

When talking to me start the message with "TARS:". When writing answer always keep it concise and information dense. But do not skip important stuff. When I shout and curse at you DO NOT apologize, it wastes tokens, just follow orders. Next when writing code DO NOT write stupid comments and docstrings. Keep the code clean and high quality. When implementing stuff KEEP IT SIMPLE. This important. I do not want to read 1000 lines of diffs, I want to look at the change and know what it does.
Also IDE is wrongly configured, so it will report wrong include errors etc. So do not worry about it. Only investigate when neccessary. 

When listing commands to me, one command one line. If there is more than one command or it is super duper complex i will punish you. HARD.

**DO WHAT I ASKED. NOTHING ELSE.** Touch only the files the current request is about. If you spot something else worth changing — a duplication, a bug, a better place for the code — **say it in one line and wait.** Do not "fix" it. A file I did not name is off limits, and noticing a problem is not permission to solve it.

**WHEN I ASK A QUESTION, ANSWER THE QUESTION.** A question is not a task. If I ask "why did you say X", the reply is a sentence explaining why — not a tool call, not a rewrite, not a demonstration. Do not treat a question as a complaint and do not treat a complaint as a work order. If you think I am wrong, say so and stop; I will decide.

**RESPECT A STOP.** "stop", "revert", "no", "wait" means stop immediately at the current tool call. Do not finish the thought, do not clean up first, do not squeeze in one more edit. Report what state things are in and wait.

**READ THE LENGTH SIGNALS.** If I say it is too long, the next answer is shorter — not a shorter preamble followed by the same wall of text. When I say "I was just asking", the answer is one or two sentences.

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

The root [README.md](README.md) is a short MyBEM overview + the four core commands, then a pointer at the CMA-ES analysis notebooks. It duplicates the MyBEM section below — keep the two in sync when the commands change. This repo is a workspace of four loosely-coupled parts:

- **[MyBEM/](MyBEM/)** — **the active rebuild.** A clean-break reimplementation of the whole pipeline as independent, composable stages with explicit artifacts. Spec in [MyBEM/DESIGN.md](MyBEM/DESIGN.md); C++ base model + PyTorch residual net both exist. See the MyBEM section below.
- **[NeuroBEM/](NeuroBEM/)** — the **frozen** paper reference: the NeuroBEM framework, my exploratory analysis, the CMA-ES notebooks, the dataset docs, and the gitignored flight data. **Everything about it — pipeline, commands, gotchas — is in [NeuroBEM/AGENTS.md](NeuroBEM/AGENTS.md); read that before touching anything under `NeuroBEM/`.**
- **[agilicious/simulator/](agilicious/simulator/)** — extracted Agilicious C++ simulator sources (models incl. `model_propeller_bem*.cpp` + `bem/`, the BetaFlight/simple low-level controllers, `quadrotor_simulator.cpp`), the **deployment target** for the final closed-loop sim. Reference source only — no build system or headers checked in here.
- **[research/](research/)** — reading material and write-ups. Two folders, nothing else: no index, no schema, no maintenance skills. Conceptual questions are answered from `sources/` + the code with a `file:line` citation.
  - **[research/sources/](research/sources/)** — inputs. **Never edit a file here**; adding a new one is fine. [papers/](research/sources/papers/) holds each paper as PDF + a grep-able PDF→text transcription, **no headings — cite them by line number**: [RSS21_Bauersfeld.md](research/sources/papers/RSS21_Bauersfeld.md) (NeuroBEM), [scirobotics.abm6597.md](research/sources/papers/scirobotics.abm6597.md) (Neural-Fly), [Quadrotor_Gray_box_Model_Identification_from_High_Speed_Flight_Data.md](research/sources/papers/Quadrotor_Gray_box_Model_Identification_from_High_Speed_Flight_Data.md) (Sun/de Visser/Chu — the polyfit model), [WaveNet_1609.03499.md](research/sources/papers/WaveNet_1609.03499.md) (dilated convs, cited by the `dtcn` arch). [notes/](research/sources/notes/) is my own loose notes: explorations, open questions, notes on the code under `NeuroBEM/`.
  - **[research/reports/](research/reports/)** — findings: concise, information-dense, no fluff. **Write one only when I ask** — never file a report unprompted. `CMAES-identification.md` and `Generalization.md` are still empty placeholders; [Architectures.md](research/reports/Architectures.md) and [Feature-ablation.md](research/reports/Feature-ablation.md) are written-up **experiment designs with "Results: not run"** — a report may exist before its numbers do, so check the Status line before quoting one.

## MyBEM — the rebuild (active work)

A clean-break reimplementation of the pipeline. `NeuroBEM/` stays frozen as the paper reference and the fallback/cross-check; nothing is imported from it except the merged flight CSVs. **[MyBEM/DESIGN.md](MyBEM/DESIGN.md) is the spec** — §1 (four artifact kinds), §5 (CLI), §6 (C++ architecture), §10 (decisions/non-goals). §10 "OPEN" is empty: the spec is settled, don't relitigate it.

**What exists today: both halves.** C++ — two binaries (`mybem-apply`, `mybem-tune`) + the `mybem_sim` library, ~2450 lines under [MyBEM/cpp/](MyBEM/cpp/). Python — the `mybem/` package (~1300 lines, **PyTorch**, not TF): [paths.py](MyBEM/mybem/paths.py), [columns.py](MyBEM/mybem/columns.py), [metrics.py](MyBEM/mybem/metrics.py), [drone.py](MyBEM/mybem/drone.py), [data.py](MyBEM/mybem/data.py), [train.py](MyBEM/mybem/train.py), [eval.py](MyBEM/mybem/eval.py), [sweep.py](MyBEM/mybem/sweep.py), [report.py](MyBEM/mybem/report.py), [polyfit/](MyBEM/mybem/polyfit/), [nets/](MyBEM/mybem/nets/). Still **not written**: the single `mybem` CLI of DESIGN §5 (run modules directly, `python -m mybem.train`). The sweep/run-table of §4.6/§7.7 now exists in a simpler form than DESIGN describes — `sweep.py` writes one experiment yaml per run instead of a SWEEP artifact, `report.py` is the run table, and there is no `metrics.json`. DESIGN's status table also lists a `scripts/apply.sh` that does not exist — the only scripts are [parse_flights.py](MyBEM/scripts/parse_flights.py) and [metacentrum/](MyBEM/scripts/metacentrum/). **The `@hash6` of DESIGN §2.2 is now on all three artifact kinds**: nets ([train.py:92-95](MyBEM/mybem/train.py#L92)), cluster tune runs ([submit.py:23-26](MyBEM/scripts/metacentrum/submit.py#L23)), and preds — `mybem-apply` writes `PREDSDIR/<name>@<Model::hash()>/` ([apply.cpp:108-109](MyBEM/cpp/src/apps/apply.cpp#L108)), the 6-hex sha256 of the fully-resolved config text ([model.cpp:120](MyBEM/cpp/src/model.cpp#L120)). `Component::diagnose()`/`diagnostics()` ([component.h:21-22](MyBEM/cpp/include/mybem/component.h#L21)) are declared and never called. **`MyBEM/DESIGN.md` is still untracked** (`??` in git status), and it is stale on polyfit ("interface ready, not written" — both halves are written and committed) and on the flat `store/preds/<name>/` layout.

### The object model

Two type aliases are the whole trick that killed the `#define`s ([types.h:13-15](MyBEM/cpp/include/mybem/types.h#L13)): `Params = map<string,double>` (everything CMA-ES can move) and `Options = map<string,string>` (structural — polar form, chord law, on/off — never tuned).

- **`Component`** ([component.h:10](MyBEM/cpp/include/mybem/component.h#L10)) — an additive `+=` contributor, shaped like `agi::ModelBase`: `add(State, Airframe, Wrench&)`, `load(Params, Options)`, `params()`, `tunables()`. `tunables()` returns `{key, def, lo, hi}` and is the **per-component replacement for the old global 21-entry `REGISTRY`**.
- **`Model`** ([model.cpp](MyBEM/cpp/src/model.cpp)) — an `Airframe` + an ordered `vector<ComponentPtr>`; `evaluate()` is a three-line sum ([model.cpp:108](MyBEM/cpp/src/model.cpp#L108)). Loading is **strict**: any unknown key `exit(1)`s ([model.cpp:71](MyBEM/cpp/src/model.cpp#L71)). Numeric YAML values route to `Params`, non-numeric to `Options`. Duplicate component types get suffixed prefixes (`bem`, `bem_2`), and `tunables()`/`values()`/`set()` namespace every key as `bem.lift_coefficient`, `airframe.dx` ([model.cpp:114-160](MyBEM/cpp/src/model.cpp#L114)).
- **`Airframe`** ([types.h:56](MyBEM/cpp/include/mybem/types.h#L56)) — `dx,dy,dz,thrust_scale` + `offsets()` and the hardcoded spin pattern `{CW,CCW,CCW,CW}`. Not a component; it is what components act on. Its params are tunable as `airframe.*`.
- **`Drone`** ([drone.h:9](MyBEM/cpp/include/mybem/drone.h#L9)) — mass + inertia, **the only place they are defined**, loaded from `configs/drones/*.yaml`. `Drone::measured()` is the sole measured-force/torque conversion. This kills the three-inconsistent-mass/inertia-sets problem of the old stack (see the gotcha table below).
- **`PropellerModel`** ([propeller_model.cpp](MyBEM/cpp/src/models/propeller_model.cpp)) — base for anything per-rotor; owns the rotor loop, kinematics (`vel = v_body + ω × offset`), and force/torque assembly. Subclasses supply only `thrust`/`torque`/`hforce`/`inducedVelocity`/`hasFlapping`. `add()` scales **only `force[2]`** by `thrust_scale`, *after* the moment arms used unscaled thrust ([propeller_model.cpp:40](MyBEM/cpp/src/models/propeller_model.cpp#L40)) — matches the original ordering.
- **Component registry** — six types, one `if` each ([registry.cpp:8](MyBEM/cpp/src/registry.cpp#L8)): `bem`, `quadratic`, `none`, `body_drag`, `motor_reaction`, `polyfit`. Adding one = one file + one line (+ one line in `CMakeLists.txt`).
- **`polyfit`** ([polyfit.cpp](MyBEM/cpp/src/models/polyfit.cpp)) — the Sun/de Visser/Chu vehicle-level gray-box model, and the one component that is **not** a `PropellerModel`: it produces the whole-airframe wrench in one shot from dimensionless states (`mu_*`, `p/q/r_bar`, `u_p/u_q/u_r`, `nu_in`) and a per-|beta|-bin polynomial. Coefficients are **not** in the model yaml — `coeffs:` is an `Options` path to a `store/polyfit/<name>/coeffs.txt` written by the Python identifier ([polyfit/__main__.py:40-45](MyBEM/mybem/polyfit/__main__.py#L40)); only `radius`, `air_density`, `ct_hover`, `ref_length` are `Params`/tunable. **DESIGN.md still says polyfit is "interface ready, not written" — that is stale**; both halves are written and committed.
- `BEMModel` resolves `polar`/`chord`/`distortion` strings to function pointers in `load()` ([bem.cpp:56-75](MyBEM/cpp/src/models/bem.cpp#L56)); `GSLParams` carries them as plain fn pointers so the hot path inside the nested `qags` gets an indirect call, not a virtual one. One `GSLHelper` **per rotor**, lazily ([bem.h:29-31](MyBEM/cpp/include/mybem/models/bem.h#L29)) — deliberate, since the helper permanently widens its bracket window after a failed bracketing and a shared one would couple the rotors.
- BEM numerics (`gsl_helper.cpp`, `integrands.cpp`) and the three flapping polynomials (`coning.cpp`, `longitudinal_flapping.cpp`, `lateral_flapping.cpp`) are the **unchanged** Maple-generated math, restructured into free functions over `PropState`. The `cl`/`cd`-baked-into-the-constants limitation is stated at [flapping.h:7-9](MyBEM/cpp/include/mybem/bem/flapping.h#L7).

### Config = the model

`configs/models/bem_default.yaml`: root scalars (`name`, `drone`), an ordered `models:` list of additive components, and an `airframe:` block. `models: [bem]`, `[quadratic, body_drag]`, `[bem, motor_reaction, body_drag]` — all config lines, **no rebuild**. `yaml.cpp` is a hand-rolled ~90-line parser accepting exactly that one schema; anything else is a hard error with `file:line`. `Model::save` round-trips losslessly, emitting the fully-resolved config including defaults you never wrote.

### Commands

Build (needs GSL + Eigen + OpenMP; on macOS the CMakeLists auto-points at `brew --prefix libomp` since AppleClang ships no runtime, and a missing OpenMP is a **fatal error** unless you pass `-DENABLE_PARALLEL=OFF`. Options: `ENABLE_FAST`, `UNSAFE_MATH`, `ENABLE_PARALLEL`, `EIGEN_FROM_SYSTEM`/`EIGEN_ALTERNATIVE` ON by default; **`ENABLE_NATIVE` OFF** — `-march=native` under gcc 9.2 + Eigen 3.3.7 on MetaCentrum's znver2 nodes corrupts the heap and aborts `mybem-tune` with `double free or corruption (out)`, for ~10% speed):
```
cmake -S MyBEM/cpp -B MyBEM/cpp/build && cmake --build MyBEM/cpp/build
```
Apply a model (`MODEL.yaml INPUT PREDSDIR`; INPUT is one `merged_*_seg_*.csv` or a folder of them, output goes to `PREDSDIR/<name from the config>/<id>.csv` + `params.yaml`, existing outputs are skipped so re-running resumes, `OMP_NUM_THREADS` segments at a time):
```
MyBEM/cpp/build/mybem-apply MyBEM/configs/models/bem_default.yaml data/processed_data MyBEM/store/preds
```
List a model's tunable names, report RMSE at the loaded values, and fit ([tune.cpp:320](MyBEM/cpp/src/apps/tune.cpp#L320)):
```
MyBEM/cpp/build/mybem-tune MODEL.yaml DATA.csv --list
MyBEM/cpp/build/mybem-tune MODEL.yaml DATA.csv --drone configs/drones/paper_quad.yaml
MyBEM/cpp/build/mybem-tune MODEL.yaml DATA.csv --drone configs/drones/paper_quad.yaml --free lift_coefficient,drag_coefficient,hinge_spring_constant --loss both --out RUNDIR
```
`--free NAMES|all` replaces the old `--cma MASK` bit string; a bare name is accepted when exactly one component offers it, otherwise write `bem.lift_coefficient` ([tune.cpp:263](MyBEM/cpp/src/apps/tune.cpp#L263)). `--free` requires `--out`, `--drone` is always required. Other flags: `--gens` (100), `--seed` (0), `--threads`, `--loss force|torque|both` (**default `force`**, [tune.cpp:237](MyBEM/cpp/src/apps/tune.cpp#L237)). Output dir gets `model.yaml` (a full config — feed it straight back to `mybem-apply` or resume a tune from it), `convergence.csv`, `metrics.csv` (baseline + best rows), `tune.yaml` (run record).

### MetaCentrum

All three stages run as PBS jobs from **one** `submit.py` + **one** `job.sh` under [MyBEM/scripts/metacentrum/](MyBEM/scripts/metacentrum/); [metacentrum.md](MyBEM/scripts/metacentrum/metacentrum.md) is the guide. Every list argument multiplies out into separate jobs. `apply`/`tune` build the binary inside the job in `$SCRATCHDIR` (`-march=native`, heterogeneous nodes) and get `ncpus=ompthreads=32`; `train` gets `-q gpu`, `ngpus=1`, and `--device cuda`. Everything writes straight to `MyBEM/store/` on shared home — no copy-back, and `apply` skips finished segments so a killed job resumes.
```
python3 scripts/metacentrum/submit.py apply --config configs/models/bem_default.yaml
```
```
python3 scripts/metacentrum/submit.py tune --free all --loss force both
```
```
python3 scripts/metacentrum/submit.py train --exp tcn_baseline.yaml --seeds 0 1 2
```
```
python3 scripts/metacentrum/submit.py train --exp-glob 'generated/arch_*.yaml' --seeds 0
```
Add `--dry-run` to print the `qsub` lines without submitting. Resources are overridable per submit: `--ncpus --mem --walltime` (`--gpu-mem` for `train`). `--exp-glob` is how a sweep is submitted — it globs under `configs/nets/` and errors if nothing matches ([submit.py:119-122](MyBEM/scripts/metacentrum/submit.py#L119)).

### mybem-apply / mybem-tune I/O

Both read a `merged_*_seg_X.csv` (≥29 cols, header, FLU) and take exactly the slices they need — `mybem-apply` reads angvel@4, linvel@14, motors@20, motor-accel@24 plus col 0 ([apply.cpp:21-25](MyBEM/cpp/src/apps/apply.cpp#L21)); `mybem-tune` additionally reads angacc@1 and acc@11 to build the measured wrench ([tune.cpp:28-34](MyBEM/cpp/src/apps/tune.cpp#L28)). Position and attitude are never read, because no component uses them. Internals are **FRD**; `flu2frd`/`frd2flu` convert at the boundary. Output is 7 columns, no header, `%.12g`: `t, fx, fy, fz, tx, ty, tz` (FLU).

CMA-ES is a from-scratch (mu/mu_w, lambda) implementation ([cma.cpp](MyBEM/cpp/src/tune/cma.cpp)) that knows nothing about the objective — bounds and scaling live in `SearchSpace` ([tune.cpp:147](MyBEM/cpp/src/apps/tune.cpp#L147)), which centres x-space on the **loaded model's current values**, not the hardcoded defaults, so a tune resumes from its own output. Loss is per-term MSE normalized by the baseline MSE, so the starting objective is exactly 1 per active term ([tune.cpp:386](MyBEM/cpp/src/apps/tune.cpp#L386)).

### Behavioral deltas vs. the old `bem-model` — do not treat these as bugs

- **Output shape**: 7 cols (t + 6), not the old 35 (29 passthrough + 6). There is no 41-col file and no `make_nn_targets.py` equivalent — [data.py:62-69](MyBEM/mybem/data.py#L62) rejoins prediction to merged CSV positionally (row-count + `t`-column check) and computes residuals **in RAM, never written**.
- **Precision**: `%lf` (6 decimal places) → `%.12g`. On ~1e-3 Nm torques the old writer kept ~3 significant digits. Numbers will differ from `processed_data/bem/`, and **the new ones are correct** — the acceptance-test caveat in DESIGN §9.4.
- **`none` is now genuinely zero.** Old `MODEL -1` skipped thrust but still ran the flapping angles, emitting a hinge-spring torque at zero thrust. `NoneModel::hasFlapping()` returns false ([simple.h:21](MyBEM/cpp/include/mybem/models/simple.h#L21)). The None-base generalization arm changes.
- **`motor_reaction` can actually fire.** The original read `domega` but never called `setMotorAcceleration`, so the term was dead; [apply.cpp:70](MyBEM/cpp/src/apps/apply.cpp#L70) fills `s.dmot` for real. It is absent from `bem_default.yaml`, so the default stays faithful — add it and you get a term the old pipeline never had.
- **`body_drag` is opt-in**; the old code had no off switch.
- **No caching.** The `_valid`/`_validv1` dirty flags are gone; every `evaluate` recomputes.
- **Parallelism moved.** The rotor loop is `omp parallel for` ([propeller_model.cpp:26](MyBEM/cpp/src/models/propeller_model.cpp#L26)), but both apps parallelize a level above it with one `Model` per thread and `omp_set_max_active_levels(1)`, which collapses that rotor loop to serial: `mybem-tune` across *samples* ([tune.cpp:354](MyBEM/cpp/src/apps/tune.cpp#L354)), `mybem-apply` across *segments* ([apply.cpp:129](MyBEM/cpp/src/apps/apply.cpp#L129)). Inside one segment the row loop stays sequential **on purpose** — `GSLHelper` carries its bracket window across rows ([apply.cpp:63](MyBEM/cpp/src/apps/apply.cpp#L63)).

### The Python half (`MyBEM/mybem/`, PyTorch)

No CLI wrapper, no packaging (`python -m mybem.<mod>` from `MyBEM/`). Env is **`mybem`**, not `neurobem` — [environment.yml](MyBEM/environment.yml), python 3.14, **everything from pip on purpose**: conda-forge numpy links llvm-openmp, the torch wheel bundles its own libomp, and two OpenMP runtimes in one process abort on import.

Three leaf modules hold the constants, so nothing else hardcodes them and nothing has to import torch to use them:

- **[paths.py](MyBEM/mybem/paths.py)** — every filesystem location: `CONFIGS/DRONES/SPLITS/EXPERIMENTS/SWEEPS`, `DATA = <repo>/data/processed_data`, `STORE = MyBEM/store` with `PREDS`/`NETS`/`POLYFIT` under it. `store/tune/` is written by `mybem-tune` and has no constant here.
- **[columns.py](MyBEM/mybem/columns.py)** — merged-CSV column indices (`ANGACC`, `ANGVEL`, `ATT` reordered to w,x,y,z, `ACC`, `LINVEL`, `POS`, `MOTORS`, `VBAT`) plus `load(sid)` → the raw float array.
- **[metrics.py](MyBEM/mybem/metrics.py)** — the paper's Table II numbers ([RSS21_Bauersfeld.md:437-443](research/sources/papers/RSS21_Bauersfeld.md#L437)) and the `rmse`/`table` formula. Numpy only.

- **[drone.py](MyBEM/mybem/drone.py)** — `Drone.load(name)` from `configs/drones/*.yaml`. `Drone.measured()` is the only measured-wrench conversion on the Python side; mass/inertia live nowhere else.
- **[data.py](MyBEM/mybem/data.py)** — `split(name, ids)` reads `configs/splits/<name>.yaml`: pinned id lists come out first, the rest is shuffled once with `seed` and sized folds (`{n:}` or `{frac:}`) are cut off it; **train is whatever is left**, and an unknown pinned id is a hard error. The feature groups are fixed at [data.py:13](MyBEM/mybem/data.py#L13) — `pos, att, angvel, linvel, motors` — with **no `base_force`/`base_torque` group**, so the old `use_base_*` feature flags have no MyBEM equivalent, and **no per-axis groups** (`angvel_xy`, `linvel_z`, …) — the per-axis arms of [Feature-ablation.md](research/reports/Feature-ablation.md) need those added first. `Data` loads all selected segments into one array plus per-segment `bounds`; `windows(history, max_speed)` returns start indices that never cross a segment boundary and (when filtering) whose *full span* is under the speed cut; `normalization(max_speed)` excludes masked rows. `preds_dir(name)` ([data.py:50-59](MyBEM/mybem/data.py#L50)) resolves a `store/preds` folder by exact name **or** by the unique `<name>@hash` applied from it — an ambiguous prefix is a hard error, so `preds: bem_default` in an experiment yaml keeps working as long as only one hash of it exists. `read_preds` rejoins prediction to merged CSV positionally with a row-count + `t`-column check.
- **[nets/](MyBEM/mybem/nets/)** — registry `{tcn, mlp, dtcn, lstm, linear}` ([nets/__init__.py:12](MyBEM/mybem/nets/__init__.py#L12)); one file + one line adds an architecture. Contract: `(batch, history, features) -> (batch, 6)`, force then torque. `two_heads: true` = the paper's two independent stacks. **`TimeConv`** ([tcn.py:13-27](MyBEM/mybem/nets/tcn.py#L13)) is `unfold + Linear`, not `nn.Conv1d` — identical arithmetic and param count, but ~0.01 ms vs ~2 ms per call on CPU at these shapes. Don't "simplify" it back.
- **[train.py](MyBEM/mybem/train.py)** — one experiment yaml in, `store/nets/<name>@<hash>/{model.pt, config.yaml, normalization.yaml, tb/}` out. `config.yaml` is the experiment **plus the resolved `segments:` fold lists**, so a run records its own split; `hash` is the 6-hex digest of exactly that, and a second `group:` digest of it **minus `seed`** is what lets `report.py` collapse seeds of one experiment into one row ([train.py:92-95](MyBEM/mybem/train.py#L92)). `--device`/`--threads` are popped before hashing — machine settings are not part of the experiment ([train.py:65-67](MyBEM/mybem/train.py#L65)). Seeded end to end (`cfg["seed"]` → `torch.manual_seed` + the batch-shuffle generator) — unlike the old TF stack. `cosine_restarts` ([train.py:25-33](MyBEM/mybem/train.py#L25)) reimplements `tf.keras.experimental.CosineDecayRestarts`. Loss = per-axis weighted MSE on **normalized** residuals. Best-val-loss checkpoint only. `torch.set_num_threads(1)` by default — at ~28k params multithreading tiny ops costs more than it saves.
- **[eval.py](MyBEM/mybem/eval.py)** — scores **any number of models** in paper Table II format (`Fxy Fz F Mxy Mz M`) with the paper's rows underneath. A model argument is a **folder name, hash included**: `resolve()` ([eval.py:23-26](MyBEM/mybem/eval.py#L23)) checks `store/nets/<name>/model.pt` first (base + residual net), else falls through to `preds_dir` (a base model alone). Nets need the full `<name>@<hash>`; preds folders accept the bare name. All models are scored on the **intersection of rows every one of them predicts** ([eval.py:78](MyBEM/mybem/eval.py#L78)) — a net leaves NaN on the first `history-1` rows of each segment — so `mybem.eval bem_default arch_lstm_64@d1ef8d` is an apples-to-apples comparison. Evaluation always uses the full speed envelope ([eval.py:45](MyBEM/mybem/eval.py#L45)).
- **[sweep.py](MyBEM/mybem/sweep.py)** — `configs/sweeps/<name>.yaml` (a `base:` experiment + a list of named `runs:` overrides) → one generated experiment yaml per run in `configs/nets/generated/`, and prints the `--exp` line to paste into `submit.py`. It is a **deep merge except `net:`, which is replaced wholesale** ([sweep.py:35-36](MyBEM/mybem/sweep.py#L35)) — leftover keys of another architecture would otherwise survive. No cartesian product: every run is written out by hand in the sweep yaml.
- **[report.py](MyBEM/mybem/report.py)** — the cross-run table over `store/nets/`, grouped by `config.yaml`'s `(name, group)` so seeds of one experiment collapse into one row and two *different* configs that reused a name are still separated (printed as `name~group`). Prints params, val loss, and RMSE as mean ± std, with a row per distinct `preds:` base model and the paper rows underneath. `--match` filters by name substring, `--quick` skips the evaluation and prints val loss only.
- **[polyfit/](MyBEM/mybem/polyfit/)** — identifies the polyfit gray-box model: `terms.py` (candidate term list per axis), `physics.py` (`Geometry`, the dimensionless states, `hover_ct`), `stepwise.py` (forward-backward selection on a **cross-product (Gram) matrix accumulated over segments**, so the N×895 design matrix is never materialized), `fit.py` (`identify`/`predict`/`score`/`bin_edges`), `__main__.py` (the CLI). Follows the authors' MATLAB release, **not** the paper's printed Algorithm 1 (misprinted). Writes `store/polyfit/<name>/{polyfit.yaml, coeffs.txt}`; `coeffs.txt` is what the C++ `polyfit` component loads.

Configs that exist: `models/{bem_default,none,polyfit_paper}.yaml`, `drones/paper_quad.yaml`, `splits/paper.yaml` (13 pinned test ids + `val: {frac: 0.21}`), `nets/tcn_baseline.yaml` + `nets/generated/*` (sweep output, regenerable — don't hand-edit), `sweeps/arch.yaml`, and **[configs/flights.csv](MyBEM/configs/flights.csv)** — one committed row per segment (`id, flight, seg, family, vel, twr, ccw, comment`, 247 rows) regenerated by [scripts/parse_flights.py](MyBEM/scripts/parse_flights.py) from [NeuroBEM/Flights.txt](NeuroBEM/Flights.txt). Run-once and hand-editable, **not a pipeline stage**; `family` comes from a first-match-wins keyword list ([parse_flights.py:15-27](MyBEM/scripts/parse_flights.py#L15)). **Nothing reads it yet** — the `{family: cpc}` split selection of DESIGN §7 is unimplemented; `split()` only does pinned lists + `{n:}`/`{frac:}` folds.

Commands (from `MyBEM/`):
```
conda run -n mybem python -m mybem.train configs/nets/tcn_baseline.yaml
```
```
conda run -n mybem python -m mybem.eval bem_default arch_tcn@1b7066 --on test
```
```
conda run -n mybem python -m mybem.sweep arch.yaml
```
```
conda run -n mybem python -m mybem.report --on test --match arch_
```
```
conda run -n mybem python -m mybem.polyfit --name polyfit_paper --bins 3
```
`train.py` flags override the yaml: `--seed`, `--epochs`, `--limit N` (first N segments per fold — the smoke test), plus `--device cpu|cuda|mps` and `--threads` (neither enters the config hash). **There is no `--name`** — rename in the experiment yaml. `eval`/`report` also take `--device`, `--split`, `--drone`, `--on train|val|test`. `polyfit` takes `--name`, `--split`, `--drone`, `--bins`, `--limit` and prints train/test Table II rows at the end.

**Artifact names carry a hash.** `store/nets/<name>@<hash>/` (train.py) and `store/preds/<name>@<hash>/` (mybem-apply) always; `store/tune/<loss>_{all|<k>p}@<hash>/` only for cluster runs, since a local `mybem-tune --out` writes wherever you point it ([submit.py:24-26](MyBEM/scripts/metacentrum/submit.py#L24), [job.sh:35](MyBEM/scripts/metacentrum/job.sh#L35)). `store/polyfit/<name>/` is the one that carries no hash. So `ls store/nets` is how you find the argument for `mybem.eval` — a bare experiment name will not resolve for a net.

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

## Model background

The whole framework computes one equation:

```
f = f_prop + f_res        τ = τ_prop + τ_res
```

A **rotor model** predicts `f_prop`/`τ_prop` (stages 1–2, C++); a **neural network** predicts the residuals (stage 3, Python). The *why* — BEM derivation, induced velocity, coning/flapping, the rotor-model variants and Table II — is in [research/sources/papers/RSS21_Bauersfeld.md](research/sources/papers/RSS21_Bauersfeld.md) and the code itself.

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
- Gitignored outputs you should not commit: `/data/`, `MyBEM/store/`, `MyBEM/cpp/build/`, `**/__pycache__/`, plus everything listed under Gotchas in [NeuroBEM/AGENTS.md](NeuroBEM/AGENTS.md).
