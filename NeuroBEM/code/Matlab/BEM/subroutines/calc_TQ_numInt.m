function [T, Q] = calc_TQ_numInt(v, K, mu, Omega, v1, a0, a1s, b1s)
%calc_TQ_numInt calculates thrust and torque using numerical integration
% [T, Q] = calc_TQ_numInt(v, K, mu, Omega, v1, a0, a1s, b1s)  where
%   v       free-stream velocity of the propeller in body-frame
%   K       thrust-distortion factor (usually 0)
%   mu      advance ratio (|v|/(Omega*R))
%   Omega   angular velocity of the propeller in rad/s
%   v1      induced velocity
%   a0      coning angle
%   a1s     lateral flapping angle
%   b1s     longitudinal flapping angle
% Returns
%   T       numerically integrated thrust
%   Q       numerically integrated drag torque
    T = calc_T_numInt(v, K, mu, Omega, v1, a0, a1s, b1s);
    Q = calc_Q_numInt(v, K, mu, Omega, v1, a0, a1s, b1s);
end