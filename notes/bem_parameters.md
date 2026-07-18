# BEM parameter inventory — groundwork for full-parameter CMA-ES

Every parameter of the C++ BEM, where it acts at runtime, and whether tuning it can do anything.
Active config: `MODEL 1, CHORD 1, POLAR 1, DIST 0` ([params.h:7-33](../NeuroBEM/code/simulator/include/params.h#L7)).

## How parameters flow

- Only `cl, cd, k` are instance members, runtime-settable via the `setAero` chain
  Quadcopter→Motor→Propeller→GSLHelper ([quadcopter.cpp:32](../NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L32),
  [motor.cpp:83](../NeuroBEM/code/simulator/src/simulator/motor.cpp#L83),
  [propeller.cpp:148-157](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L148)).
  Everything else is `static constexpr` ([params.h:61-79](../NeuroBEM/code/simulator/include/params.h#L61)) —
  tuning it means promoting to instance member + extending the set-chain (the struct comment
  already anticipates this, [params.h:56-58](../NeuroBEM/code/simulator/include/params.h#L56)).
- Physics parameters act in exactly two places: the blade-element integrand + momentum balance
  ([gslHelper.cpp:155-232](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L155)) and the
  force/torque assembly ([propeller.cpp:129-136](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L129)).
  The coning/flapping angles are frozen Maple polynomials with every coefficient baked in numerically —
  zero `param.` references ([calculateConing.cpp:13-33](../NeuroBEM/code/simulator/src/simulator/calculateConing.cpp#L13), same for both flapping files).

## Propeller_s ([params.h:59-107](../NeuroBEM/code/simulator/include/params.h#L59))

| param | value | meaning | runtime use | tunable? |
|---|---|---|---|---|
| `cl` | 15.24214 | polar lift coeff, `cl·sinα·cosα` | integrand lift [gslHelper.cpp:191](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L191) | ✅ already (setAero) |
| `cd` | 13.54894 | polar drag coeff, `cd·sin²α` | integrand drag [gslHelper.cpp:192](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L192) | ✅ already |
| `k` | 5.89 | hinge-spring stiffness kβ | flapping→torque map only: `τ=[±k·b1s, k·a1s, ∓Q]` [propeller.cpp:135](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L135) | ✅ already; pure linear gain on the Mxy flapping torque — does **not** move the angles (baked) |
| `theta0` | 21.77° | blade pitch | AoA in integrand [gslHelper.cpp:185](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L185) | ✅ integrand only, not flapping. Agilicious remeasured: 22.95° |
| `theta1` | −11° | twist over full span (multiplies `r/R`; the "[rad/m]" comment is misleading) | same line | ✅ integrand only. Agilicious: −8° |
| `ci`, `co` | 1.7, 0.7 cm | chord root/tip, `c(r)=ci+r/R·(co−ci)` | [gslHelper.cpp:205](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L205) | ✅ joint (ci,co) scale ≈ scaling cl&cd together → degeneracy; the *ratio* is the real dof |
| `R` | 6.48 cm | prop radius | advance ratio [propeller.cpp:84](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L84), A=πR², integration limit + `r/R` [gslHelper.cpp:123,160,180-185,205](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L123) | ⚠ measured geometry; also feeds mu → flapping input |
| `A` | πR² | disk area | momentum eq [gslHelper.cpp:166](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L166) | derived — tune R, never A |
| `b` | 3 | blade count | linear prefactor `b·ρ/4π` on T,Q,H [gslHelper.cpp:92,100,108,163](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L92) | ❌ pure joint scale, 100 % degenerate with (cl,cd) |
| `rho` | 1.204 | air density | same prefactor + momentum eq + body drag [quadcopter.cpp:163](../NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L163) | ❌ near-degenerate with (cl,cd); physical constant |
| `g` | 9.81 | gravity | only inside constexpr `Mb` [params.h:79](../NeuroBEM/code/simulator/include/params.h#L79) | dead in C++ |
| `ef`, `e` | 0.1, 0.1·R | hinge offset | **unused at runtime** (baked into Maple) | dead |
| `sigma` | 0.215 | solidity | **unused anywhere** | dead |
| `c` | 1.3 cm | constant chord | only `CHORD==0` [gslHelper.cpp:203](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L203) | dead under CHORD 1 |
| `mb, cg, Ib, Mb` | — | blade mass/CoG/inertia/hinge moment | **unused at runtime** (Maple derivation inputs) | dead |
| `cl1, cl2, M, alpha0` | — | stall model | only `POLAR==2` | dead under POLAR 1 |

Under `MODEL==0`, `cl`/`cd` are reused as quadratic thrust/torque coefficients
([propeller.cpp:19,36](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L19)).

## Motor_s / Quadcopter_s ([params.h:109-129](../NeuroBEM/code/simulator/include/params.h#L109))

| param | value | runtime use | tunable? |
|---|---|---|---|
| `Motor::tau` | 0.033 | **unused** here (replay uses measured Ω; `simulateOmega` declared [motor.h:20](../NeuroBEM/code/simulator/include/motor.h#L20), never defined) | closed-loop sim only |
| `Motor::inertia` | 9.36e-6 | only [motor.cpp:24](../NeuroBEM/code/simulator/src/simulator/motor.cpp#L24) — buggy & inert, see fact 6 | effectively dead |
| `mass, Ixx–Izz` | 0.752, … | **unused in C++**; CMAES.cpp has its own MASS 0.772 / INERTIA [CMAES.cpp:23-24](../NeuroBEM/code/simulator/src/CMAES.cpp#L23) | ❌ these define the *target* — tuning them is leakage |
| `dx, dy, dz` | 0.078, 0.1, 0.027 | motor placement [quadcopter.cpp:12-26](../NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L12) → torque arms [quadcopter.cpp:155](../NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L155) + per-motor velocity `vel_M=vel_B+ω×offset` [motor.cpp:20](../NeuroBEM/code/simulator/src/simulator/motor.cpp#L20) | ✅ strong lever on torques |
| `cxy, cz, Ax, Ay, Az` | 1, 1, 54/90/60 cm² | fuselage drag [quadcopter.cpp:162-168](../NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L162), **in bem-model output** via `getForce()=drag+thrust` [quadcopter.cpp:60-63](../NeuroBEM/code/simulator/src/simulator/quadcopter.cpp#L60), [simulator.cpp:57](../NeuroBEM/code/simulator/src/simulator/simulator.cpp#L57) | ✅ but only the products `cxy·Ax, cxy·Ay, cz·Az` are identifiable → 3 effective params |
| `rho` (quad) | = Propeller rho | drag eq | alias, not separate |

## Hardcoded constants that are really model parameters

| constant | where | status |
|---|---|---|
| `+0.07` lift offset | [gslHelper.cpp:191](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L191) | agilicious port (commit e39651d). Remove for raw BEM → reintroduce as param `cl_offset`, default 0 |
| `×3.0` h-force | [propeller.cpp:115](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L115) | agilicious port (e39651d). Remove → reintroduce as `hforce_scale`, default 1 |
| `thrust_scale` ≈1.30 | agilicious only, [model_propeller_bem.cpp:123](../agilicious/simulator/model_propeller_bem.cpp#L123) | never ported → introduce as param, default 1 |
| VRS quartic `k0..k4` = 1, −1.125, −1.372, −1.718, −0.655 | [gslHelper.cpp:40-44](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L40) | empirical lit. curve; tunable in principle but only acts inside VRS region + `max()` switch [gslHelper.cpp:55](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L55) |
| VRS trigger `vver/v1∈[0,2]` | [gslHelper.cpp:39](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L39) | structural |
| K distortion slope 0.25 (cap 1) | [propeller.cpp:106](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L106) | dead — DIST 0 under MODEL 1 [params.h:33](../NeuroBEM/code/simulator/include/params.h#L33) |
| Maple coefficient blobs | calculate*.cpp | linear polar, kβ, e, Ib, Mb, θ0, θ1, chord, ρ, R all baked → untunable without re-derivation |
| Ω cutoffs (<10→vi=0, <1→mu=0), ε=1e-6, qags tol 1e-3, v1 brackets −5/20→−20/30 | [gslHelper.cpp:27,66-68](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L27), [propeller.cpp:81,103-104](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L81) | numerics — keep fixed |

## Structural facts that shape the optimization

1. **Flapping/coning are frozen.** Their only inputs are Ω, mu, alpha, vind, p, q — no tunable reaches
   them. Tuning θ0/θ1/chord creates integrand-vs-flapping inconsistency; the paper's own code already
   has this for cl/cd/k ([bem_limitations.md](bem_limitations.md) #7).
2. **Flapping never re-enters the integrals.** T/Q/H and vi are integrated with a0=a1s=b1s=0:
   `setPropellerState` is called with 7 args, angle defaults 0
   ([propeller.cpp:21,38,55](../NeuroBEM/code/simulator/src/simulator/propeller.cpp#L21),
   [gslHelper.h:57-59](../NeuroBEM/code/simulator/include/gslHelper.h#L57)), so the β/U_P flapping terms
   [gslHelper.cpp:179-183](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L179) are dead.
   Paper algorithm step 4 (re-evaluate (13)–(15) *with* the angles,
   [RSS21_Bauersfeld.md:337](../papers/RSS21_Bauersfeld.md#L337)) is **not implemented** — plumbing exists,
   candidate paper-correctness toggle.
3. **Degeneracies to expect:** `b`, `rho`, joint (ci,co), joint (cl,cd) scale all multiply T/Q/H
   identically; `k` is a pure gain on Mxy; only `cxy·A` products are identifiable. A fully-free run
   will sit in flat valleys → fix physical constants (R, b, rho, g), tune shapes/ratios, keep bounds.
4. **Current CMA-ES** ([CMAES.cpp](../NeuroBEM/code/simulator/src/CMAES.cpp)): hand-rolled, N=2 (cl,cd),
   k fixed, force-only normalized loss `Σfmse/sf²` [CMAES.cpp:81-87](../NeuroBEM/code/simulator/src/CMAES.cpp#L81),
   normalized coords x=param/default, box [0.05,5], λ=8, μ=4, σ0=0.3, ≤200 gen, seed 0.
   `st` is already computed [CMAES.cpp:202](../NeuroBEM/code/simulator/src/CMAES.cpp#L202) — joint loss is
   `fm.sum()/sf² + tm.sum()/st²`.
5. **Ground-truth constants** live in CMAES.cpp (MASS 0.772, INERTIA 0.00254/0.00214/0.00436) —
   params.h inertia values on the README mass; consistent with [results.md](results.md) but not with
   README's rounded [0.0025, 0.0021, 0.0043] used by make_nn_targets.py. Keep one convention when comparing.
6. **Upstream bug:** [motor.cpp:24](../NeuroBEM/code/simulator/src/simulator/motor.cpp#L24)
   `torque[3] -= isCW()·dOmega·inertia` — index 3 is out of bounds on `Vector3d` (should be 2), *and*
   dOmega is never set (simulator.cpp reads the domega columns [simulator.cpp:49](../NeuroBEM/code/simulator/src/simulator/simulator.cpp#L49)
   but never calls `setMotorAcceleration`; identical upstream at 126ed98). Numerically inert today, but UB.
   Fix the index; leave domega unwired for paper parity (candidate later param: motor-inertia reaction torque).
7. **Fuselage drag is part of "paper BEM".** Upstream always wrote `getForce()=drag+thrust`
   (verified at import commit 126ed98) — the paper's Table II "BEM" numbers include the crude
   cxy=cz=1 drag. Raw-BEM baseline keeps it.
8. **Robustness/cost:** extreme params can break the vi bracket (printf + garbage
   [gslHelper.cpp:66-74](../NeuroBEM/code/simulator/src/simulator/gslHelper.cpp#L66)) and inflate qags
   runtime — the CMA-ES loss needs non-finite guards and sane bounds.

## Proposed tunable tiers

| tier | params | n | note |
|---|---|---|---|
| T0 raw re-tune | cl, cd, k | 3 | joint force+torque loss; extends `bem-tuned` |
| T1 blade | + theta0, theta1, ci, co | 7 | shape via co/ci; watch scale degeneracy with cl/cd |
| T2 corrections | + cl_offset(0), hforce_scale(1), thrust_scale(1) | 10 | agilicious set as free params, neutral defaults = raw BEM |
| T3 assembly/body | + dx, dy, dz, cxy·Ax, cxy·Ay, cz·Az | 16 | torque levers + body drag |
| fixed | rho, R, b, A, g, mass/inertia, numerics, VRS quartic | — | measured constants / target-side / degenerate |

## Code state (2026-07-16)

Working tree = `bem-agi` configuration: `+0.07` and `×3.0` live (commit e39651d),
[applyBM.sh:8-9](../NeuroBEM/code/Scripts/applyBM.sh#L8) passes cl=4.797, cd=4.169.
Revert both lines + applyBM defaults before regenerating any raw-BEM baseline
(see warning in [results.md](results.md)).
