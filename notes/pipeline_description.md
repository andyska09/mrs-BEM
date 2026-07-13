# NeuroBEM pipeline (new drone)

NeuroBEM = BEM rotor physics + NN residual: `f = f_prop + f_res`, `τ = τ_prop + τ_res`.

## Need
- Motion-capture volume (Vicon/OptiTrack), pose @ ~400 Hz.
- Betaflight FC with bi-directional DSHOT (logs motor speeds).
- Thrust test stand + a coning video (with audio) for prop params.
- Drone mass, diagonal inertia, prop geometry.

## Steps
1. **Identify BEM params.** Thrust-stand run → `QuadraticFit.m` (cl_q, cd_q). Coning video → `Coning.m` (kβ). Fit → `ParameterID.m` (cl, cd, kβ). Put geometry+values in `Matlab/BEM/subroutines/setParam.m` and `simulator/include/params.h` (mass, inertia, cl/cd/k, motor τ).
2. **Fly & record.** Rosbag (mocap pose) + Betaflight `.BFL` (motor speeds) + trajectory csv, per flight, shared FlightID.
3. **Merge/process.** `Matlab/OptiTrack/MergeAndProcessData.m` → `merged_<ID>_seg_X.csv` (splines fuse+differentiate mocap+IMU; airborne segments only).
4. **Apply base model.** Build C++ `bem-model`, run `Scripts/applyBM.sh` → per-row predicted `f_prop/τ_prop`. Residual label = `measured − predicted` (`measured f = m·a`, `τ = J·α + ω×Jω`).
5. **Train NN.** Split (`Python/data/get_datafiles.bash`, 70/20/10), `train.py --settings_file config/bem_settings.yaml`. Input 20×10 (linvel, body rates, 4 motors) @2.5 ms; output 6 (residual f+τ), two heads. Target: single-step RMSE (paper Table I/II).
6. **Export.** `tf2onnx` `.pb → network.onnx`; ship normalization constants (`means/stds_in/out` from `all.yaml`). Verify ONNX == TF on ones-input.
7. **Deploy.** In simulator per 1 ms tick: low-level ctrl → 1st-order motor → BEM+NN (ONNX Runtime/TensorRT, normalize→infer→un-normalize→add to BEM) → symplectic-Euler integrate. Validate closed-loop tracking (paper Table III).

Per-drone re-tune: BEM params (step 1), motor τ, Betaflight PID + battery/motor-speed model (`Matlab/LowLevelController/`).

## Repo files
- `code/Matlab/` — BEM identification + flight-data merge (stages 1–3).
- `code/simulator/` — C++ `bem-model`: applies BEM, writes predicted f/τ (stage 4).
- `code/Python/` — NN train/eval/export (stages 5–6): `train.py`, `generate_ablation_study.py` (RMSE), `verify_onnx.py`, `config/bem_settings.yaml`, `utils/` (net/loader/loss/norm).
- `code/README.md` — authoritative end-to-end tutorial.
- `RSS21_Bauersfeld.md` — the paper (source of truth for the model).
- `Readme.md`, `Flights.txt`, `testset.txt`, `bem_output_columns.md` — dataset docs, flight catalog, test hold-out, CSV column layouts.
- `processed_data/` — merged flight CSVs + `bem/` predictions (gitignored data).
- `analysis/` — my EDA + residual-RMSE scripts (`utils.py`, `make_nn_input.py`).
- `my_bem/` — Agilicious-style C++ sim (target for deployment, stage 7).
- `current_goal.md` — active objective (port into Agilicious, reproduce Table III).
