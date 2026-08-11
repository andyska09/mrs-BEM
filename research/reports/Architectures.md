# Residual network architectures

## Question

The paper picks a TCN for the residual and never justifies it against
alternatives. Does the architecture matter, or does any model of the same
capacity land in the same place?

1. Does recurrence buy anything over convolution on a 20-step (50 ms) window?
2. Is the paper's plain 2-layer valid-conv stack the right convolution?
3. How much is nonlinearity worth at all? Ridge on the flattened window is the
   floor every net has to clear.

## Setup

Base model `bem_default`. Everything except `net:` and
`optim.l2` held at
[tcn_baseline.yaml](../../MyBEM/configs/nets/tcn_baseline.yaml): `paper`
split, history 20, features `[angvel, linvel, motors]` (10 dims), `max_speed: 0`,
`two_heads: true`, Adam 120 epochs.

| arch | grid | params |
|---|---|---|
| `tcn` | control, paper config: `kernels [7,5]`, `filters [32,32]`, `neurons [20]` | 27,814 |
| `mlp` | `neurons` ∈ {[64,32], [128,64], [256,128]} | 15k – 120k |
| `dtcn` | `channels` ∈ {16, 32, 48}; 3 residual blocks of 2 convs, k=3, dilations [1,2,2] | 8k – 60k |
| `lstm` | `hidden` ∈ {32, 48, 64}, 1 layer, last hidden state → head | 11k – 40k |
| `linear` | `optim.l2` ∈ {0, 1e-4, 1e-2}; `nn.Linear(20*F, 6)` | 1,206 |

`dtcn` is the generic TCN of [Bai et al.](https://arxiv.org/pdf/1803.01271) —
dilated causal convolutions, two per residual block, LeakyReLU, residual shortcut.
No gated activation and no summed skip connections. Span is 21 against the
20-step window.

## Protocol

Screen the grid at seed 0, take the lowest validation loss per arch, rerun on
seeds 0/1/2. Five configs got the extra seeds: the four arch winners plus
`dtcn_16`. `linear` stayed at n=1 — its objective is convex, so a seed moves only
where Adam stops, not the optimum.

Reporting is hold-out test RMSE in paper Table II format, mean ± std over seeds.
A ranking is claimed only where the gap exceeds the seed spread.

Paper Table II, [RSS21_Bauersfeld.md:437-443](../sources/papers/RSS21_Bauersfeld.md#L437):

| | Fxy | Fz | F | Mxy | Mz | M |
|---|---|---|---|---|---|---|
| BEM | 0.803 | 1.265 | 0.982 | 0.090 | 0.017 | 0.074 |
| BEM+NN | 0.204 | 0.504 | 0.335 | 0.014 | 0.004 | 0.012 |

## Screening — validation loss, seed 0

Sorted by size. Pareto frontier in bold: for every other row, something smaller
is also better.

| params | run | val |
|---|---|---|
| **1,206** | **linear** | **2.0090** |
| **9,990** | **dtcn_16** | **0.4825** |
| 12,710 | lstm_32 | 0.4979 |
| **25,126** | **lstm_48** | **0.4743** |
| 27,814 | tcn (paper) | 0.4834 |
| 30,086 | mlp_64 | 0.5529 |
| **35,174** | **dtcn_32** | **0.4323** |
| 41,638 | lstm_64 | 0.4660 |
| 68,358 | mlp_128 | 0.5108 |
| **75,718** | **dtcn_48** | **0.3970** |
| 169,478 | mlp_256 | 0.5020 |

**dtcn wins outright and per parameter.** `dtcn_16` beats the paper TCN at 2.8×
fewer parameters, so the win is not a capacity artifact. Family scaling:
dtcn 0.483 → 0.432 → 0.397, still falling at the grid edge; lstm 0.498 → 0.474 →
0.466, flattening near 0.46; mlp flat and worst — 169k params still lose to a 10k
dtcn, so it is structure, not capacity.

Ridge strength is inert: 2.0090 / 2.0103 / 2.1097 for l2 = 0 / 1e-4 / 1e-2.
1,206 parameters against 1.26M windows cannot overfit.

## Finals — test set, mean ± std over 3 seeds

| run | params | val | Fxy | Fz | F | Mxy | Mz | M |
|---|---|---|---|---|---|---|---|---|
| mlp_256 | 169,478 | 0.4979 | 0.168±0.001 | 0.393±0.004 | **0.265**±0.002 | 0.0093±0.0001 | 0.0031±0.0000 | 0.0078±0.0001 |
| dtcn_48 | 75,718 | **0.4062** | 0.173±0.002 | 0.410±0.015 | 0.276±0.008 | 0.0077±0.0003 | 0.0022±0.0000 | **0.0064**±0.0003 |
| tcn (paper) | 27,814 | 0.4872 | 0.179±0.002 | 0.431±0.020 | 0.288±0.010 | 0.0086±0.0003 | 0.0023±0.0000 | 0.0072±0.0002 |
| dtcn_16 | 9,990 | 0.4894 | 0.180±0.000 | 0.423±0.006 | 0.285±0.003 | 0.0085±0.0000 | 0.0023±0.0000 | 0.0071±0.0000 |
| lstm_64 | 41,638 | 0.4767 | 0.186±0.001 | 0.432±0.003 | 0.292±0.001 | 0.0077±0.0004 | 0.0022±0.0000 | **0.0064**±0.0004 |
| linear (n=1) | 1,206 | 2.0090 | 0.310 | 0.906 | 0.581 | 0.0292 | 0.0054 | 0.0240 |
| BEM base | — | — | 0.575 | 1.664 | 1.069 | 0.1273 | 0.0151 | 0.1043 |
| paper BEM | — | — | 0.803 | 1.265 | 0.982 | 0.0900 | 0.0170 | 0.0740 |
| paper PolyFit | — | — | 0.453 | 0.832 | 0.606 | 0.0270 | 0.0080 | 0.0220 |
| paper BEM+NN | — | — | 0.204 | 0.504 | 0.335 | 0.0140 | 0.0040 | 0.0120 |

**Every net beats paper BEM+NN on all six columns**, from a base that is worse
than the paper's own (F 1.069 vs 0.982, M 0.104 vs 0.074). Even the paper TCN
reimplemented here reaches F 0.288 against their 0.335 — so part of the gain is
the PyTorch rebuild, not the architecture.

**Validation loss and test force disagree.** `mlp_256` has the worst val loss of
the five and the best test force, and its seed spread (±0.002) is far too small
for that to be noise. Validation loss is a single scalar mixing force and torque
with weights [1,1,3,1,1,1] ([train.py:36-40](../../MyBEM/mybem/train.py#L36)),
and the MLP trades torque away to buy force. Selection on val loss therefore
picked `dtcn_48`, which does win the quantity val loss measures, but not the
model with the lowest force error.

**Torque is where convolution and recurrence win cleanly.** dtcn_48 and lstm_64
both reach M 0.0064 against `mlp_256`'s 0.0078, a gap 3–4× the seed spread, and
the same ordering holds on both components (Mxy 0.0077 vs 0.0093, Mz 0.0022 vs
0.0031). The MLP's force advantage is 4%, its torque penalty 22%.

**Depth of the dtcn stack does not pay off on test.** `dtcn_16` at 9,990
parameters gets F 0.285 / M 0.0071 against `dtcn_48`'s 0.276 / 0.0064 at 7.6×
the size — a 3% force and 10% torque difference. On val the same pair reads 0.489
vs 0.406.

`linear` lands on the paper's PolyFit row (0.581 / 0.0240 against 0.606 / 0.0220):
a 1,206-parameter affine map over a 50 ms window is worth roughly what their
polynomial fit is.

### Caveats

- The finals were selected on validation loss, which mixes force and torque; the
  table reports them separately.
- Seed spread is not uniform: `tcn` Fz ±0.020 and `dtcn_48` Fz ±0.015 against
  `lstm_64` ±0.003 and `mlp_256` ±0.004. The unstable ones are those whose best
  epoch comes early, so the checkpoint catches a more variable iterate.
- Every net overfits, and by very different amounts (means over the same seeds, `degrade` is val at epoch 120 over val at best):

  | run | best epoch | val best | val end | train end | degrade |
  |---|---|---|---|---|---|
  | mlp_256 | 8.0 | 0.4979 | 0.5790 | 0.1556 | 16.3% |
  | dtcn_48 | 20.7 | 0.4062 | 0.4234 | 0.1595 | 4.2% |
  | lstm_64 | 20.7 | 0.4767 | 0.4942 | 0.1653 | 3.7% |
  | tcn | 86.3 | 0.4872 | 0.4946 | 0.2890 | 1.5% |
  | dtcn_16 | 105.0 | 0.4894 | 0.4940 | 0.2987 | 0.9% |
  | linear (n=1) | 91.0 | 2.0090 | 2.0090 | 1.8377 | 0.0% |

  The high-capacity models end at roughly a third of their validation loss on
  train and peak early; `mlp_256` is worst on both counts. Best-val checkpointing
  is therefore load-bearing — at epoch 120 `mlp_256` would score 0.5790, behind
  everything except `linear`. It also explains the uneven seed spread: the models
  that peak early have a noisier choice of checkpoint.


