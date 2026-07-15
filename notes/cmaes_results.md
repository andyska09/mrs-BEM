# CMA-ES aero fit — results

Re-fit BEM aero coefficients from flight data. Force-only normalized MSE objective,
2-D over (cl, cd) with k fixed. Fit on the regime-stratified 20k subset, test excluded.

- **default:** cl=15.2421, cd=13.5489, k=5.89 (params.h)
- **tuned:**   cl=14.329,  cd=14.454,  k=5.89

RMSE via `analysis/measure_bem_RMSE.py` (mass 0.772, inertia [0.00254, 0.00214, 0.00436]).

## Full dataset (1,798,481 rows)

| params  | Fx | Fy | Fz | F total | Tx | Ty | Tz | T total |
|---------|------|------|------|---------|--------|--------|--------|---------|
| default | 0.5620 | 0.4652 | 1.5484 | 1.7117 | 0.10408 | 0.12462 | 0.0130 | 0.16289 |
| tuned   | 0.5642 | 0.4664 | 1.4383 | 1.6138 | 0.10146 | 0.12128 | 0.0132 | 0.15868 |

## Test set (13 held-out segments, 215,829 rows)

| params  | Fx | Fy | Fz | F total | Tx | Ty | Tz | T total |
|---------|------|------|------|---------|--------|--------|--------|---------|
| default | 0.6156 | 0.5308 | 1.6634 | 1.8513 | 0.11012 | 0.14232 | 0.0151 | 0.18058 |
| tuned   | 0.6187 | 0.5322 | 1.6123 | 1.8071 | 0.10735 | 0.13859 | 0.0154 | 0.17598 |

Force [N], torque [Nm]. Total = RMSE of the 3-axis magnitude.

## Notes

- Win is z-force (thrust): −7.1% full, −3.1% test — expected from lowering cl ~6%.
- x/y force flat; torque improved slightly despite not being in the objective.
- Total force RMSE: −5.7% full, −2.4% test. Test improved though it was excluded from
  the fit → generalizes, no overfit.
- cd is not identifiable from force alone (drag is a torque param); its fitted value is
  incidental. cl is the real lever.
