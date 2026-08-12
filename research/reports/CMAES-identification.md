# CMA-ES re-identification of the BEM parameters

Status: **run, results below.** Fits were done on the old stack (`NeuroBEM/`,
`cmaes` binary); the three carried-forward tunes were then ported to MyBEM and
re-evaluated there — last section.

## Setup

Fit data: [CMAES-dataset/subset_20k.csv](../../NeuroBEM/CMAES-dataset/subset_20k.csv)
— ~20k rows sampled on a 6×6 quantile grid of per-row mean `(mu, vi)` from all
non-test flights. Optimizer: `cmaes` ([CMAES.cpp](../../NeuroBEM/code/simulator/src/CMAES.cpp)),
100 generations, `--loss both`, objective = residual MSE normalized by the
defaults' residual MSE, one term per wrench half, so **loss at defaults = 2.0**
([CMAES.cpp:606](../../NeuroBEM/code/simulator/src/CMAES.cpp#L606)).
`MASS = 0.772`, `INERTIA = [0.00254, 0.00214, 0.00436]`.

13 runs total; three were carried forward to full predictions + NN training.
All three post-date the switch to the normalized loss (`2026-07-22-12-36-31`),
so their loss numbers are comparable to each other and to 2.0.

## The three tunes

| | run id | freed | fit loss (gen 99) |
|---|---|---|---|
| **c1** | `2026-07-23-11-57-11` | `lift_coefficient, drag_coefficient, lift_offset, hforce_scale, thrust_scale` (5) | 0.932 |
| **c2** | `2026-07-22-18-39-25` | all 19 | 1.126 |
| **c3** | `2026-07-23-15-54-29` | all 19 **except** `lift_offset, hforce_scale, thrust_scale` (16) | 0.529 |

c1 is the minimal "thrust-scaling" set; c2 is everything free; c3 is c2 with the
three empirical scale/offset knobs pinned at their defaults.

### Coefficients

| param | default | c1 | c2 | c3 |
|---|---|---|---|---|
| `lift_coefficient` | 15.242 | **2.179** | **6.607** | **12.307** |
| `drag_coefficient` | 13.549 | **26.515** | **12.431** | **13.625** |
| `hinge_spring_constant` | 5.89 | 5.89 | **13.387** | **11.607** |
| `lift_offset` | 0 | **0.2137** | **0.2295** | 0 |
| `hforce_scale` | 1 | **0.7436** | **0.9515** | 1 |
| `thrust_scale` | 1 | **1.6193** | **1.1655** | 1 |
| `pitch` | 21.77 | 21.77 | **19.402** | **28.578** |
| `twist` | −11 | −11 | **−19.198** | **−2.228** |
| `chord_inner` | 0.017 | 0.017 | **0.01124** | **0.01020** |
| `chord_outer` | 0.007 | 0.007 | **0.002908** | **0.006441** |
| `radius` | 0.06477 | 0.06477 | **0.06973** | **0.05277** |
| `dx` | 0.078 | 0.078 | **0.08524** | **0.04662** |
| `dy` | 0.1 | 0.1 | **0.05220** | **0.06578** |
| `dz` | 0.027 | 0.027 | **0.02301** | **−0.02922** |
| `horizontal_drag_coefficient` | 1 | 1 | **2.937** | **0.6804** |
| `vertical_drag_coefficient` | 1 | 1 | **0.7751** | **0.6613** |
| `frontarea_x` | 0.0054 | 0.0054 | **0.003331** | **0.010171** |
| `frontarea_y` | 0.009 | 0.009 | **0.007583** | **0.011553** |
| `frontarea_z` | 0.006 | 0.006 | **0.001882** | **0.003415** |

Bold = freed in that run. `num_blades` (3) and `air_density` (1.204) are
load-only, never tuned.

## Results — base model alone, 13-segment hold-out

RMSE in paper Table II format, `processed_data/<run>` vs. the measured wrench
([table_testset_rmse.ipynb](../../NeuroBEM/CMAES-results-analysis/table_testset_rmse.ipynb),
215 829 rows per config).

| config | Fxy | Fz | Mxy | Mz | F | F impr% | M | M impr% |
|---|---|---|---|---|---|---|---|---|
| `bem` (defaults) | 0.575 | 1.663 | 0.127 | 0.015 | 1.069 | +0.0% | 0.104 | +0.0% |
| **c1** | 0.626 | 1.086 | 0.084 | 0.025 | 0.809 | +24.3% | 0.070 | +33.0% |
| **c2** | 0.829 | 1.006 | 0.088 | 0.009 | 0.892 | +16.6% | 0.072 | +30.6% |
| **c3** | 0.413 | 0.899 | 0.040 | 0.029 | **0.619** | **+42.1%** | **0.036** | **+65.2%** |
| paper BEM | 0.803 | 1.265 | 0.090 | 0.017 | 0.982 | +8.1% | 0.074 | +29.0% |

Improvement is relative to the untuned `bem` baseline. All three beat the
defaults on both F and M; c3 is best on both and is the only tune that also
beats the paper's own BEM row on every column except `Mz`.

## Results — BEM + residual TCN

Each tune got the full pipeline (predictions → NN targets → split → train) with
an identical NN config (paper TCN, two heads, features `angvel+linvel+motors`,
history 20, 120 epochs); only `base_type` differs.
[table2.ipynb](../../NeuroBEM/CMAES-results-analysis/table2.ipynb).

| config | Fxy | Fz | Mxy | Mz | F | M |
|---|---|---|---|---|---|---|
| BEM+NN (defaults) | 0.193 | 0.547 | 0.009 | 0.002 | 0.353 | 0.008 |
| **c1**+NN | 0.201 | 0.541 | 0.008 | 0.005 | 0.353 | 0.007 |
| **c2**+NN | 0.201 | 0.375 | 0.007 | 0.002 | **0.272** | 0.006 |
| **c3**+NN | 0.186 | 0.430 | 0.004 | 0.003 | 0.291 | **0.004** |
| paper BEM+NN | 0.204 | 0.504 | 0.014 | 0.004 | 0.335 | 0.012 |

Train logs: c1 `20260723-181423`, c2 `20260724-140022`, c3 `20260804-110251`,
baseline `20260708-223500`.

## Observations

- **A better base model is not proportionally a better hybrid.** c1 gains 24% on
  F as a base model and 0% after the NN. c2 is the *worse* base model of c2/c3
  yet gives the best F after the NN (0.272 vs 0.291).
- **The NN absorbs most of the difference.** Spread across the four base models
  is 1.069 → 0.619 N in F (42%) and collapses to 0.353 → 0.272 N (23%) after the
  residual net; on M, 0.104 → 0.036 (65%) collapses to 0.008 → 0.004.
- **All four hybrid variants beat the paper's BEM+NN on torque** (0.004–0.008 vs
  0.012), and c2/c3 beat it on force as well.
- **c3's parameters are not physical.** `dz = −0.029` flips the rotor plane below
  the CoG, `radius` shrinks 19%, `pitch` grows to 28.6°. CMA-ES is fitting an
  effective model, not identifying geometry. c2 is likewise off (`dy` halved,
  `horizontal_drag_coefficient` ≈ 2.9).
- **c1 drives `lift_coefficient` to 2.18 and compensates with
  `thrust_scale = 1.62`** — the two are near-degenerate on the force axis, which
  is why freeing the scale knobs (c1, c2) buys less than freeing the geometry
  (c3).
- `hinge_spring_constant` moves 5.89 → ~12–13 in both runs that free it, but the
  flapping angles use the *linear* `a`/`d` coefficients, so the retunes never
  reach that torque path.
- `Mz` gets *worse* in c1 and c3 (0.015 → 0.025 / 0.029) while `Mxy` improves;
  the combined M still improves because `Mxy` dominates.

## Port to MyBEM

The three tunes were transcribed into MyBEM model configs; no refit was run.
Names now carry the number of freed parameters instead of c1/c2/c3:

| old | MyBEM config | freed |
|---|---|---|
| c1 | [bem_f5.yaml](../../MyBEM/configs/models/bem_f5.yaml) | 5 |
| c3 | [bem_f16.yaml](../../MyBEM/configs/models/bem_f16.yaml) | 16 |
| c2 | [bem_f19.yaml](../../MyBEM/configs/models/bem_f19.yaml) | 19 |

The old `best.yaml` sections map onto the MyBEM schema 1:1 — `bem:` → the `bem`
component, `quad:` → `airframe:`, `body_drag:` → the `body_drag` component. Two
keys had to be added because they were compile-time in the old stack: `polar:
sin_cos` and `chord: linear`, which are what `MODEL 1` selected
([params.h:33-35](../../NeuroBEM/code/simulator/include/params.h#L33)).

Transcription check: `mybem-tune` on the same `subset_20k.csv` reproduces each
run's original `metrics.csv` to 4 significant digits (F/M — f5 0.6710/0.05481,
f16 0.5992/0.03053, f19 0.7748/0.05661).

### Base model, MyBEM test split

Same 13 held-out segments, scored by `mybem.eval`, 215 829 rows per config.
Improvement is relative to the untuned `bem_default`, as in the old-stack table.

| config | Fxy | Fz | F | F impr% | Mxy | Mz | M | M impr% |
|---|---|---|---|---|---|---|---|---|
| `bem_default` | 0.575 | 1.663 | 1.069 | +0.0% | 0.1272 | 0.0151 | 0.1043 | +0.0% |
| `bem_f5` | 0.626 | 1.086 | 0.809 | +24.3% | 0.0837 | 0.0254 | 0.0699 | +33.0% |
| `bem_f16` | 0.413 | 0.899 | **0.619** | **+42.1%** | 0.0395 | 0.0285 | **0.0362** | **+65.3%** |
| `bem_f19` | 0.829 | 1.006 | 0.892 | +16.6% | 0.0884 | 0.0087 | 0.0724 | +30.6% |
| paper BEM | 0.803 | 1.265 | 0.982 | +8.1% | 0.0900 | 0.0170 | 0.0740 | +29.0% |

Every row reproduces the old-stack table above at its printed precision, so the
two pipelines agree on the base model despite the different output precision
(`%.12g` vs `%lf`) and the inertia used for the measured wrench.

### BEM + residual TCN, 3 seeds

Identical net config across arms ([tcn_baseline.yaml](../../MyBEM/configs/nets/tcn_baseline.yaml):
paper TCN, two heads, `angvel+linvel+motors`, history 20, 120 epochs); only
`preds:` differs. Mean ± std over seeds 0/1/2, 27 814 parameters each. The
untuned arm is `arch_tcn`, the same config under the name it carries in the
[architecture sweep](Architectures.md).

| config | val | Fxy | Fz | F | Mxy | Mz | M |
|---|---|---|---|---|---|---|---|
| `arch_tcn` (untuned base) | 0.487 | 0.179±0.002 | 0.431±0.020 | 0.288±0.010 | 0.0086±0.0003 | 0.0023±0.0000 | 0.0072±0.0002 |
| `tcn_f5` | 0.753 | 0.188±0.002 | 0.401±0.010 | 0.278±0.005 | 0.0076±0.0002 | 0.0051±0.0001 | 0.0068±0.0002 |
| `tcn_f16` | 1.039 | 0.179±0.000 | 0.380±0.014 | **0.264±0.006** | 0.0041±0.0001 | 0.0025±0.0000 | **0.0037±0.0001** |
| `tcn_f19` | 0.695 | 0.191±0.002 | 0.404±0.016 | 0.280±0.009 | 0.0064±0.0001 | 0.0021±0.0000 | 0.0053±0.0001 |
| paper BEM+NN | — | 0.204 | 0.504 | 0.335 | 0.0140 | 0.0040 | 0.0120 |

### What changed against the old-stack hybrid numbers

- **The base-model ranking now survives the NN, exactly.** On F the order is
  identical before and after: f16 < f5 < f19 < default (0.619 / 0.809 / 0.892 /
  1.069 as bases, 0.264 / 0.278 / 0.280 / 0.288 after). On the old stack c2
  (=f19) gave the best hybrid F despite being the worse base model; that
  inversion was a single unseeded TF run per arm and does not reproduce with 3
  seeds.
- **Every arm is better than its old-stack counterpart** (F 0.264–0.280 vs
  0.272–0.353). The training stacks differ in two known ways: MyBEM is seeded end
  to end, and its batches mix segments, while the old loader shuffled only within
  one flight.
- **Val loss does not rank the arms.** `tcn_f16` has the worst val loss (1.039)
  and the best test RMSE. The loss is MSE on residuals normalized per base model,
  so each arm divides by its own residual std — the numbers are comparable across
  seeds of one arm, never across arms.
- **The spread between base models still collapses.** Best to worst, F is 0.619
  → 1.069 (+73 %) before the net and 0.264 → 0.288 (+9 %) after; M is 0.0362 →
  0.1043 (+188 %) before and 0.0037 → 0.0072 (+95 %) after. Torque keeps more of
  the difference than force.
- **The untuned base costs little after the net.** `arch_tcn` is last at F
  0.288±0.010, but only 9 % behind `tcn_f16` (0.264±0.006) from a base model with
  73 % more force error. Torque keeps more of the gap: M 0.0072 vs 0.0037, `Mxy`
  0.0086 vs 0.0041. Tuning the base buys a factor of 1.7 on the base model and
  1.1 on the hybrid.
