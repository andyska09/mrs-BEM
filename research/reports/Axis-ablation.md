# Per-axis ablation of the residual network

Status: proposed, not run, blocked on code. Sharper follow-up to
[Feature-ablation.md](Feature-ablation.md), which removes whole groups; this one
splits a group by axis.

## Question

Is the residual dominated by the vertical channel?

BEM's own error is anisotropic — `Fz` RMSE 1.265 N against `Fxy` 0.803 N
([RSS21_Bauersfeld.md:437](../sources/papers/RSS21_Bauersfeld.md#L437)) — so the
residual should be too. If so, removing `v_z` costs far more than removing
`v_xy`, and the same asymmetry shows for `ω_z` against `ω_xy`. If not, the
residual is not the missing axial-inflow physics it is assumed to be.

## Blocker

`FEATURES` ([data.py:13](../../MyBEM/mybem/data.py#L13)) only holds whole groups
— `pos, att, angvel, linvel, motors`. The per-axis arms need `angvel_xy`,
`angvel_z`, `linvel_xy`, `linvel_z` added as their own column lists first.

`Data.speed` ([data.py:93](../../MyBEM/mybem/data.py#L93)) keys off `"linvel" in
features`, so a `linvel_xy`/`linvel_z` arm would silently lose the speed filter;
it should be computed from the `LINVEL` columns regardless of feature selection.

## Setup

Frozen at [tcn_baseline.yaml](../../MyBEM/configs/nets/tcn_baseline.yaml), same
as the group ablation. 4 arms × 3 seeds = 12 jobs, plus the shared `full`
(`arch_tcn`, already trained).

| arm | `features:` | dropped |
|---|---|---|
| full (`arch_tcn`) | `[angvel, linvel, motors]` | — |
| `axis_no_angvel_z` | `[angvel_xy, linvel, motors]` | ω_z |
| `axis_no_angvel_xy` | `[angvel_z, linvel, motors]` | ω_xy |
| `axis_no_linvel_z` | `[angvel, linvel_xy, motors]` | v_z |
| `axis_no_linvel_xy` | `[angvel, linvel_z, motors]` | v_xy |

Reported as test RMSE delta against `full`, mean ± std over seeds, read per
output axis (`Fxy Fz Mxy Mz`) rather than only the totals — the asymmetry is the
whole point.

## Results

Not run.
