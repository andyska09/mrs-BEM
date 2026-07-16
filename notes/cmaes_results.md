# CMA-ES aero fit — results

Re-fit BEM aero coefficients from flight data. Force-only normalized MSE objective,
2-D over (cl, cd) with k fixed. Fit on the regime-stratified 20k subset, test excluded.

- **default:** cl=15.2421, cd=13.5489, k=5.89 (params.h)
- **tuned:**   cl=14.329,  cd=14.454,  k=5.89

RMSE via `analysis/measure_bem_RMSE.py` (mass 0.772, inertia [0.00254, 0.00214, 0.00436]).
Paper Table II format: `Fxy=√((Fx²+Fy²)/2)`, `Fz` per-axis; `F=√((Fx²+Fy²+Fz²)/3)`; same for M.

## Full dataset (1,798,481 rows)

| params  | Fxy | Fz | Mxy | Mz | F | M |
|---------|------|------|--------|--------|------|--------|
| default | 0.516 | 1.548 | 0.1148 | 0.0130 | 0.988 | 0.0940 |
| tuned   | 0.518 | 1.438 | 0.1118 | 0.0132 | 0.932 | 0.0916 |

## Test set (13 held-out segments, 215,829 rows)

| params  | Fxy | Fz | Mxy | Mz | F | M |
|---------|------|------|--------|--------|------|--------|
| default | 0.575 | 1.663 | 0.1272 | 0.0151 | 1.069 | 0.1043 |
| tuned   | 0.577 | 1.612 | 0.1240 | 0.0154 | 1.043 | 0.1016 |

Force [N], torque [Nm].

## Notes

- Win is z-force (thrust): −7.1% full, −3.1% test — expected from lowering cl ~6%.
- x/y force flat; torque improved slightly despite not being in the objective.
- Total force RMSE: −5.7% full, −2.4% test. Test improved though it was excluded from
  the fit
- cd is not identifiable from force alone (drag is a torque param); its fitted value is
  incidental. cl is the real lever.