# Project Goal

Port NeuroBEM (Bauersfeld et al., RSS 2021) into the Agilicious simulator and reproduce the closed-loop simulation performance reported in the paper: simulated flights of the test trajectories should match the real recorded flights with accumulated positional error comparable to the paper's Table III results.

**Advisor (Michal Pliska) priorities, in order:**
1. Reproduce the paper's results.
2. Try CMA-ES for parameter tuning — see what it can actually recover.
3. Get the whole pipeline working end-to-end.

The NN can be trained by me once the pipeline runs. **The real downstream task is applying this to the Eagle drone in closed-loop simulation** — the NeuroBEM reproduction is the stepping stone to that.

## Context

- NeuroBEM = BEM rotor physics + a small NN predicting residual forces/torques on top.
- Agilicious already ships the BEM rotor model from the paper. The work is: port the NN residual, wire it into the sim loop, and build the evaluation pipeline.
- **No motion-capture access.** All training data and all ground truth come from the public NeuroBEM dataset (already preprocessed): https://download.ifi.uzh.ch/rpg/NeuroBEM/ (processed_data.zip). We never record our own flights.

## NN residual — fixed interface

- **Input:** 20 timesteps x 10 channels (3 body-frame linear velocities, 3 body rates, 4 motor speeds), sampled every 2.5 ms = 50 ms history.
- **Output:** 6 values — residual force (x,y,z) + residual torque (x,y,z), added to BEM output.
- **Architecture:** two heads (force / torque), leaky-ReLU or similarly smooth activations, linear output layer. 12k–72k params (paper's pick: TCN-medium, 25k; a 30k MLP was within 1%).
- Strictly causal. Bounded outputs (clamp/tanh-scale to plausible residual magnitudes) — especially the torque head; an unbounded learned torque residual caused a feedback-loop crash in the paper's ablations.
- Must export to C++ (ONNX Runtime or libtorch), deterministic cost, called at 1 kHz.
- Reuse the exact normalization constants from training; ship them with the network.

## Training (do not change)

Single-step supervised regression, Adam, RMSE loss on residual force/torque labels from the dataset. Target parity with paper Table I: ~0.352 N force RMSE, ~5.3e-3 Nm torque RMSE on the held-out split.

## Simulation loop

Per 1 ms tick: (simulated low-level controller) -> first-order motor model -> BEM + NN aero -> rigid-body integration.

- Integrator: **symplectic Euler, 1 ms**, to match the paper's evaluation setup (Agilicious defaults to RK4 — switch it).
- NN inputs come from the sim's own ground-truth state, rotated to body frame. No estimator, no added noise.
- Maintain the 50 ms input ring buffer; define warm-up for the first 50 ms after reset (match training padding) and clear it between episodes.
- Closed-loop runs must emulate the BetaFlight-style low-level controller behavior (rate loop, command filtering/interpolation), or results are not comparable to the paper.

## Evaluation ladder (in this order)

1. **Single-step:** reproduce Table I RMSE on the held-out dataset split.
2. **Open-loop rollout:** start from a recorded state, replay the recorded motor speeds, integrate forward (~1 s windows), measure position drift vs. the recording. Average over many windows and trajectories.
3. **Closed-loop:** MPC + simulated low-level fly the paper's reference trajectories (ellipses, lemniscates at multiple speeds) entirely in sim; compare the generated path against the recorded real flight (paper Table III protocol).

Model selection and regression testing use trajectory drift (2 and 3), never per-step RMSE alone.

## Known pitfalls

- Frame and unit conventions (body vs. world frame, rad/s vs. rpm) and normalization constants — verify against dataset docs before debugging anything else.
- Recorded data is only ever an input (open-loop motor replay) or a ground-truth ruler; the sim always generates states itself.
- Integrator or low-level-controller mismatch silently invalidates comparisons with the paper.

## Out of scope for now

Architecture swaps, multi-step/rollout training, and BEM solver improvements (robust bracketed v_i residual, smooth VRS closure, dynamic inflow, lookup tables) are later phases. Do not start them until the evaluation ladder reproduces the paper.