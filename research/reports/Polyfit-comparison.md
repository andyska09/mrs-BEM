# PolyFit + NN

## Question

Table II augments None, Fit and BEM with a learned residual, but not PolyFit. Does
PolyFit + NN outperform BEM + NN?

## Naming

`Fit` in Table II is the quadratic thrust/torque model, not PolyFit ([RSS21_Bauersfeld.md:493](../sources/papers/RSS21_Bauersfeld.md#L493)). 

PolyFit is the Sun/de Visser/Chu model, introduced separately as a standalone baseline ([:500](../sources/papers/RSS21_Bauersfeld.md#L500)). The paper's three `+NN` rows are therefore quadratic, none and BEM.

## Setup

Base model `polyfit_paper`, 3 bins. Network is the paper TCN with `preds: polyfit_paper` and no other change ([tcn_polyfit.yaml](../../MyBEM/configs/nets/tcn_polyfit.yaml)), 3 seeds.

## Results

RMSE, force in N, torque in Nm.

| model | Fxy | Fz | F | Mxy | Mz | M |
|---|---|---|---|---|---|---|
| polyfit_my | 0.357 | 0.839 | 0.565 | 0.0163 | 0.0071 | 0.0139 |
| paper PolyFit | 0.453 | 0.832 | 0.606 | 0.0270 | 0.0080 | 0.0220 |
| tcn_polyfit (n=3) | 0.204 ±0.001 | 0.476 ±0.003 | 0.322 ±0.002 | 0.0062 ±0.0001 | 0.0035 ±0.0001 | 0.0055 ±0.0001 |
| | | | | | | |
| bem_default | 0.575 | 1.664 | 1.069 | 0.1273 | 0.0151 | 0.1043 |
| paper BEM | 0.803 | 1.265 | 0.982 | 0.0900 | 0.0170 | 0.0740 |
| tcn_BEM (n=1) | 0.168 | 0.399 | 0.268 | 0.0082 | 0.0023 | 0.0069 |
| paper BEM+NN | 0.204 | 0.504 | 0.335 | 0.0140 | 0.0040 | 0.0120 |

## Findings

- No, PolyFit + NN does not win. It is better on torque (M 0.0055 against 0.0069),
  BEM + NN is better on force (F 0.268 against 0.322).
- Both beat the published BEM+NN on every column except Mz, which is uninformative:
  the largest yaw torque in the dataset is 0.072 Nm
  ([:497](../sources/papers/RSS21_Bauersfeld.md#L497)).
- Before training, PolyFit leads `bem_default` by 1.9x on force and 7.5x on torque.
  After training the order reverses on force. The residual absorbs most of the
  difference between the two bases, so a better base does not give a better model.
- Seed spread on `tcn_polyfit` is 0.3 %.
- The PolyFit base as identified here beats the paper's PolyFit on Fxy (0.357
  against 0.453) and M (0.0139 against 0.0220), and ties on Fz.

## Caveats

- `bem_default` is untuned, so the BEM side of this comparison is not the paper's BEM.
- `tcn_BEM` is a single seed, so its row has no error bar to compare against.

## Open

The reduced training set. Paper PolyFit* degrades severely (F 4.011, M 2.301,
[:444](../sources/papers/RSS21_Bauersfeld.md#L444)) where BEM+NN* holds. Whether the
residual recovers it under a slow-only split belongs to the
[generalization sweep](Generalization.md).
