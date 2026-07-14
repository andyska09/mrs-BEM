# Data sources of the merged/segment CSV

Per-column provenance of the 29-col `merged_*_seg_*.csv` training files.
Assembly: [MergeAndPreprocessData.m:276-286](../NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L276), headers [:290-297](../NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L290).

| Col | Header | Source | Derivation |
|-----|--------|--------|------------|
| 1 | `t` | shared | aligned time base (Betaflight clock, drift-corrected to Vicon) |
| 2-4 | `ang acc x/y/z` | **Betaflight gyro** | d/dt of smoothed gyroADC |
| 5-7 | `ang vel x/y/z` | **Betaflight gyro** | SG/spline-smoothed gyroADC |
| 8-11 | `quat x/y/z/w` | **OptiTrack** | smoothed pose orientation |
| 12-14 | `acc x/y/z` | **OptiTrack** | 2nd deriv of pose position → rotated to body ([:191](../NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L191)) |
| 15-17 | `vel x/y/z` | **OptiTrack** | 1st deriv of pose position → rotated to body ([:192](../NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L192)) |
| 18-20 | `pos x/y/z` | **OptiTrack** | smoothed pose position |
| 21-24 | `mot 1-4` | **Betaflight** | motor eRPM → rad/s, 4th-order Butterworth ([log_smoother.m:127-134](../NeuroBEM/code/Matlab/OptiTrack/subroutines/log_smoother.m#L127)) |
| 25-28 | `dmot 1-4` | derived | gradient of motor speed ([:284](../NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L284)) |
| 29 | `vbat` | **Betaflight** | battery voltage |

## Key points

- **Angular velocity AND angular acceleration come from the Betaflight gyro** (gyroADC), SG/spline-smoothed — NOT from OptiTrack. Their own comment at [:277](../NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L277) says "betaflight". gyroADC input map: [log_smoother.m:30-32](../NeuroBEM/code/Matlab/OptiTrack/subroutines/log_smoother.m#L30); output cols 2-7: [log_smoother.m:46-51](../NeuroBEM/code/Matlab/OptiTrack/subroutines/log_smoother.m#L46), [:97-110](../NeuroBEM/code/Matlab/OptiTrack/subroutines/log_smoother.m#L97).
- OptiTrack DOES compute a quaternion-derivative `ω` ([bag_smoother.m:138](../NeuroBEM/code/Matlab/OptiTrack/subroutines/bag_smoother.m#L138)) — it lands in the intermediate `data` array cols 24-26 and the raw 45-col dump, but is **never written to the merged/segment CSV**.
- OptiTrack gyro is used **only for time-sync** (cross-correlation with the log) via [align_data.m](../NeuroBEM/code/Matlab/OptiTrack/subroutines/align_data.m), called at [:147-154](../NeuroBEM/code/Matlab/OptiTrack/MergeAndPreprocessData.m#L147). This is what README's "filtered using both onboard and Vicon" means: value = gyro, sync = Vicon.
