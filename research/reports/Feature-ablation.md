# Feature ablation of the residual network

Status: proposed, not run. Runs after [Architectures.md](Architectures.md); the
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

Reported as test RMSE delta against `full`, mean ± std over seeds. A feature is
called irrelevant only when its removal costs less than the seed spread.

```
conda run -n mybem python -m mybem.sweep features.yaml
```
```
python3 scripts/metacentrum/submit.py train --exp-glob 'generated_features/*.yaml' --seeds 0 1 2
```
```
conda run -n mybem python -m mybem.report --on test
```

`max_speed` stays 0. The arms without `linvel` leave `Data.speed` at `None`
([data.py:93](../../MyBEM/mybem/data.py#L93)), so a nonzero speed cut would crash
them in `windows()`.

## Results

Not run.
