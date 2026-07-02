clear all
close all
clc

addpath ../OptiTrack/subroutines

basepath = "../../ExampleData/OptiTrack/";
dataset = [
            "Ex_OptiTrack";
          ];

% DATA STRUCTURE
%   1   time
%	2	ang. acceleration body_x    12	acceleration body_x     21  motor 1
%	3	ang. acceleration body_y    13  acceleration body_y     22  motor 2
%	4	ang. acceleration body_z    14	acceleration body_z     23  motor 3
%	5	angular rate body_x         15	velocity body_x         24  motor 4
%	6	angular rate body_y         16	velocity body_y         25  dmotor 1
%	7	angular rate body_z         17	velocity body_z         26  dmotor 2
%	8	quaternion qx               18	position x              27  dmotor 3
%	9	quaternion qy               19	position y              28  dmotor 4
%	10	quaternion qz               20	position z              29  battery v
%   11  quaternion qw               


%%
motor_rpm = [];
motor_cum = [];
motor_cmd = [];
mpc_cmd = [];
mpc_pwm = [];
orig_voltage = [];
voltage = [];

for i = 1:length(dataset)
    data = csvread(basepath + "merged_" + dataset(i) + ".csv", 1);
    cmds = csvread(basepath + "command_" + dataset(i) + ".csv", 1);
    rpm = mean(data(:,21:24),2)/1000;
    cum = cumsum(rpm.^3)/100/400;
    cmd = mean(cmds(:,28:31),2)/1000;
    v = data(:,29)/10;
    idx = find(rpm > 1, 1, 'first');
    V1 = v(idx+100);
    if V1 > 15.95
        motor_rpm = [motor_rpm; rpm];
        motor_cum = [motor_cum; cum];
        motor_cmd = [motor_cmd; interp1(cmds(:,1), cmd, data(:,1))];
        mpc_cmd = [mpc_cmd; interp1(cmds(:,1), cmds(:,2), data(:,1))];
        mpc_pwm = [mpc_pwm; interp1(cmds(:,1), cmds(:,27), data(:,1))];
        voltage = [voltage; v-V1];
        orig_voltage = [orig_voltage; v];
    end
    if max(rpm) > 3
       dataset(i) 
    end
end

%% Identify Battery Voltage Model
lm = fitlm([motor_rpm, motor_cum], voltage, ' Volt ~ 1 + RPM + Cum + RPM^2', 'VarNames', ["RPM", "Cum", "Volt"],'RobustOpts', 'on')

eval_rpm = [min(motor_rpm):0.01:max(motor_rpm)]';
eval_cum = [min(motor_cum):0.1:max(motor_cum)]';
[eval_x, eval_y] = meshgrid(eval_rpm,eval_cum);
eval_x = reshape(eval_x, [], 1);
eval_y = reshape(eval_y, [], 1);
eval_z = reshape(predict(lm, [eval_x, eval_y]), length(eval_cum), length(eval_rpm));

figure;
plot3(motor_rpm, motor_cum, voltage, 'b.')
hold on
surf(eval_rpm, eval_cum, eval_z)
shading interp
alpha 0.5
grid on
xlabel("Motor RPM x 1000")
ylabel("Cumulative Motor RPM x 1000")
zlabel("Voltage [V]")

%% Identify dependency of motor RPM on battery voltage and command
lm = fitlm([sqrt(motor_cmd), motor_cmd, orig_voltage], motor_rpm, 'linear', 'VarNames', ["sqrt cmd", "cmd", "volt", "rpm"],'RobustOpts', 'on')

eval_cmd = [48:2047]/1000';
eval_volt = [13.5:0.1:16.5]';
[eval_x, eval_y] = meshgrid(eval_cmd,eval_volt);
eval_x = reshape(eval_x, [], 1);
eval_y = reshape(eval_y, [], 1);
eval_z = reshape(predict(lm, [sqrt(eval_x), eval_x, eval_y]), length(eval_volt), length(eval_cmd));

figure;
plot3(motor_cmd, orig_voltage, motor_rpm, 'b.')
hold on
surf(eval_cmd, eval_volt, eval_z)
shading interp
alpha 0.5
grid on
xlabel("Motor CMD x 1000")
ylabel("Voltage")
zlabel("Motor RPM [rad/s]")

%% Identify SBUS Logic
% thrust map in MATLAB
map = csvread('thrust_map.csv');
map_t = linspace(map(1,2),map(1,3),map(1,1));
map_v = linspace(map(1,4),map(1,5),map(1,1));
map = map(2:end,:);
sbus = interp2(map_t, map_v, map, mpc_cmd * 0.772, orig_voltage);

idx_bad = logical(isnan(sbus));
sbus = sbus(~idx_bad);
mpc_pwm_clean = mpc_pwm(~idx_bad);

p = polyfit(sbus, mpc_pwm_clean, 1)

figure;
plot(sbus, mpc_pwm_clean, 'b.')
hold on
grid on
plot(300:1792, polyval(p, 300:1792), 'r')
xlabel("SBUS Command")
ylabel("Motor Command")
legend("Measurement", "Fit")





%%
lm = fitlm([motor_rpm, motor_cum], voltage, ' Volt ~ 1 + RPM + Cum + RPM^2', 'VarNames', ["RPM", "Cum", "Volt"],'RobustOpts', 'on');
dataset = "Ex_OptiTrack";        % 3d circle, delta_z = 4, vel=2.9
data = csvread(basepath + "merged_" + dataset + ".csv", 1);
rpm = mean(data(:,21:24),2)/1000;
cum = cumsum(rpm.^3)/40000;
v = data(:,29)/10;
idx = find(rpm > 1, 1, 'first');
V1 = v(idx+100);

figure;
hold on
grid on
plot(data(:,1), v, 'b', 'LineWidth', 1)
plot(data(:,1), predict(lm, [rpm, cum]) + V1, 'r', 'LineWidth', 1)
legend("Measurement", "Modeling")
ylabel("Voltage [V]")
xlabel("Time [s]")

