function [T, a0, a1s, b1s, Q] = propeller_state_numint(v, att_rate, Omega)
%propeller_state_numint: the function calculates propeller data using BEM
% [T, a0, a1s, b1s, Q] = propeller_state_numint(v, att_rate, Omega) where
%    v          velocity vector in the body-frame of the propeller (z downwards)
%    att_rate   roll, pitch and yaw-rate of the vehicle
%    Omega      angular velocity of the propeller
% The function return the numerically integrated solution for
%    T          thrust of the propeller
%    a0         coning angle of the propeller
%    a1s        lateral flapping angle
%    b1s        longitudinal flapping angle
    param = setParam();
    K = param.K;
    mu = calc_mu(v, Omega);
    v1 = calc_v1(v, Omega);
    alpha_s = calc_alpha_s(v);
    a0 = calc_a0(Omega, v1, mu, alpha_s, K, att_rate);
    a1s = calc_a1s(Omega, v1, mu, alpha_s, K, att_rate);
    b1s = calc_b1s(Omega, v1, mu, alpha_s, K, att_rate);
    T = calc_T_numInt(v, K, mu, Omega, v1, a0, a1s, b1s);
    Q = calc_Q_numInt(v, K, mu, Omega, v1, a0, a1s, b1s);
end
