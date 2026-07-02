% CONING ANALYSIS, HACKY SCRIPT

clear all;
close all;
clc;

addpath ../Common/

basepath = "../../ExampleData/BEM/";
videoname = "Coning_Video.MOV";
audioname = "Coning_Audio.wav";
outfile = "Ex_Coning.csv";

% Attention you might need to adjust the region of interest depending on
% where the tip of the propeller is within the frame! It is found in lines
% 34, 35.
% Also make sure to check the results for plausiblity. The fourier analysis
% of the audio is not the most exact measure and sometimes detecs a
% lower/higher frequency (overtone) or multiple of the prop blades (e.g. a
% factor of 3 wrong). In this case, fix the generated csv by hand or use
% something like in lines 112 to 114 (fixes the errors in the example
% video case)

%% Analyze Video
t_start = 15; %s
t_end   = 59; %s


v = VideoReader(basepath + videoname);
v.CurrentTime = t_start;
v_result = [];
while (hasFrame(v) && v.CurrentTime < t_end)
   frame = readFrame(v);
   roi_1_x = 1670:1720;
   roi_1_y = 470:540;
   frame = frame(roi_1_y, roi_1_x, :);
   rel_b = double(frame(:,:,3)).^2./(double(frame(:,:,1)) + double(frame(:,:,2))).^2;
   curve = sum(rel_b,2);
   [val_max, idx]= max(curve);
   val_min = min(curve);
   val_max = (val_max-val_min)/val_min;
   v_result(end+1,1:3) = [v.CurrentTime, val_max, idx];
end

%% Plot Video Analysis Results
figure;
plot(v_result(:,1), movmean(v_result(:,2),5),'bx');
title('Values')
ylim([0, 50])

figure; 
plot(v_result(:,1), movmean(v_result(:,3),5),'bx')
title('pos')
ylim([0, roi_1_y(end)-roi_1_y(1)])
grid on
title("Tip position");
xlabel("Time Index [s]");
ylabel("Pixel Position");


%% Use Spektrogram to get RPM of the prop

[y, Fs] = audioread(basepath + audioname);
y = mean(y(t_start*Fs:t_end*Fs,:),2);
figure; spectrogram(y, blackman(16384), 8192, [], Fs, 'yaxis');
[s, f, t] = spectrogram(y, blackman(16384), 8192, [], Fs);

%% 
fupper = @(t) 150 + 1400*t/45;
flower = @(t) max(0, 1200*(t-10)/45);
val = zeros(length(t),1);
idx = zeros(length(t),1);
for i = 1:length(t)
    time = t(i);
    imin = 0;
    imax = fupper(time);
    freq_range = find((f < imax) & (f > imin));
    mag  = abs(s(freq_range,i)).^2;
    [val(i), idx(i)] = max(mag);    
    idx(i) = f(freq_range(idx(i)));
end

t_vid = t + t_start;
omega = idx * 2 * pi/3;

%%
omega_s = movstd(omega, 5);
threshold = 10;
seg_idx = find(omega_s < threshold);
segs = find_segments(seg_idx);
segs = segs(segs(:,3) >= 4,:);

segs_times(:,1) = t_vid(segs(:,1));
segs_times(:,2) = t_vid(segs(:,2));
seg_means = zeros(size(segs,1), 1);
for i = 1:size(segs,1)
   seg_means(i,:) = mean(omega(segs(i,1):segs(i,2),:),1);
end

figure;
plot(t_vid, omega);
hold on
for i = 1:size(segs,1)
    patch([segs_times(i,1), segs_times(i,2), segs_times(i,2) segs_times(i,1)], ...
          [min(ylim) min(ylim) max(ylim) max(ylim)], [0.8 0.8 0.8])
end
plot(t_vid, omega, 'b');
xlabel("Time [s]");
ylabel("Omega [rad/s]");

%% Fix data if wrong frequency was detected
% only for 8044
wrong_freq_idx = find(seg_means < circshift(seg_means,1));
wrong_freq_idx = wrong_freq_idx(2:end);
seg_means(wrong_freq_idx) = seg_means(wrong_freq_idx) * 3;

%% Assemble dataset
prop_length = (1705-210)/2;
idx = find(v_result(:,1) < segs_times(1,2));
prop_zero = mean(v_result(idx,3));
result = zeros(length(segs_times), 2);
for i = 1:length(segs_times)
    result(i,1) = seg_means(i);
    idx = find( (v_result(:,1) > segs_times(i,1)) & (v_result(:,1) < segs_times(i,2)) );
    result(i,2) = atan((prop_zero-mean(v_result(idx,3)))/prop_length);
end
figure;
plot(result(:,1), result(:,2)*180/pi, 'bx');
xlabel("Omega [rad/s]");
ylabel("Coning Angle [deg]");

%% Write files
header = ["omega", "a0"];
write_csv(basepath + outfile, result, header);






