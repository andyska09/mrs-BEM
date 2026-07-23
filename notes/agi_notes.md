# BEM tuning results

Metrics (paper Table II): `Fxy=√((Fx²+Fy²)/2)`, `Fz` per-axis; `F=√((Fx²+Fy²+Fz²)/3)`; same for torque M.
Ground truth = `f=m·a`, `τ=I·α+ω×Iω` with mass 0.772, inertia [0.00254,0.00214,0.00436].


## Old folders

| folder | files | cols | cl | cd | k | extra | purpose |
|---|---|---|---|---|---|---|---|
| `bem` | full | 35 | 15.242 | 13.549 | 5.89 | — | canonical baseline, default identified params (params.h) |
| `bem-007` | test | 35 | 15.242 | 13.549 | 5.89 | +0.07 lift offset | failed: +0.07 on the large default cl over-thrusts |
| `bem-vi-baseline` | full | 47 | 15.242 | 13.549 | 5.89 | +12 per-motor vi/mu/as diag | same as `bem`; source for CMA-ES subset binning |
| `bem-agi` | test | 35 | 4.797 | 4.169 | 5.89 | +0.07 offset, ×3 h-force | agilicious recipe; thrust_scale (~1.30 on Fz) NOT baked |


## Agilicious simulator additions

Four empirical corrections the agilicious BEM applies over paper BEM.
All are constant linear scales or a fixed offset.

| # | addition | where | effect |
|---|---|---|---|
| 1 | re-identified coeffs: cl 15.242→**4.797**, cd 13.549→**4.169** (k=5.89 same) | [params.cpp:19](../agilicious/simulator/model_propeller_bem_params.cpp#L19) | ~3× smaller lift/drag |
| 2 | `+0.07` lift offset: `cl·(sinα·cosα + 0.07)` | [functions.cpp:35](../agilicious/simulator/bem/functions.cpp#L35) | baseline lift at all α → more thrust |
| 3 | h-force **×3.0** (per prop, pre-assembly) | [bem.cpp:87](../agilicious/simulator/model_propeller_bem.cpp#L87) | triples in-plane drag; fixes Fxy (commented "BEM underestimates drag") |
| 4 | `force.z() ×= thrust_scale_` (value unknown, total body-z, post-sum) | [bem.cpp:123](../agilicious/simulator/model_propeller_bem.cpp#L123) | scales thrust; fixes Fz |
| 5 | remeasured propeller angles: theta0=0.40055 rad (22.9°), theta1=−0.13963 rad/m (−8.0°/m) | [params.cpp:12](../agilicious/simulator/model_propeller_bem_params.cpp#L12) (YAML `pitch`/`twist`, [:31](../agilicious/simulator/model_propeller_bem_params.cpp#L31)) | geometry vs repo's 21.77°, −11°/m ([params.h:69](../NeuroBEM/code/simulator/include/params.h#L69)) |

Notes:
- #2/#3 are hardcoded constants; #4 is a YAML-tunable member (value not in repo, back-fit ≈1.30); #1 are identified params.

## Key findings
- Baseline `bem` **Fxy (0.575) already beats the paper (0.803)**; the whole gap is Fz.
- Fz error is a **forward-flight** effect: 0.33 N at hover → 4.06 N at 14–18 m/s.
- Corrections are **not independently portable**: `+0.07` on the default large cl blows up
  (`bem-007`, Fz 7.0); it only works paired with agilicious's smaller cl and thrust_scale.


