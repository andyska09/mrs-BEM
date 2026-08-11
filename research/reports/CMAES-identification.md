# CMA-ES re-identification of the BEM parameters

Status: **run, results below.** Old stack (`NeuroBEM/`), `cmaes` binary.

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
