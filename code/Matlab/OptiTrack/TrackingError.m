close all
clear all
% clc


addpath ../Common/
addpath subroutines/

dataset = ["Ex_OptiTrack"]';  % note, must be 1xN array

traj_file = "Ex_OptiTrack_traj";
basepath = "../../ExampleData/OptiTrack/";

z_offset = 0;  % Optional z offset used when the trajectory was flown
PLOTS = true;

opt = create_options();


%% Read in all datasets
% The index change results in the order used inside agilicious and is
% compatible with the trajectory data.
idx_change = [1 18 19 20 11 8 9 10 15 16 17 5 6 7 12 13 14  2 3 4];
idx_align = [2:3]; % indices used for alignment of the data

traj_data = read_traj(basepath + traj_file + ".csv");
traj_data(:,4) = traj_data(:,4) + z_offset;

meas_data = {};
for d = dataset
    bagfile = d + ".bag";
    data_bag = read_bag(basepath + bagfile);
    data_bag = pre_process_bag(data_bag);
    bag_smooth = bag_smoother(data_bag, opt);
    bag_smooth = bag_smooth(:, idx_change);
    meas_data{end+1} =  align_to_reference(bag_smooth, traj_data, ...
                                               idx_align, idx_align, true);
end

% indices used to calculate the tracking error (2:4 = position)
idx = [2:4]; 

d_traj = cellfun(@(x) vecnorm(x(:,idx) - traj_data(:,idx),2,2), ...
                 meas_data, 'UniformOutput', false);
             
%%
if PLOTS
    figure;
    hold on
end
for i = 1:length(d_traj)
    fprintf("Flight %s has tracking error %5.3f\n", dataset(i), sqrt(mean(d_traj{i}.^2)));
    if PLOTS
       plot(meas_data{i}(:,1), d_traj{i}, 'LineWidth', 1)
    end
end

if PLOTS
    xlabel("Time [s]")
    ylabel("Position Error [m]")
    legend(dataset)
end





