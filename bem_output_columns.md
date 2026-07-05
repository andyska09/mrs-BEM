# BEM output column structure (`processed_data/bem/bem_*.csv`)

47 columns, no header. Cols 1–29 = the processed-data input (see `Readme.md`);
cols 30–47 are appended by the simulator (`code/simulator`). 1-indexed.

| Col | Quantity |
|-----|----------|
| 1 | time [s] |
| 2–4 | angular acceleration body x,y,z [rad/s^2] |
| 5–7 | angular velocity body x,y,z [rad/s] |
| 8–11 | quaternion qx, qy, qz, qw |
| 12–14 | acceleration body x,y,z [m/s^2] (includes gravity) |
| 15–17 | velocity body x,y,z [m/s] |
| 18–20 | position x,y,z [m] |
| 21–24 | motor speed 1,2,3,4 [rad/s] |
| 25–28 | derivative motor speed 1,2,3,4 [rad/s^2] |
| 29 | battery voltage [V] |
| 30–32 | **predicted force** body x,y,z [N] |
| 33–35 | **predicted torque** body x,y,z [Nm] |
| 36–47 | **diagnostics**, interleaved per motor: `[v_i, mu, alpha_s]` × 4 |

### Diagnostics detail (cols 36–47)
Per motor: induced velocity `v_i` [m/s], advance ratio `mu` [-], shaft angle of attack `alpha_s` [rad].

| Motor | v_i | mu | alpha_s |
|-------|-----|----|---------|
| 1 | 36 | 37 | 38 |
| 2 | 39 | 40 | 41 |
| 3 | 42 | 43 | 44 |
| 4 | 45 | 46 | 47 |

0-indexed slices: `v_i = [35,38,41,44]`, `mu = [36,39,42,45]`, `alpha_s = [37,40,43,46]`.
