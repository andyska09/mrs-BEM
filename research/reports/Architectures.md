# Residual network architectures

Status: proposed, not run.

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
[tcn_baseline.yaml](../../MyBEM/configs/experiments/tcn_baseline.yaml): `paper`
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

## Results

Not run.
