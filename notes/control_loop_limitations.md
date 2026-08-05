# Control-loop & latency limitations (paper Discussion, points i & ii)

Plain-language walkthrough of the two limitations the paper raises in [RSS21_Bauersfeld.md:583-604](../papers/RSS21_Bauersfeld.md#L583): the BetaFlight inner loop is the wrong place to cut the control loop, and the simulator has no latency. Both inflate the Table III numbers for reasons that are not aerodynamics.

## 0. Vocabulary

| term | meaning here |
|---|---|
| motor speed Ω | rad/s of one propeller. The physical quantity that makes force; the input the aerodynamic model actually eats |
| aerodynamic model | (velocity, body rates, 4×Ω) → force + torque. BEM+NN |
| simulator | steps the drone forward: model → Newton → new state, symplectic Euler @ 1 ms ([RSS21_Bauersfeld.md:~408](../papers/RSS21_Bauersfeld.md#L400)) |
| outer loop / MPC | 100 Hz, on a laptop. Decides "tilt this fast, push this hard" |
| inner loop / low-level controller | 1 kHz, on the flight-controller board. Makes that happen. Here: BetaFlight, firmware written for human racing pilots |
| body rate | rotation speed about the drone's own 3 axes (rad/s). What a pilot's sticks command; what the MPC commands |
| collective thrust | total push of all 4 props as one number. The throttle stick |
| throttle | normalized 0–1 number internal to the flight controller. Not physical — a radio-transmitter convention |
| PID | feedback recipe: out = P·err + I·∫err + D·d(err)/dt, with err = desired − measured body rate |
| gyro | onboard sensor measuring body rate; feeds the PID's "measured" term |
| mixer / allocation | (thrust, 3 torques) → 4 per-motor commands, via a fixed matrix |
| ESC / DSHOT | circuit driving the motor / digital protocol to command it. You send a *command number*, not a speed |
| low-pass filter | smooths a signal. Cannot smooth without delaying |
| latency (dead time) | delay between something happening and the controller reacting to it |
| motion capture (Vicon) | room cameras giving pose at 400 Hz ([RSS21_Bauersfeld.md:395](../papers/RSS21_Bauersfeld.md#L395)). The indoor "GPS" |

## 1. What physically happens in a real flight

Chain per [RSS21_Bauersfeld.md:385-390](../papers/RSS21_Bauersfeld.md#L385) and [:394-400](../papers/RSS21_Bauersfeld.md#L394):

1. Cameras see the drone → Vicon computes pose @ 400 Hz and **filters** it (smoothing ⇒ delay).
2. Pose goes over the network to a **laptop**.
3. **MPC** @ 100 Hz on the laptop → 1 collective thrust + 3 body rates.
4. Those 4 numbers go over a **Laird radio** to a **Jetson TX2** on the drone.
5. Jetson forwards over a wire to the flight-controller board running **BetaFlight**.
6. BetaFlight @ 1 kHz: read gyro → low-pass → PID on body-rate error → 3 desired torques; thrust → throttle; mixer → 4 per-motor throttles → 4 DSHOT command numbers.
7. Each **ESC** spins its motor to whatever speed that number happens to produce *at the current battery voltage*.
8. Props spin at some actual Ω → air makes force and torque → drone moves. Back to 1.

The 4 numbers leaving the MPC (step 3) and the 4 motor speeds that actually occur (step 8) are separated by a radio hop, a serial hop, a filtered PID, a mixer, and a voltage-dependent ESC response. **Nobody in the loop ever says "motor 2 should spin at 2100 rad/s."**

The whole conversion is visible in the agilicious port: thrust→sbus→throttle ([low_level_controller_betaflight.cpp:63-66](../agilicious/simulator/low_level_controller_betaflight.cpp#L63)), PID with two low-pass filters ([:69-79](../agilicious/simulator/low_level_controller_betaflight.cpp#L69), filters built at [:14-17](../agilicious/simulator/low_level_controller_betaflight.cpp#L14)), mixer ([:81-83](../agilicious/simulator/low_level_controller_betaflight.cpp#L81)), and finally DSHOT-command → Ω ([:87-92](../agilicious/simulator/low_level_controller_betaflight.cpp#L87)):

```
omega = omega_offset + omega_volt·voltage + omega_cmd_lin·cmd + omega_cmd_sqrt·√cmd
```

That curve is **fitted from bench measurements** — a model, with model error, sitting between controller and physics.

## 2. What the Table III experiment actually measures

Not sim and reality flying simultaneously. The procedure is:

1. **Real flight, once, recorded.** Fly a trajectory with the real stack above; record true position over time.
2. **Offline, simulate the same flight.** Same initial state, same trajectory to follow; the simulated drone flies *itself*: simulated MPC → simulated BetaFlight → simulated motors → aerodynamic model → integrate. BetaFlight runs as a software module inside the simulator with the same gains ([RSS21_Bauersfeld.md:399](../papers/RSS21_Bauersfeld.md#L399)).
3. **Compare paths.** Table III = accumulated position difference between simulated and recorded real flight.

Implicit claim: a perfect aerodynamic model ⇒ identical paths. **Catch: the position gap measures every sim-vs-real difference, not just aerodynamics.** A 2% mismatch in the simulated BetaFlight shows up as position error and gets charged to the aerodynamic model. Points (i) and (ii) are the two biggest such contaminants.

## 3. Point (i) — BetaFlight is the wrong place to cut the loop

**(a) The interface is narrow and carries the wrong quantity.** The MPC can only say "total push + 3 rotation rates." The aerodynamic model's input is 4 motor speeds. Between them sits the translation layer (PID → mixer → DSHOT→Ω curve). In reality that translation is done by real firmware and real ESCs on a battery that sags as it drains; in simulation by the fitted formula above at an assumed voltage. They will not agree exactly, so the simulated props spin at slightly different speeds than the real ones did — and from that moment the flights diverge regardless of how good the aerodynamic model is.

Hence the proposed fix: **MPC outputs 4 motor speeds, an inner loop closes on measured RPM.** Then the commanded quantity *is* the model's input, the translation layer disappears, sim and hardware are fed the identical thing, and the leftover position difference really is aerodynamics.

**(b) BetaFlight adds dynamics that are not the drone.** It low-passes the gyro and the D-term, and smooths/interpolates the 100 Hz setpoint up to its 1 kHz loop. Each filter delays and reshapes. So the controlled object is no longer "a drone" but "a drone plus filters" — a system that lags, overshoots, can ring. That is what *undesirable control-loop shaping* means: the loop's dynamic character is set by firmware conveniences for humans, not by physics or by design. For a simulation study this is poison, since the filters must be replicated exactly. They did what they could: feed-forward terms disabled, reduced to a plain PID with identical gains in sim and real ([RSS21_Bauersfeld.md:396-399](../papers/RSS21_Bauersfeld.md#L396)). The filters remain.

**(c) Why it makes their results look worse.** The contamination is identical across every row of Table III — their model and all baselines. It is an error floor. Remove it and the rows separate more cleanly in their favour, because what remains is aerodynamic error, where BEM+NN's advantage lives.

## 4. Point (ii) — no latency in sim; dirty Ω measurement

**(a) Latency ⇒ the simulator is unrealistically good.** Real chain: cameras → Vicon filtering → network → laptop MPC → Laird radio → Jetson → serial → BetaFlight. The command acting at time *t* was computed from where the drone was some ms earlier. A feedback loop acting on stale state overshoots, over-corrects, oscillates; at 10+ m/s the drone moves centimetres during that delay.

In the simulator all of it is **zero** — the simulated MPC reads the exact current unfiltered state and its command takes effect instantly. The simulated drone is therefore a *better-behaved* drone: tighter tracking, better damping, no overshoot where the real one had it. The two paths diverge for reasons that have nothing to do with air, and Table III charges it to the aerodynamic model. Modelling the delays (buffer the pose, buffer the command, add transmission lag) makes the simulated drone misbehave the same way the real one does; errors drop for everyone, most for the model with the least remaining error of its own.

**(b) Dirty Ω ⇒ a floor under Table II.** Different table, different failure. Table II is single-step accuracy: one recorded state in, predicted force/torque out, compared to measurement — no loop, no controller. Its input includes the 4 motor speeds, measured onboard by BetaFlight @ 1 kHz via ESC telemetry ([RSS21_Bauersfeld.md:396-397](../papers/RSS21_Bauersfeld.md#L396)). That measurement is coarse, noisy, and on a clock that disagrees with the cameras'. The pipeline repairs it: 4th-order Butterworth at the motor time constant, plus offset and **~2.4% clock-skew** estimated by correlating gyro against the fitted spline ([RSS21_Bauersfeld.md:412-425](../papers/RSS21_Bauersfeld.md#L412)).

Repair ≠ restoration. What remains is a slightly wrong Ω. Force scales ≈ Ω², so a small Ω error is a not-so-small force error — and it feeds *both* BEM and the NN, plus the first-order motor lag τ = 33 ms ([params.h:153](../NeuroBEM/code/simulator/include/params.h#L153)). Part of the 0.352 N in Table I/II is the model being wrong and part is the *input* being wrong. No architecture crosses that floor. A custom low-level controller doing genuine closed-loop RPM control would log Ω cleanly, fast, and correctly timestamped, improving Table II without touching the model.

## 5. Consequences for this project

We replay the public dataset and never fly, so neither problem is ours to fix. Two decisions follow:

- **Table II is the honest target, and it has a floor** that is partly Ω measurement noise. Don't chase RMSE below 0.352 N / 5.3e-3 Nm, and don't read a small shortfall as a modelling failure.
- **Keep BetaFlight in the loop, with matching gains, for the closed-loop port.** Agilicious ships it ([low_level_controller_betaflight.cpp](../agilicious/simulator/low_level_controller_betaflight.cpp)). A cleaner controller would give prettier trajectories and numbers that cannot be compared to Table III, whose values include the BetaFlight contamination by construction.
