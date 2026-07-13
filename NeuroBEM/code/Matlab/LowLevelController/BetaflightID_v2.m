close all;
clear all;
clc

% To enable more than a few debug plots, set PLOT to true
% To write the identified PID gains to disk, set write to true and adjust
% the filename 'fname' accordingly.

addpath ../Common/
addpath ../OptiTrack/subroutines

basepath = "../../ExampleData/OptiTrack/";
list = [
            "Ex_OptiTrack";
          ];

PLOT = false;
WRITE = false;
fname = "gain_matrix.csv"

% SBUS parameters
max_thrust = 8.5;
mass = 0.772;


%% Read in Data
data = [];
for i = 1:length(list)
    cmdfile = "command_" + list(i) + ".csv";
   tmp = csvread(basepath + cmdfile,1);
   if ~isempty(data)
       tmp(:,1) = tmp(:,1) + data(end,1);
   end
   data = [data; tmp];
end
dt = mean(diff(data(:,1)));
fs = 1/dt;

data(:,1) = data(:,1) - data(1,1);

% Define some filters
biquad = @(x, fc) apply_biquad(x, fc, fs);
pt1 = @(x, fc) apply_pt1(x, fc, fs);

%% Invert motor allocation to get torques and thrust command
B = [1 -1  1  1; 
     1 -1 -1 -1;
     1  1  1 -1;
     1  1 -1  1];
 B_inv = inv(B);

mot_pwm_min = 48;
mot_pwm_max = 2047;
tlmn = (B_inv * ((data(:,28:31)-mot_pwm_min)/mot_pwm_max)'.^2)';


%% Find P, I, D, FF coefficients
errPID = @(ref, pid, coeff) mean(abs(sum(pid.*coeff,2)-ref).^2);
errPI = @(ref, pi, coeff) mean(abs(sum(pi.*coeff,2)-ref).^2);

rollArr =  [data(:,9)  data(:,12) data(:,15)];
pitchArr = [data(:,10) data(:,13) data(:,16)];
yawArr =   [data(:,11) data(:,14)           ];

coeffsRoll =  fminsearch(@(coeffs) errPID(tlmn(:,2), rollArr,  coeffs), 1e-3*[1 1 1]);                   
coeffsPitch = fminsearch(@(coeffs) errPID(tlmn(:,3), pitchArr, coeffs), 1e-3*[1 1 1]);
coeffsYaw =   fminsearch(@(coeffs) errPI(tlmn(:,4),  yawArr,   coeffs), 1e-3*[1 1]);

[coeffsRoll * 1e3; coeffsPitch * 1e3; coeffsYaw(1) * 1e3 0 coeffsYaw(2)*1e3]
coeffsRoll = [1e-3, 1e-3, 1e-3];
coeffsPitch = [1e-3, 1e-3, 1e-3];
coeffsYaw = [1e-3, 1e-3];

if PLOT
    figure;
    subplot(3,1,1);
    hold on;
    grid on;
    plot(data(:,1), tlmn(:,2), 'k', 'LineWidth', 1);
    plot(data(:,1), sum(rollArr .* coeffsRoll,2), 'r', 'LineWidth', 1);
    ylabel("Roll Torque");
    title("PID Plot");
    legend("Measurment", "Model");
    subplot(3,1,2);
    hold on;
    grid on;
    plot(data(:,1), tlmn(:,3), 'k', 'LineWidth', 1);
    plot(data(:,1), sum(pitchArr .* coeffsPitch,2), 'r', 'LineWidth', 1);
    ylabel("Pitch Torque");
    legend("Measurment", "Model");
    subplot(3,1,3);
    hold on;
    grid on;
    plot(data(:,1), tlmn(:,4), 'k', 'LineWidth', 1);
    plot(data(:,1), sum(yawArr .* coeffsYaw,2), 'r', 'LineWidth', 1);
    ylabel("Yaw Torque");
    xlabel("Time[s]");
    legend("Measurment", "Model");
end

%% Find SBUS Thrust Map
throttle = (data(:,23)-1000) / 1000;
norm_thrust = data(:,2)*mass/(4*max_thrust);
norm_thrust_eval = linspace(0,1,1000);
throttle_eval = csaps(norm_thrust, throttle, 0.9, norm_thrust_eval);
% csvwrite('MPCtoBFL.csv',[norm_thrust_eval', throttle_eval']);

if PLOT
    figure;
    plot(norm_thrust, throttle, 'b.');
    hold on
    plot(norm_thrust_eval, throttle_eval, 'r', 'LineWidth', 1);
    legend('Measurement', 'Spline Fit');
    grid on
    xlabel('Normalized MPC Thrust')
    ylabel('Normalized Betaflight Throttle')
    title('Find SBUS Throttle Mapping');
end

%% Find Throttle to Motor PWM Map
motor_cmd = mean(data(:,28:31),2);
coeffs = polyfit(throttle, motor_cmd,1);
throttle_eval2 = linspace(0,1,100);
fprintf("Throttle to PWM: %5.3f*throttle+%5.3f\n", coeffs(1), coeffs(2));

if PLOT
    figure;
    hold on
    plot(throttle,motor_cmd, 'b.');
    plot(throttle_eval2, polyval(coeffs, throttle_eval2),'r');
    grid on
    xlabel('Normalized Betaflight Throttle')
    ylabel('Motor Command PWM')
    title('Find Throttle to Motor Mapping');
end

%%
% DATA CTRL STRUCT
% 1     time [s]                    20  rc command [0]
% 2     acceleration [m/s^2]        21  rc command [1]
% 3     omega_x body des            22  rc command [2]
% 4     omega_y body des            23  rc command [3]
% 5     omega_z body des            24  setpoint [0]
% 6     betaflight roll rate        25  setpoint [1]
% 7     betaflight pitch rate       26  setpoint [2]
% 8     betaflight yaw rate         27  setpoint [3]
% 9     axis P[0]                   28  mot command 1
% 10    axis P[1]                   29  mot command 2
% 11    axis P[2]                   30  mot command 3
% 12    axis I[0]                   31  mot command 4
% 13    axis I[1]
% 14    axis I[2]
% 15    axis D[0]
% 16    axis D[1]
% 17    axis F[0]
% 18    axis F[1]
% 19    axis F[2]

gain_matrix = zeros(3,3);
torques = [];
for ax = ["roll", "pitch", "yaw"]
fprintf("Direction %s\n", ax);

if ax == "roll"
    idxTorque = 2;
    idxP = 9;
    idxI = 12;
    idxD = 15;
    idxFF = 17;
    idxRC = 20;
    idxSet = 24;
    idxRef = 3;
    idxMeas = 6;
elseif ax == "pitch"
    idxTorque = 3;
    idxP = 10;
    idxI = 13;
    idxD = 16;
    idxFF = 18;
    idxRC = 21;
    idxSet = 25;
    idxRef = 4;
    idxMeas = 7;
elseif ax == "yaw"
    idxTorque = 4;
    idxP = 11;
    idxI = 14;
    idxFF = 19;
    idxRC = 22;
    idxSet = 26;
    idxRef = 5;
    idxMeas = 8;
end

myDiff = @(x) [x(2)-x(1); diff(x)];

%% Find MPC Input to Setpoint
scale = 180/pi;

if PLOT
    figure;
    hold on
    grid on
    plot(data(:,1), data(:,idxRef)*scale,'b')
    plot(data(:,1), data(:,idxSet),'r')
    legend('MPC Command', 'Beta Setpoint');
    ylim([-200, 200])
end


%% P Part
delta_roll = data(:,idxSet)/scale-data(:,idxMeas);
p_gain = tune_gain(data(:,idxP), delta_roll);

if PLOT
    figure;
    hold on
    grid on
    title("P Part");
    plot(data(:,1), data(:,idxP), 'g', 'LineWidth', 1);
    plot(data(:,1), delta_roll*p_gain, 'b', 'LineWidth', 1);
    legend('Measurement', 'Modeling');
end

err_p = data(:,idxP) - delta_roll*p_gain;
fprintf("P Part RMS:\t %f\n", rms(err_p));

%% I Part
tracking_err = data(:,idxSet)/scale - data(:,idxMeas);
i_value = cumsum(tracking_err);
i_correction = movmean(data(:,idxI) - i_value,5000);
i_gain = tune_gain(data(:,idxI), i_value + i_correction);
i_value =  i_value + i_correction;

if PLOT
    figure;
    hold on;
    grid on;
    title("I Part");
    plot(data(:,1), data(:,idxI), 'g', 'LineWidth', 1);
    plot(data(:,1), round(i_gain * i_value), 'b', 'LineWidth', 1);
    plot(data(:,1), i_gain * i_correction, 'r', 'LineWidth', 1);
    legend('Measurement', 'Modeling', 'Correction');
end
err_i = data(:,idxI) - i_value*i_gain;
fprintf("I Part RMS:\t %f\n", rms(err_i));

%% D Part
if ax ~= "yaw"
    gyro_lowpass_freq1 = 350;
    gyro_lowpass_freq2 = 250;
    d_lowpass_freq1 = 170;
    
    delta_roll_lp = pt1(data(:,idxMeas), gyro_lowpass_freq1);
    delta_roll_lp = pt1(delta_roll_lp, gyro_lowpass_freq2);
    d_value = myDiff(delta_roll_lp);
    d_value = pt1(d_value, d_lowpass_freq1);
    d_gain = tune_gain(data(:,idxD), d_value);
    
    
    if PLOT
        figure;
        hold on;
        grid on;
        title("D Part");
        plot(data(:,1), data(:,idxD), 'g', 'LineWidth', 1);
        plot(data(:,1), round(d_value * d_gain), 'b', 'LineWidth', 1);
        legend('Measurement', 'Modeling');
    end
    
    err_d = data(:,idxD) - d_value*d_gain;
    fprintf("D Part RMS:\t %f\n", rms(err_d));
else
    d_gain = 0;
end

fprintf("P Gain:\t% 5.3f\nI Gain:\t% 5.3f\nD Gain:\t% 5.3f\n", p_gain, i_gain, d_gain);
if ax=="roll"
    gain_matrix(1,1) = p_gain;
    gain_matrix(1,2) = i_gain;
    gain_matrix(1,3) = d_gain;
elseif ax=="pitch"
    gain_matrix(2,1) = p_gain;
    gain_matrix(2,2) = i_gain;
    gain_matrix(2,3) = d_gain;
elseif ax=="yaw"
    gain_matrix(3,1) = p_gain;
    gain_matrix(3,2) = i_gain;
end
if WRITE
    csvwrite(fname, gain_matrix);
end
%% Do same with MPC Setpoint as input
tracking_err = data(:,idxRef) - data(:,idxMeas); % realistic setting
% tracking_err = data(:,idxSet)/scale - data(:,idxMeas); % easy setting
p_part = p_gain * tracking_err;
i_part = i_gain * i_value; % need to use original value
d_part = d_gain * d_value; % independent of mpc commands
combined = (p_part + i_part + d_part) * 1e-3;
torques = [torques, combined];
end

%% Validation
throttle = interp1(norm_thrust_eval, throttle_eval, data(:,2)*mass/(4*max_thrust));
% figure;
% hold on
% grid on
% plot(throttle, 'k', 'LineWidth', 1);
% plot(torques(:,1), 'r', 'LineWidth', 1);
% plot(torques(:,2), 'g', 'LineWidth', 1);
% plot(torques(:,3), 'b', 'LineWidth', 1);
% ylim([-1, 1]);

force_torques = [throttle, torques];
mot_cmd = (B * force_torques')';

%%
figure;
for i = 1:4
    subplot(2,2,i)
    hold on
    grid on
    plot(data(:,1),data(:,27+i),'k')
    plot(data(:,1),coeffs(1)*mot_cmd(:,i)+coeffs(2), 'r');
    ylim([47 2048])
    legend('Truth', 'Model');
    title("Motor " + i)
    xlabel("Time [s]")
    ylabel("DSHOT Motor Signal")
end



%% Function Definitions

function y=apply_biquad(x, fc, fs)
    [b, a] = butter(2, 2*fc/fs);
    y = filter(b,a,x);
end

function y=apply_pt1(x, fc, fs)
    alpha = (2*pi*fc/fs)/(2*pi*fc/fs+1);
    y = alpha*x;
    y(1) = x(1);
    for i=2:length(x)
        y(i) = y(i) + (1-alpha)*y(i-1);
    end
end


function k=tune_gain(x1, x2)
    k = fminsearch(@(a) rms(x1-round(a*x2)), 100);
end

% end





