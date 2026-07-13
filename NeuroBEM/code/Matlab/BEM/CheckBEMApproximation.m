clear all;
close all;
% clc;

addpath subroutines
addpath ../Common/

basepath = "../../ExampleData/BEM/";
meas_thrust_torque = "Ex_AllMeasurements.csv";
meas_coning = "Ex_Coning.csv";

global param;
param = setParam(); 

%% Read in previous data
data = csvread(basepath + meas_thrust_torque ,1);
coning = csvread(basepath + meas_coning,1);

%% Check fitting quality
att_rate = [0,0,0];
v = [0, 0, 0];
K = 0;

omegas = [1:100:3000]';
fun_int = @(Omega) propeller_state_numint(v, att_rate, Omega);
[T, a0, a1s, b1s, Q] = arrayfun(fun_int, omegas);
a0 = asin(sin(a0) * (1-param.ef));

figure;
subplot(1,3,1);
hold on
plot(data(:,1), data(:,5), 'bx');
plot(omegas, T, 'r-');
xlabel('Omega [rad/s]');
ylabel('Thrust [N]');
grid on
title('Thrust: fit slope of lift curve');
legend('Exp. Data', "Fit: cl = " + num2str(param.cl, "%4.2f") + " [1/rad]", 'Location', 'northwest');

subplot(1,3,2);
hold on
plot(data(:,1), data(:,8), 'bx');
plot(omegas, Q ,'r-');
xlabel('Omega [rad/s]');
ylabel('Torque [Nm]');
grid on
title('Torque: fit drag coefficient');
legend('Exp. Data', "Fit c_d = " + num2str(param.cd, "%4.2f") , 'Location', 'northwest');


subplot(1,3,3);
hold on
plot(coning(:,1), coning(:,2)*180/pi, 'bx');
plot(omegas, a0*180/pi ,'r-');
xlabel('Omega [rad/s]');
ylabel('Coning [deg]');
grid on
title('Coning: fit stiffness');
legend('Exp. Data', "Fit k_\beta = " + num2str(param.k_beta, "%4.2f") + " [Nm/rad]" , 'Location', 'northwest');
ylim([0, 0.7])