% THIS SCIRPT CAN BE USED TO IDENTIFY THE DRAG AND THRUST COEFFICIENT OF
% THE PROPELLER BASED ON THE TESTSTAND DATA. BLADE-ELEMENT-THEORY IS USED.
% NOTE: TAKES FOREVER TO RUN!

clear all;
close all;
% clc;

addpath subroutines
addpath ../Common/

basepath = "../../ExampleData/BEM/";
meas_thrust_torque = "Ex_AllMeasurements.csv";
meas_coning = "Ex_Coning.csv";

global param;
global linearMode;
param = setParam(); 

%% Read in previous data
data = csvread(basepath + meas_thrust_torque ,1);
coning = csvread(basepath + meas_coning,1);

%% Fit data


% The static thrust of the propeller on a non-moving test stand depends on
% the parameters 

linearMode = true;
fit_cl_cd = @(x) fit_cl_cd_data(data, x(1), x(2));
tmp = fminsearch(fit_cl_cd, [15, 13]);
param.cl = tmp(1);
param.cd = tmp(2);
fprintf("Linear Lift coefficient: %6.3f\nLinear Drag coefficient: %6.3f\n", ...
        tmp(1), tmp(2));

error("If you are wondering what just happend, go read the docs again");

linearMode = false;
fit_cl_cd = @(x) fit_cl_cd_data(data, x(1), x(2));
tmp = fminsearch(fit_cl_cd, [15, 13]);

fit_kbeta = @(k) fit_k_data(coning, k);
kbeta_opt = fminsearch(fit_kbeta, 1);
param.k_beta = kbeta_opt;

fprintf("Lift coefficient: %6.3f\nDrag coefficient: %6.3f\n" ...
        + "k_beta:           %6.3f\n", tmp(1), tmp(2), kbeta_opt);
    
%% Function Definitions
function err = fit_cl_cd_data(data, a, cd)
    err = fit_a_data(data, a, cd)/167.0815 + fit_cd_data(data, a, cd)/0.0242;
end

function err = fit_a_data(data, a, cd)
    n = length(data);
    omega = data(:,1);
    T = data(:,5);
    T_eval = zeros(n,1);
    for i = 1:n
        T_eval(i) = T_calc(omega(i,1), T(i,1), a, cd);
    end
    err = sum((T-T_eval).^2);
end

function err = fit_cd_data(data, a, cd)
    n = length(data);
    omega = data(:,1);
    T = data(:,5);
    Q = data(:,8);
    Q_eval = zeros(n,1);
    for i = 1:n
        Q_eval(i) = Q_calc(omega(i,1), T(i,1), a, cd);
    end
    err = sum((Q-Q_eval).^2);
end

function err = fit_k_data(coning, k)
    global param;
    param.k_beta = k;
    n = length(coning);
    v = [0, 0, 0];
    att_rate = [0, 0, 0];
    K = 0;
    alpha_s = 0;
    mu = 0;
    omega = coning(:,1);
    a0 = coning(:,2);
    a0_eval = 0*a0;
    for i = 1:n
        v1 = calc_v1(v, omega(i));
        a0_eval(i) = calc_a0(omega(i), v1, mu, alpha_s, K, att_rate, k);
    end
    a0_eval = asin(sin(a0_eval)*(1-param.ef));
    err = sum((a0-a0_eval).^2);
end

function T_c = T_calc(Omega, T, aval, cdval)
    global param;
    global linearMode;
    v = [0, 0, 0];
    K = param.K;
    a0 = 0;
    a1s = 0;
    b1s = 0;
    v1 = sqrt(T/(2*param.rho*param.A));
    mu = 0;
    
    f_int_T = @(r, Psi) diffThrust(r,Psi);
    T_c = param.b * param.rho / (4*pi) * ...
        integral2(f_int_T, 0, param.R, 0, 2*pi);
    
    function dT = diffThrust(r, Psi)
        sPsi = sin(Psi);
        cPsi = cos(Psi);
        beta = a0 -  a1s.*cPsi - b1s.*sPsi;
        U_T = Omega .* (r + param.R.*mu.*sPsi);
        U_P = v(3) - v1*(1+K*r./param.R.*cos(Psi)) ...
             - r.*Omega.*(a1s*sPsi + b1s*cPsi) ...
             - v(3)*beta.*cPsi;
        phi = atan2(U_P,U_T);
        alpha = param.theta_0 + param.theta_1 .* r./param.R + phi; 
        if linearMode
            cd = cdval;
            cl = aval .* alpha;
            c = param.c;
        else
            cl = aval.*sin(alpha).*cos(alpha);
            cd = cdval .* sin(alpha).^2;
            c = param.ci + r/param.R * (param.co - param.ci);
        end
        U2 = U_T.^2 + U_P.^2;
        dL = U2 .* cl .* c;
        dD = U2 .* cd .* c;
        dT = dL .* cos(phi) + dD .* sin(phi);
   end
    
end

function Q_c = Q_calc(Omega, T, aval, cdval)
    global param;
    global linearMode;
    v = [0, 0, 0];
    K = param.K;
    a0 = 0;
    a1s = 0;
    b1s = 0;
    v1 = sqrt(T/(2*param.rho*param.A));
    mu = 0;
    
    f_int_Q = @(r, Psi) diffTorque(r,Psi);
    Q_c = param.b * param.rho /(4*pi) * integral2(f_int_Q, 0, param.R, 0, 2*pi);
    
    function dQ = diffTorque(r, Psi)
        sPsi = sin(Psi);
        cPsi = cos(Psi);
        beta = a0 -  a1s.*cPsi - b1s.*sPsi;
        U_T = Omega .* (r + param.R.*mu.*sPsi);
        U_P = v(3) - v1*(1+K*r./param.R.*cos(Psi)) ...
             - r.*Omega.*(a1s*sPsi + b1s*cPsi) ...
             - v(3)*beta.*cPsi;
        phi = atan2(U_P,U_T);
        alpha = param.theta_0 + param.theta_1 .* r./param.R + phi; 
        if linearMode
            cd = cdval;
            cl = aval .* alpha;
            c = param.c;
        else
            cl = aval.*sin(alpha).*cos(alpha);
            cd = cdval .* sin(alpha).^2;
            c = param.ci + r/param.R * (param.co - param.ci);
        end
        U2 = U_T.^2 + U_P.^2;
        dL = U2 .* cl .* c;
        dD = U2 .* cd .* c;
        dQ = r .* (-dL .* sin(phi) + dD .* cos(phi));
   end
end



