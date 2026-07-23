# BEM assumptions & the generalization claim — research notes

## Fundamental BEM assumptions the paper leaves unaddressed

1. **Uniform induced velocity over the disk.** Eq. (5) assumes constant `vi` across the whole flow tube ([RSS21_Bauersfeld.md:242-249](../papers/RSS21_Bauersfeld.md#L242)). Real edgewise rotors have a strong fore-aft inflow gradient (that's why helicopter literature has linear-inflow/Glauert corrections, Pitt–Peters, etc. — none used here). No tip loss, no root cutout either. This biases exactly the forward-flight regime — consistent with the finding that Fz error grows 0.33 N (hover) → 4.06 N (14–18 m/s) ([results.md:48](results.md#L48)).
2. **Quasi-steady everything.** `vi` is re-solved instantaneously from a static momentum balance every timestep; there is no dynamic inflow (wake lag), no unsteady blade aero, no dynamic stall, no wake reingestion — at 46.8 m/s² maneuvers the wake demonstrably can't be in steady state. The 50 ms NN history is implicitly tasked with all wake dynamics.
3. **Momentum theory in edgewise flight is itself heuristic.** Eq. (5) is Glauert's interpolation formula, not derived physics for a rotor in edgewise flow — its validity degrades in fast forward flight, the regime the paper claims to win.
4. **Vortex-ring state is an empirical patch.** A quartic polynomial fit plus a `max(ṽi, vh,i)` switch ([RSS21_Bauersfeld.md:296-322](../papers/RSS21_Bauersfeld.md#L296)) — a steady-state curve for an intrinsically unsteady, fluctuating regime, applied per-rotor with a uniform criterion.
5. **Flat-plate polar, constant coefficients.** `cl = cl₀ sinα cosα`, `cd = cd₀ sin²α`, identified once on a static test stand ([RSS21_Bauersfeld.md:290-293](../papers/RSS21_Bauersfeld.md#L290)). No Reynolds dependence (Re varies several-fold along the blade and with rpm), no stall hysteresis, extrapolated far outside test-stand conditions.
6. **First-harmonic rigid-blade flapping.** Rigid blade + single torsion spring `kβ`, only a0/a1/b1; no blade torsion/pitch-flap coupling, no lag motion, no higher harmonics. `kβ` comes from the fragile audio-FFT `Coning.m` hack. `vi` is additionally computed with a0=a1=b1=0 (step 1 of the algorithm, [RSS21_Bauersfeld.md:325](../papers/RSS21_Bauersfeld.md#L325)).
7. **Internal inconsistency in the implementation.** The coning/flapping expressions are frozen Maple output with all coefficients baked in numerically ([calculateConing.cpp:12-14](../NeuroBEM/code/simulator/src/simulator/calculateConing.cpp#L12)) — identified `cl`/`cd` (and the CMA-ES retunes) never reach the flapping angles or the `kβ` torque path. The "identified" model is only half-identified.
8. **Still air baked in.** Airspeed ≡ mocap ground velocity for both BEM inputs and NN features. Any wind is invisible to the model; the lab's own later work (HDVIO2.0) treats wind estimation as a separate open problem.
9. **Environment/actuation simplifications:** no ground/wall/ceiling effect, constant air density, thrust independent of battery voltage, fixed first-order motor lag (τ = 33 ms).
10. **Rotor–rotor and body–rotor interaction deliberately excluded** and delegated to the NN ([RSS21_Bauersfeld.md:352-354](../papers/RSS21_Bauersfeld.md#L352)) — acknowledged, but it means the "physics" is one isolated-rotor model applied four times.

## Challenging the generalization claim

The claim ("strong generalization beyond the training set … predicts accurate forces and torques where other methods break down") is far narrower than it reads:

1. **Only one generalization axis was tested: speed.** Same drone, same indoor arena, still air, same trajectory families flown faster (Fig. 8: "each trajectory is flown multiple times with varying speeds"). The reduced-set experiment ([RSS21_Bauersfeld.md:487-507](../papers/RSS21_Bauersfeld.md#L487)) extrapolates ≤5 m/s → 18 m/s and nothing else. No wind, no other platform, no payload change.
2. **The hold-out has near-leakage.** [testset.txt](../NeuroBEM/testset.txt) holds out e.g. `2021-02-18-13-44-23_seg_2` while `_seg_1`/`_seg_3` of the *same flight* go to training ([get_datafiles.bash:60-64](../NeuroBEM/code/Python/data/get_datafiles.bash#L60); confirmed in `processed_data/bem/`). Segments are the same commanded trajectory minutes apart. "Unseen test set" for Tables I/II is generous — it's near-in-distribution.
3. **The baselines that "break down" are weak by construction.** PolyFit is polynomial regression — guaranteed to explode when extrapolated; Fit is a hover model; None+NN is a pure NN. Beating them outside the fit range is a low bar. Crucially, BEM's parameters come from the *test stand*, not the flight training set — so the reduced-training experiment barely perturbs the physics prior. What Fig. 9 demonstrates is "physics priors extrapolate," not that the *learned* hybrid generalizes.
4. **The paper's own numbers undercut the strong wording.** In closed loop (Table III) plain BEM is already competitive at speed (0.369 vs 0.285 m on the fastest ellipse); the NN residual visibly degrades out-of-envelope (BEM+NN\* 0.371 vs BEM+NN 0.286 on the 12 m/s lemniscate); a pure NN beat them on torques in the reduced setting ([RSS21_Bauersfeld.md:559-561](../papers/RSS21_Bauersfeld.md#L559)); and the yaw envelope is degenerate by design (max 0.072 Nm), so Mz accuracy is essentially untested.
5. **External benchmark on the same data:** [NeuroMHE](https://arxiv.org/abs/2206.10397) reports 37–78% lower force RMSE than NeuroBEM on the NeuroBEM test set itself (largest gain on Fz). Caveat: it's an online estimator using acceleration feedback, not a pure predictor — but it shows how much residual force NeuroBEM leaves on the table in its home regime.
6. **The authors' own deployment contradicts the claim.** Agilicious's BEM ships four coupled empirical patches — ~3× smaller re-identified cl/cd, +0.07 lift offset, ×3 h-force (commented "BEM underestimates drag"), ~1.3 thrust scale ([results.md:16-31](results.md#L16)) — and the repro shows the unpatched BEM's Fz RMSE (1.66 N) is ~22% of the drone's weight on the test set. If the physics truly "predicted accurate forces where other methods break down," their own simulator wouldn't need a 30% thrust multiplier and a 3× drag fudge.

**Verdict:** the defensible version of the claim is *"a test-stand-identified physics prior extrapolates in speed better than polynomial regression, and the NN residual degrades gracefully near its training distribution."* The strong wording is unsupported for wind, yaw torque, other platforms — and even for Fz in fast forward flight, which the numbers show is the dominant miss.

## Sources

- [NeuroBEM (arXiv:2106.08015)](https://arxiv.org/abs/2106.08015)
- [NeuroMHE (arXiv:2206.10397)](https://arxiv.org/abs/2206.10397)
- [PI-TCN (arXiv:2206.03305)](https://arxiv.org/abs/2206.03305)
- [PI-WAN (arXiv:2507.00816)](https://arxiv.org/html/2507.00816)
- [HDVIO2.0 (arXiv:2504.00969)](https://arxiv.org/html/2504.00969v1)
- [Bauersfeld et al., quadrotor induced airflow (arXiv:2403.13321)](https://arxiv.org/html/2403.13321v2)
- [QBlade BEM limitations](https://docs.qblade.org/src/theory/aerodynamics/bem/bem.html)
- [Helicopter BEMT notes](https://kumar-sumeet.github.io/HeliAeroNotes/Lectures/2_BET/2_BEMT.html)
