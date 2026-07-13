% THRUST DRAG EXPERIMENTS
% STEP 1

clear all;
close all;
clc;

addpath ../Common/

basepath = "../../ExampleData/BEM/";
meas_file = "Ex_ThrustTestStand.csv";     % Measurement file
proc_file = "Ex_Measurement.csv";         % Output file with processed data
data = csvread(basepath + meas_file,1);

CORRECT_DRIFT = true;                     % Linear drift correction
HAS_ARMING_COMMAND = true;                % Contains command sequence at 
                                          % start with no props spinning
ts = mean(diff(data(:,1)));               % Sampling time
fs = 1/ts;

%% Drift Analysis force Force Z direction

if CORRECT_DRIFT
    % Find indices where command was zero
    zero_idx = find(data(:,3) == 0);
    segs_off = find_segments(zero_idx);
    
    % Throw away first and last 10% of measurement duration
    segs_off(:,1) = segs_off(:,1) + round(segs_off(:,3)/10);
    segs_off(:,2) = segs_off(:,2) - round(segs_off(:,3)/10);
    
    seg_means_off = zeros(size(segs_off,1), size(data,2));
    for i = 1:size(segs_off,1)
        seg_means_off(i,:) = mean(data(segs_off(i,1):segs_off(i,2),:),1);
    end
    p = polyfit(seg_means_off(:,1), seg_means_off(:,6),1);
    slope = p(1);
else 
    slope = 0;
end


%% Find all plateaus and get mean value
% this movstd is a good measure of where the command was constant
thr_s = movmax(movstd(data(:,3), fs/3),fs/3);
threshold = 5;
seg_idx = find(thr_s < threshold);
segs = find_segments(seg_idx);

segs_times(:,1) = data(segs(:,1));
segs_times(:,2) = data(segs(:,2));
seg_means = zeros(size(segs,1), size(data,2));
for i = 1:size(segs,1)
   seg_means(i,:) = mean(data(segs(i,1):segs(i,2),:),1);
end

%% Calibrate by subtracting mean value and optionally the slope
off_seq = find(seg_means(:,3) == 0, 1, 'first');
zero_vals = seg_means(off_seq,:);
t0 = segs_times(off_seq,2);

data(:,4:9) = data(:,4:9) - zero_vals(4:9);
data(:,6)   = data(:,6) - slope*(data(:,1)-t0); % correct slope in force z

if HAS_ARMING_COMMAND % discard the first sequence as arming command
    segs = segs(off_seq+1:end,:);
    seg_means = seg_means(off_seq+1:end,:);
    segs_times = segs_times(off_seq+1:end,:);
end

%% Delete 'zero' segments where no command was given (i.e. cooldown)
nonzero_segs = find(seg_means(:,3) ~= 0);
segs = segs(nonzero_segs, :);
segs_times = segs_times(nonzero_segs, :);
seg_means = seg_means(nonzero_segs,:);

figure;
plot(data(:,1), data(:,6));
hold on
for i = 1:size(segs,1)
    patch([segs_times(i,1), segs_times(i,2), segs_times(i,2) segs_times(i,1)], ...
          [min(ylim) min(ylim) max(ylim) max(ylim)], [0.8 0.8 0.8])
end
plot(data(:,1), data(:,6));
xlabel("Time [s]");
ylabel("Thrust [N]");

%% Write files
header = ["omega", "thrust", "fx", "fy", "fz", "mx", "my", "mz"];
write_csv(basepath + proc_file, [seg_means(:,2:end)], header);

