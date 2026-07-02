% THRUST DRAG EXPERIMENTS
% STEP 2

clear all;
close all;
clc;

addpath ../Common/

basepath = "../../ExampleData/BEM/";
meas_files = ["Ex_Measurement.csv";] ;    % input all measurement files 
                                          % from Step 1 here (can be many)
proc_file = "Ex_AllMeasurements.csv";     % File where combined measurement
                                          % are written to


%% Read in the files
data = [];
for i=1:length(meas_files)
    data = [data; csvread(basepath + meas_files(i),1)];
end

[~, ind] = sort(data(:,1));
data = data(ind, :);
omega = data(:,1);
T = data(:,5);
thr = data(:,2);
Q = data(:,8);

%% Write files
header = ["omega", "thrust", "fx", "fy", "fz", "mx", "my", "mz"];
write_csv(basepath + proc_file, data, header);

figure;
plot(thr, omega, 'bx');
grid on;
xlabel("Commanded Throttle Value");
ylabel("Propeller Speed [rad/s]");
title("Command - Speed")


% Custom cost functions could be defined below
T_func = @(ct, w) ct*w.^2;
Q_func = @(cq, w) cq * w.^2;
T_err = @(ct) sum((T - T_func(ct, omega)).^2); 
Q_err = @(cq) 1000*sum((Q - Q_func(cq, omega)).^2); % *1000 improves fminsearch performance

ct = fminsearch(T_err, 0);
cq = fminsearch(Q_err, 0);

figure;
subplot(1,2,1);
hold on
plot(omega, T, 'bx');
plot(0:max(omega), T_func(ct, 0:max(omega)),'r-');
xlabel('Omega [rad/s]');
ylabel('Thrust [N]');
grid on
title('Pure Quadratic Thrust Fit');
legend('Exp. Data', "\Omega^2 Fit: Q = " + num2str(ct,'%8.3e') + " \Omega^2", 'Location', 'northwest');

subplot(1,2,2);
hold on
plot(omega, Q, 'bx');
plot(0:max(omega), Q_func(cq, 0:max(omega)),'r-');
xlabel('Omega [rad/s]');
ylabel('Torque [Nm]');
grid on
title('Pure Quadratic Torque Fit');
legend('Exp. Data', "\Omega^2 Fit: Q = " + num2str(cq,'%8.3e') + " \Omega^2" , 'Location', 'northwest');

