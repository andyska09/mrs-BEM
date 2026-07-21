# BEM tuning results

Metrics (paper Table II): `Fxy=√((Fx²+Fy²)/2)`, `Fz` per-axis; `F=√((Fx²+Fy²+Fz²)/3)`; same for torque M.
Ground truth = `f=m·a`, `τ=I·α+ω×Iω` with mass 0.772, inertia [0.00254,0.00214,0.00436].

## Results (test set, Table II format)

| config | Fxy | Fz | Mxy | Mz | F | M |
|---|---|---|---|---|---|---|
| `bem` (baseline) | 0.575 | 1.663 | 0.127 | 0.015 | 1.069 | 0.104 |
| `bem-01` | 0.577 | 1.612 | 0.124 | 0.015 | 1.043 | 0.102 |
| `bem-02`² | 0.397 | 0.907 | 0.129 | 0.014 | 0.616 | 0.106 |
| **paper BEM** | 0.803 | 1.265 | 0.090 | 0.017 | 0.982 | 0.074 |
| paper BEM+NN | 0.204 | 0.504 | 0.014 | 0.004 | 0.335 | 0.012 |


² CMA-ES 6-param **force-only** fit (cl, cd, k, lift_offset, hforce_scale, pitch) on the 20k subset. Force −42% vs baseline out-of-sample; torque unchanged (not in the objective). cl 4.85 / pitch 27° / h-force ×2.65 are an entangled force-fitting compromise, not identified physics — rerun with `--joint` to constrain torque.

## Folders

| folder | files | cols | cl | cd | k | extra | purpose |
|---|---|---|---|---|---|---|---|
| `bem` | full | 35 | 15.242 | 13.549 | 5.89 | — | canonical baseline, default identified params (params.h) |
| `bem-01` | full | 35 | 14.329 | 14.454 | 5.89 | — | CMA-ES force-only fit (cl, cd) |
| `bem-02` | test | 35 | 4.846 | 3.086 | 1.729 | +0.046 offset, ×2.65 h-force, pitch 27.0° | CMA-ES 6-param force-only fit (subset_20k) |

### Old runs

| folder | files | cols | cl | cd | k | extra | purpose |
|---|---|---|---|---|---|---|---|
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
- The set is **coupled** — porting #2 alone onto the repo's large cl blows up (`bem-007`, Fz 7.0). #1+#2+#3+#4 together recover the paper (Fz≈1.0, F≈0.71).

## Key findings
- Baseline `bem` **Fxy (0.575) already beats the paper (0.803)**; the whole gap is Fz.
- Fz error is a **forward-flight** effect: 0.33 N at hover → 4.06 N at 14–18 m/s.
- Corrections are **not independently portable**: `+0.07` on the default large cl blows up
  (`bem-007`, Fz 7.0); it only works paired with agilicious's smaller cl and thrust_scale.


