function param=setParam()
%setParam sets all required parameters for the BEM model

% Physics
param.g = 9.81;                     % gravitational acceleration
param.rho = 1.204;                  % ISA reference air density at sea level

% Propeller
param.R = 5.1*2.54/2 * 1E-2;        % diameter: 5.1 inch (datasheet)
param.A = pi*param.R^2;             % Area of propeller disc 
param.ef = 0.1;                     % Put a number that gives senseful results, somewhere between 0.05 and 0.2
param.e = param.ef*param.R;         
param.sigma = 0.215;                % solidity ratio of the prop
param.theta_0 = 21.77*pi/180;       % pitch angle theta0 of the propeller [deg] (datasheet)
param.theta_1 = -11*pi/180;         % blade twist (measured)
param.a = 8.962;                     % Linear slope of the lift curve (for angles)
param.d = 0.0833;                   % Constant drag (for angles)
param.b = 3;                        % number of blades
param.c = 1.3E-2;                   % mean chord [m] (measured, approx)
param.ci = 1.7E-2;                  % inner chord [m] (measured, approx)
param.co = 0.7E-2;                  % outer chord [m] (measured, approx)
param.mb = 1.22E-3;                 % mass of one blade (measured)
param.cg = 2.7E-2;                  % distance between blade CoG and root of the blade
param.Ib = param.mb * param.cg^2;   % blade moment of inertia around hinge (estimate)
param.Ib = param.Ib + 1/12*param.mb*((param.R/2)^2); % add moment of lenghty rod
param.Mb = param.mb * param.g * param.cg;            % moment of blade around hinge (estimate)
param.k_beta = 7.571;               % in [N*m/rad, based on coning
param.K = 0;                        % thrust distortion factor
param.cl = 13.848;
param.cd = 14.030;

param.opt_fzero = optimset('fzero');
param.opt_fzero.TolX = 1E-3;
param.opt_fzero.Display = 'off';

end

% Linear Lift coefficient:  8.962
% Linear Drag coefficient:  0.083
% Lift coefficient:  8.962
% Drag coefficient:  0.083
% k_beta:            7.571