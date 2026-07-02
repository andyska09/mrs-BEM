clear all;
close all;
clc;

addpath ../OptiTrack/subroutines
addpath ../Common/

% The naming of the files should be as follows: all except the RotorS bags
% are located in the same folder and encoded by the name of the Measurement
% Rosbag, e.g. Ex_OptiTrack
% Then, the corresponding simulation files need to be named
%               Ex_OptiTrack_sim_XXX.csv
% where XXX is the name of the model as written in SIMTYPE.
% If a RotorS bag is p;resent, it needs to be names
%               RotorS_Ex_OptiTrack.bag
% and the RotorS variable must be true. A different basepath for RotorS can
% be set.
% An optional z-offset of the trajectory can be defined as well.

SIMTYPE = {"fit", "bem+nn"}';
dataset = "Ex_OptiTrack";

USE2D = false;
RotorS = true;
basepath = "../../ExampleData/OptiTrack/";
rotorspath = "../../ExampleData/OptiTrack/";

z_offset = 0;




%% Read in Data
measurement_file = "merged_" + dataset + ".csv";
simulation_files = num2cell(dataset + "_sim_" + SIMTYPE + ".csv");
trajectory_file = dataset + "_traj.csv";
merged_data = csvread(basepath + measurement_file, 1);


traj_data = read_traj(basepath + trajectory_file);

idx_change = [1 18 19 20 11 8 9 10 15 16 17 5 6 7 12 13 14  2 3 4 29];
idx_align = [2:3];
meas_data = csvread(basepath + measurement_file, 1);
meas_data = meas_data(:, idx_change);
meas_data(:,4) = meas_data(:,4) - z_offset;
meas_data = align_to_reference(meas_data, traj_data, idx_align, idx_align, true);

if RotorS
   rotors_data = read_rotors_bag(rotorspath + "RotorS_" + dataset + ".bag");
   rotors_data = [rotors_data(:,[3 5 6 7 8 9 10 11]), zeros(length(rotors_data), 13)];
end

sim_data = {};
for i = 1:length(simulation_files)
    tmp = read_sim(basepath + simulation_files{i});
    tmp = align_to_reference(tmp, traj_data, idx_align, idx_align, true);
    sim_data{i} = tmp;
end
sim_data = sim_data';

if RotorS
   sim_data{end+1} = align_to_reference(rotors_data, traj_data, idx_align, idx_align, true);
   SIMTYPE{end+1} = "RotorS";
end

%% Evaluate Errors
idx = [2:4];
d_meas = cellfun(@(x) vecnorm(x(:,idx) - meas_data(:,idx),2,2), ...
                 sim_data, 'UniformOutput', false);
d_traj = cellfun(@(x) vecnorm(x(:,idx) - traj_data(:,idx),2,2), ...
                 sim_data, 'UniformOutput', false);
             
for i = 1:length(d_meas)
    fprintf("Model %s has measurement error %5.3f\n", SIMTYPE{i}, sqrt(mean(d_meas{i}.^2)));
end


fprintf("\nMeasurement has tracking error %5.3f\n", rms(vecnorm(meas_data(:,idx) - traj_data(:,idx),2,2)));
for i = 1:length(d_meas)
    fprintf("Model %s has tracking error %5.3f\n", SIMTYPE{i}, sqrt(mean(d_traj{i}.^2)));
end

%%
colors = ['b', 'r', 'k', 'm', 'g', 'c', 'y'];
figure;
plot3(meas_data(:,2), meas_data(:,3), meas_data(:,4), colors(1), 'LineWidth', 1);
hold on
for j = 1:length(sim_data)
    plot3(sim_data{j}(:,2), sim_data{j}(:,3), sim_data{j}(:,4), colors(j+1), 'LineWidth', 1);
end
xlabel("X [m]")
ylabel("Y [m]")
zlabel("Z [m]")
title("Trajectory");
daspect([1 1 1])
legend(["Measurement", num2cell(SIMTYPE)']);
grid on

%% Position Plot
figure; 
names = ["X", "Y", "Z"];
colors = ['b', 'r', 'k', 'm', 'g', 'c', 'y'];
for i = 1:3
    ax(i) = subplot(2,3,i);
    hold on;
    grid on;
    idx = i+1;
%     plot(traj_data(:,1), traj_data(:,idx), 'b--', 'LineWidth', 1.5);
    plot(meas_data(:,1), meas_data(:,idx), colors(1), 'LineWidth', 1.5);
    for j = 1:length(sim_data)
        plot(sim_data{j}(:,1), sim_data{j}(:,idx), colors(j+1), 'LineWidth', 1.5);
    end
    title("Position " + names(i));
%     legend(["Measurement", num2cell(SIMTYPE)']);
end

ax(4) = subplot(2,3,[4:6]);
hold on;
grid on;
title("Position Error");
idx = [2:4];
for i = 1:length(sim_data)
    plot(meas_data(:,1), d_meas{i}, colors(i+1), 'LineWidth', 1.5);
end
xlabel("Time [s]");
ylabel("Error [m]");
legend(["" + num2cell(SIMTYPE)']);
linkaxes(ax, 'x');

%% Battery
colors = ['b', 'r', 'k', 'm', 'g', 'c', 'y'];
figure; 
grid on; 
hold on
plot(meas_data(:,1), meas_data(:,end)/10, 'b', 'LineWidth', 1.5); 
for j = 1:length(sim_data) - RotorS
    plot(sim_data{j}(:,1), sim_data{j}(:,41), colors(j+1), 'LineWidth', 1.5);
end
xlabel("Time [s]");
ylabel("Voltage [V]");
legend(["Measurement", num2cell(SIMTYPE)']);

