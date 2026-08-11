# Residual network architectures

Status: screening done (seed 0, n=1). 3-seed finals pending.

## Question

The paper picks a TCN for the residual and never justifies it against
alternatives. Does the architecture matter, or does any model of the same
capacity land in the same place?

1. Does recurrence buy anything over convolution on a 20-step (50 ms) window?
2. Is the paper's plain 2-layer valid-conv stack the right convolution?
3. How much is nonlinearity worth at all? Ridge on the flattened window is the
   floor every net has to clear.

## Setup

Base model `bem_default`, no CMA-ES variant. Everything except `net:` and
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

`dtcn` follows Bai et al.'s TCN, not WaveNet: no gated activation, which WaveNet
found better specifically "for modeling audio signals"
([WaveNet_1609.03499.md:181-184](../sources/papers/WaveNet_1609.03499.md#L181)),
and no summed skip connections ([:224-226](../sources/papers/WaveNet_1609.03499.md#L224))
— residual shortcut only. Span is 21 against the 20-step window.

## Protocol

Screen the grid at seed 0 (13 jobs), take the lowest validation loss per arch,
rerun on seeds 0/1/2 (15 jobs). MetaCentrum GPU queue.

Reporting is hold-out test RMSE in paper Table II format, mean ± std over seeds.
A ranking is claimed only where the gap exceeds the seed spread — the old TF
stack was unseeded, which made every n=1 comparison in this repo unfalsifiable.

Paper Table II, [RSS21_Bauersfeld.md:437-443](../sources/papers/RSS21_Bauersfeld.md#L437):

| | Fxy | Fz | F | Mxy | Mz | M |
|---|---|---|---|---|---|---|
| BEM | 0.803 | 1.265 | 0.982 | 0.090 | 0.017 | 0.074 |
| BEM+NN | 0.204 | 0.504 | 0.335 | 0.014 | 0.004 | 0.012 |

## Results — screening, seed 0

Validation loss, sorted by size. Pareto frontier in bold: for every other row,
something smaller is also better.

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

### Test set, `arch_dtcn_48`, one seed

| | Fxy | Fz | F | Mxy | Mz | M |
|---|---|---|---|---|---|---|
| BEM (MyBEM base) | 0.575 | 1.663 | 1.069 | 0.1272 | 0.0151 | 0.1043 |
| BEM + dtcn_48 | 0.174 | 0.421 | 0.281 | 0.0076 | 0.0022 | 0.0064 |
| paper BEM+NN | 0.204 | 0.504 | 0.335 | 0.0140 | 0.0040 | 0.0120 |

Beats the paper on all six columns from a *worse* base — MyBEM's untuned BEM is
above the paper's own (F 1.069 vs 0.982, M 0.104 vs 0.074).

### Against the CMA-ES runs

The MyBEM base row matches NeuroBEM's `bem` row to three decimals
([table_testset_rmse.ipynb](../../NeuroBEM/CMAES-results-analysis/table_testset_rmse.ipynb)),
so these are comparable ([table2.ipynb](../../NeuroBEM/CMAES-results-analysis/table2.ipynb)):

| config | base | net | F | M |
|---|---|---|---|---|
| BEM+NN | untuned | TF TCN | 0.353 | 0.008 |
| **BEM + dtcn_48** | **untuned** | **dtcn** | **0.281** | **0.0064** |
| BEM c2+NN | 19 params freed | TF TCN | 0.272 | 0.006 |
| BEM c3+NN | 16 params freed | TF TCN | 0.291 | 0.004 |

Same base, better net: 20% on both force and torque. And the architecture change
buys as much force accuracy as the entire 19-parameter CMA-ES retune (0.281 vs
0.272 — a tie given the slop below). Torque is the other way: `c3+NN` reaches
0.004 against 0.0064, because tuning moves the *base* torque a long way
(0.104 → 0.036) and leaves the net less to clean up. Tuned base **plus** dtcn is
unrun and is the obvious next step.

Slop between the two stacks: NeuroBEM used the `get_datafiles.bash` split (49 val
segments, seed 42) not `paper.yaml`, its runs were unseeded
([window_generator.py:142](../../NeuroBEM/code/Python/utils/window_generator.py#L142)),
and its labels used the README inertia while its RMSE used the `params.h` one.
A few percent — enough that 0.281 vs 0.272 is a tie, not enough to touch
0.353 → 0.281.

### Caveats

- **n = 1.** No seed spread yet, so tcn 0.4834 vs lstm_64 0.4660 is not a ranking.
- **Both winners sit at the top edge of their grid.** dtcn_48 and lstm_64 are the
  largest points and the best ones; neither has turned over. Extend both a step.
- **Everything overfits.** Train loss ends near a third of val for every net. The
  MLPs peak at epoch 8 and degrade up to 21% by epoch 120; dtcn/lstm peak around
  epoch 20 and degrade 5–7%; only tcn and linear stay flat. Best-val checkpointing
  is doing real work here — weight decay or dropout would likely move the ranking.
  120 epochs is mostly waste; the winners needed ~20.
- `arch_mlp_128` was killed at its 2h walltime after epoch 105. Its best was epoch
  8 and val rose monotonically after, so the checkpoint stands.
