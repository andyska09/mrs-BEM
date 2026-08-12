# Feature ablation of the residual network

Status: run, 7 arms × 3 seeds. Runs after [Architectures.md](Architectures.md); the
architecture stays the paper TCN regardless of how that one turns out. The
per-axis version of the same question is [Axis-ablation.md](Axis-ablation.md).

## Question

Which of the paper's three input groups actually carries the residual?

## What goes in today

`features: [angvel, linvel, motors]`, 10 dims × 20 timesteps
([tcn_baseline.yaml:8](../../MyBEM/configs/nets/tcn_baseline.yaml#L8)):

| group | dims | cols | quantity |
|---|---|---|---|
| `angvel` | 3 | 4–6 | body rates ω [rad/s] |
| `linvel` | 3 | 14–16 | body-frame velocity [m/s] |
| `motors` | 4 | 20–23 | rotor speeds [rad/s] |

In the CSV but unused: position, attitude quaternion, motor-speed derivatives,
battery voltage, and the base model's own predicted wrench. Adding any of them is
a separate experiment — this one only removes.

## Setup

Frozen at [tcn_baseline.yaml](../../MyBEM/configs/nets/tcn_baseline.yaml).
Only `features:` changes, driven by
[sweeps/features.yaml](../../MyBEM/configs/sweeps/features.yaml). Input dimension
changes with the arm, so parameter count is not constant — the comparison is
"same architecture, less information", not "same capacity".

7 arms × 3 seeds. `full` is `arch_tcn` from the architecture sweep —
byte-identical config, already trained — so 18 new jobs.

| arm | `features:` | dims |
|---|---|---|
| full (`arch_tcn`) | `[angvel, linvel, motors]` | 10 |
| `feat_no_angvel` | `[linvel, motors]` | 7 |
| `feat_no_linvel` | `[angvel, motors]` | 7 |
| `feat_no_motors` | `[angvel, linvel]` | 6 |
| `feat_angvel` | `[angvel]` | 3 |
| `feat_linvel` | `[linvel]` | 3 |
| `feat_motors` | `[motors]` | 4 |

## Results

`--on test`, mean ± std over seeds 0/1/2. RMSE, force in N, torque in Nm. `val`
is comparable across arms here because every arm shares `preds: bem_default`.

| arm | params | val | Fxy | Fz | F | Mxy | Mz | M |
|---|---|---|---|---|---|---|---|---|
| full (`arch_tcn`) | 27814 | 0.487 | **0.179**±0.002 | 0.431±0.020 | **0.288**±0.010 | **0.0086**±0.0003 | **0.0023**±0.0000 | **0.0072**±0.0002 |
| `feat_no_angvel` | 26470 | 0.858 | 0.207±0.000 | **0.428**±0.013 | 0.299±0.006 | 0.0151±0.0004 | 0.0057±0.0001 | 0.0128±0.0003 |
| `feat_no_linvel` | 26470 | 1.018 | 0.219±0.001 | 0.779±0.002 | 0.484±0.001 | 0.0125±0.0001 | 0.0025±0.0000 | 0.0103±0.0000 |
| `feat_no_motors` | 26022 | 0.857 | 0.190±0.002 | 0.485±0.008 | 0.320±0.003 | 0.0276±0.0002 | 0.0072±0.0002 | 0.0229±0.0001 |
| `feat_angvel` | 24678 | 3.210 | 0.339±0.001 | 1.248±0.003 | 0.772±0.002 | 0.0733±0.0007 | 0.0104±0.0001 | 0.0602±0.0006 |
| `feat_linvel` | 24678 | 1.612 | 0.245±0.002 | 0.534±0.003 | 0.367±0.001 | 0.0312±0.0002 | 0.0089±0.0002 | 0.0260±0.0002 |
| `feat_motors` | 25126 | 2.189 | 0.305±0.001 | 1.039±0.012 | 0.649±0.006 | 0.0263±0.0001 | 0.0060±0.0001 | 0.0218±0.0001 |
| | | | | | | | | |
| `bem_default` (base) | — | — | 0.575 | 1.664 | 1.069 | 0.1273 | 0.0151 | 0.1043 |
| paper BEM+NN | — | — | 0.204 | 0.504 | 0.335 | 0.0140 | 0.0040 | 0.0120 |

Cost of removing one group, against `full`:

| dropped | ΔF | ΔM |
|---|---|---|
| `angvel` | +4 % | +78 % |
| `motors` | +11 % | +218 % |
| `linvel` | +68 % | +43 % |

## Findings

- **Force and torque are carried by different groups.** `linvel` owns the force
  residual (+68 % F without it); `motors` owns the torque residual (+218 % M
  without it). Neither dominates the other overall — the two channels do not
  share a driver.
- **The force residual is the vertical channel, and it is velocity.** Dropping
  `linvel` moves `Fz` 0.431 → 0.779 (+81 %) while `Fxy` moves only 0.179 → 0.219
  (+22 %). Dropping `angvel` leaves `Fz` untouched (0.428 ± 0.013, inside the
  seed spread of `full`'s 0.431 ± 0.020). That is the axial-inflow signature the
  base model is missing, and it is the direct motivation for
  [Axis-ablation.md](Axis-ablation.md).
- **`angvel` is free on force, expensive on torque.** ΔF is +4 %, i.e. 0.299 ±
  0.006 against 0.288 ± 0.010 — overlapping within a standard deviation, so by
  this report's own criterion ω is irrelevant to the force residual. ΔM is +78 %.
- **Yaw is a rotor effect, not a velocity effect.** `Mz` is 0.0023 at full and
  0.0025 without `linvel` — no change. Without `motors` it is 0.0072 (3.1×),
  without `angvel` 0.0057 (2.5×). Consistent with `Mz` being rotor drag torque
  and spin-up, which velocity does not observe.
- **No proper subset reaches `full`.** The best pair (`feat_no_angvel`) is within
  4 % on force but 1.8× worse on torque. All three groups are load-bearing
  together, so the paper's feature set is not redundant.
- **Single groups are not competitive** but rank as expected: `linvel` alone is
  the best force model (F 0.367), `motors` alone the best torque model (M
  0.0218), `angvel` alone is worst on both and improves the base model's `Fz` by
  only 25 % (1.248 vs 1.664).
- **Val loss does not rank the arms on test.** `feat_no_motors` (0.857) and
  `feat_no_angvel` (0.858) tie on val but differ by 1.8× on test M. The loss is a
  weighted MSE with `axis_weight_force: [1, 1, 3]`, so it is not the quantity
  these tables report.
- Seed spread is small everywhere — ≤0.020 on F, ≤0.0004 on M — so every delta
  above except `angvel`-on-force is real.

## Caveats

- Parameter count is not held constant (27 814 down to 24 678); part of every
  delta is lost capacity, not lost information. The largest gap is 11 %, against
  RMSE deltas up to 218 %, so it does not change any conclusion here.
- `bem_default` is untuned, so these residuals are larger than they would be on a
  CMA-ES-tuned base ([CMAES-identification.md](CMAES-identification.md)). Which
  group carries the residual may shift once the base absorbs more of it.
