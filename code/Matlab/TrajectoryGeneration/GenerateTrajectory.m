clear all;
close all;
clc;

% This script calculates a trajectory based on an analytic representation
% of the path that is to be flown. This position function needs to be input
% into f_pos.
% The trajectory has different velocity options. First, it can be scaled by
% the factor vel_scale which has _no_ real meaning. It's just a number so
% play with it until you are happy with the result.
% The "scale" parameter determines how the velocity profile changes.
% Consider a circle which is parametrized by time (and thus also arc
% length). The velocity of the vehicle over time will be
%   - constant if "none" or "constant" are selected. "None" is equal to
%           constant with vel_scale=1
%   - linear increase/decrease if "linear" is selected
%   - parabolic if "sqrt" is selected
%   - nearly constant but with a short ramp-up/ramp-down phase if "aggr" is
%           selected




%% Initialization stuff (don't change)
addpath subroutines
addpath ../Common/
syms t

%% 1. Set up analytic representation of the desired trajectory
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% User Inputs %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEBUGPLOTS = false;
MINIMIZE_YAW = true;
WRITE=true;

duration = 30;  % s
dt = 0.01;      % s
filename = "Ex_Ellipse.csv";
basepath = "";

% scale: how the velocity grows over time, e.g. sqrt means that the drone will
% spend a lot of time near peak velocity and accelerate quickly.
scale = "aggr"; % "none", "constant", "linear", "sqrt", "aggr"
vel_scale = 2.4;  % maximum amout of scaling that is applied
radius = 6;

% applies desired time scaling
tadj = timeScaler(duration, scale, vel_scale, DEBUGPLOTS);

% Note: the expression must be symbolic! Below are some example
% trajectories that can be tried out
% wobbly circle
% f_pos = [radius*sin(tadj(t))*(1+0.3*sin(4*tadj(t))) + 3.5, ...
%          radius*cos(tadj(t))*(1+0.3*sin(4*tadj(t))), ...
%         2];
    
% Linear oscillation across the diagonal
% f_pos = [radius * sin(tadj(t)) + 3.5, ...
%          radius * sin(tadj(t)), ...
%          2];

% Ellipse
f_pos = [4/3*radius*sin(tadj(t))+3,  ...
         radius*cos(tadj(t))+1, ...
         2];

% Gerono lemniscate
% f_pos = [radius*cos(tadj(t))+2.5,  ...
%          -2*radius*sin(2*tadj(t))/3+1, ...
%          2];

% Vertical oscillations
% f_pos = [0,  ...
%          1.5 + 0.75 * sin(tadj(t)), ...
%          3.5 - radius/2 * cos(tadj(t))];
            
f_yaw = [0*t];

quad = setupKingfisher();


%% 2. calculate up to fourth derivative and numerically evaluate the trajectory
%%%%%%%%%%%%%% Do not change below this point as user input %%%%%%%%%%%%%%%%%%%
time = [0:dt:duration]';
N = length(time);

f_xyz=calculateDerivatives(f_pos);
f_yaw=calculateDerivatives(f_yaw, 1);
traj_xyz=evaluateTrajectory(time, f_xyz, false);
traj_yaw=evaluateTrajectory(time, f_yaw, false);
   

%% 3. compute the attitude dynamics
gravity = 9.81;
acc_vec = squeeze(traj_xyz(3,:,:))';
acc_vec(:,3) = acc_vec(:,3) + gravity;
body_z = acc_vec ./ vecnorm(acc_vec,2,2);

thrust = quad.mass * vecnorm(acc_vec,2,2);

% compute attitude according to:
% https://math.stackexchange.com/questions/2251214/calculate-quaternions-from-two-directional-vectors
q = quaternion([1 + body_z(:,3) -body_z(:,2) body_z(:,1) zeros(N,1)]);
q = normalize(q);
omega = diffQuaternionToOmega(q, time);

% Elia special yaw hack, 5 iterations
if MINIMIZE_YAW
    for i = 1:5
        yaw_correction_cum = cumtrapz(time,-omega(:,3));
        q_correction = quaternion([cos(yaw_correction_cum/2), ...
                                   zeros(N, 2),               ...
                                   sin(yaw_correction_cum/2)]);
        qmult = @(p, q) p * q;
        q = arrayfun(qmult, q, q_correction);
        omega = diffQuaternionToOmega(q, time);
    end
end

omega_dot = [gradient(omega(:,1), time), ...
             gradient(omega(:,2), time), ...
             gradient(omega(:,3), time)];

% compute body torques: tau = J*w_dot + w x J*w
tau = omega_dot .* quad.inertia + cross(omega', quad.J*omega')';

% Compute reference inputs and convert to normalized thrusts [0-1]
uref = (quad.CA\[thrust tau]')';
uref = uref / quad.thrust_max;

trajectory = [time,                      ...
              squeeze(traj_xyz(1,:,:))', ...
              compact(q),                ...
              squeeze(traj_xyz(2,:,:))', ...
              omega,                     ...
              squeeze(traj_xyz(3,:,:))', ...
              omega_dot,                 ...
              uref];





%% 4. validate computed trajectory
figure;
subplot(2,1,1);
hold on
grid on
plot(time, uref(:,1), 'r');
plot(time, uref(:,2), 'g');
plot(time, uref(:,3), 'b');
plot(time, uref(:,4), 'k');
title("Relative Motor Commands");
xlabel("Time[s]");
ylabel("Motor Command");
legend("Motor 1", "Motor 2", "Motor 3", "Motor 4");
axis tight;
subplot(2,1,2);
hold on
grid on
plot(time, omega(:,1), 'r');
plot(time, omega(:,2), 'g');
plot(time, omega(:,3), 'b');

figure;
subplot(3,2,[1 3 5]);
plot3(  squeeze(traj_xyz(1,1,:)), ...
    squeeze(traj_xyz(1,2,:)), ...
    squeeze(traj_xyz(1,3,:)));
grid on
title("3D Position");
xlabel("X [m]");
ylabel("Y [m]");
zlabel("Z [m]");

ax(1) = subplot(3,2,2);
hold on
plot(time, squeeze(traj_xyz(1,1,:)), 'r');
plot(time, squeeze(traj_xyz(1,2,:)), 'g');
plot(time, squeeze(traj_xyz(1,3,:)), 'b');
legend("x", "y", "z");
grid on;
ylabel("Position [m]");


ax(2) = subplot(3,2,4);
hold on
plot(time, squeeze(traj_xyz(2,1,:)), 'r');
plot(time, squeeze(traj_xyz(2,2,:)), 'g');
plot(time, squeeze(traj_xyz(2,3,:)), 'b');
plot(time, vecnorm(squeeze(traj_xyz(2,1:3,:)),2,1), 'k');
legend("x", "y", "z", "||.||");
grid on;
ylabel("Velocity [m/s]");


ax(3) = subplot(3,2,6);
hold on
plot(time, squeeze(traj_xyz(3,1,:)), 'r');
plot(time, squeeze(traj_xyz(3,2,:)), 'g');
plot(time, squeeze(traj_xyz(3,3,:)), 'b');
plot(time, vecnorm(squeeze(traj_xyz(3,1:3,:)),2,1), 'k');
legend("x", "y", "z", "||.||");
grid on;
xlabel("Time [s]");
ylabel("Acceleration [m/s^2]");

linkaxes(ax, 'x');

%% 5. write to csv
if WRITE
    header = ["t",                              ... % 1
              "p_x", "p_y", "p_z",              ... % 2-4
              "q_w", "q_x", "q_y", "q_z",       ... % 5-8
              "v_x", "v_y", "v_z",              ... % 9-11
              "w_x","w_y", "w_z",               ...
              "a_lin_x", "a_lin_y", "a_lin_z",  ...
              "a_rot_x", "a_rot_y", "a_rot_z",  ...
              "u_1", "u_2", "u_3", "u_4"];
    write_csv(basepath + filename, trajectory, header);
    trajectory(1,2:4)
end




