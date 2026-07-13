% THIS SCIRPT PREPROCESSES AND MERGES ROSBAGS WITH THE CORRESPONDING BETAFLIGHT
% LOG FILES. 

% Description of the Options, defaults set in create_options()
% opt.SMOOTHPLOTS       % Generates plots to illustrate the smoothing
% opt.SPLINE            % Use spline smoothing instead of Savitzky Golay
% opt.INTERPOLATE       % Resamples the OptiTrack data to equal times (only
                        % with spline smoothing.

% DATAPLOTS             % Output plots of the final merged data and save
                        % them to a pdf file
% ALIGN_ACC             % Use accelerometer data for time synchronization
                        % should be off
% ALIGN_RATE            % Use gyro for time alignment (should be on)
% DEBUGPLOTS            % For developers only, plots lots of stuff!
% COMBINEPLOTS          % Plot how betaflight and OptiTrack are merged
                        % (pretty much useless)
% WRITE                 % If set to false, no output files are written to
                        % the disk. Good for debugging.
% SEGMENTS              % If set to true, the script will segment the
                        % flights such that long hover periods are cut out
                        
% NOTE: To batch process files, just create an array of filenames called
% 'list' and uncomment lines 35-42, as well as 47 and 573.

clear all;
close all;
clc;

addpath ../Common/
addpath subroutines/


% for i = 1:length(list)
%    try
%        doit(list(i)); 
%    catch me
%        fprintf("Error when processing file %s", list(i));
%        me
%    end 
% end

%%

dataset = "Ex_OptiTrack";

% function doit(dataset)
close all

% Good Defaults!
opt = create_options();
DATAPLOTS = true;
ALIGN_ACC = false;
ALIGN_RATE = true;
DEBUGPLOTS = false;
COMBINEPLOTS = false;
WRITE = false;
SEGMENTS = true;

basepath = "../../ExampleData/OptiTrack/";
logfile = dataset + ".01.csv";
bagfile = dataset + ".bag";
rawoutfile = "raw_merged_" + dataset + ".csv";
outfile = "merged_" + dataset;
cmdfile = "command_" + dataset + ".csv";

data_log = read_log(basepath + logfile);
data_bag = read_bag(basepath + bagfile);
commands = read_bag(basepath + bagfile, "command");
if commands == 0
    HAS_CMD = false;
else
    HAS_CMD = true;
end

%% Pre-Process the data and find segments without errors
dt = 1/round(1/mean(diff(data_bag(:,3))));

data_bag = pre_process_bag(data_bag);
segs_valid = find_consecutive_segments(data_bag);
segs_times = reshape(data_bag(segs_valid(:),3), size(segs_valid));
segs_times(:,3) = segs_times(:,2) - segs_times(:,1);


%% Smooth the two datasets
%             1      2 3 4        5 6 7       8 9 10
% log_smooth: t acc_[r|p|y] rate_[r|p|y] acc_[x|y|z] mot_meas_[1|2|3|4] bat_v bat_a
% bag_smooth: t acc_[r|p|y] rate_[r|p|y] q_[x|y|z|w] acc_[x|y|z] vel_[x|y|z] pos_[x|y|z]
log_smooth = log_smoother(data_log, opt);
log_raw = data_log;
bag_smooth = bag_smoother(data_bag, opt);

%% Find hover segments and exclude this region if hovering is longer than 5s
% idea: model hovering as mean(||v||_2) = 0, std(||v||_2) = 0.4 m/s
p_hover = normpdf(vecnorm(bag_smooth(:,15:17),2,2), 0, 0.4) / normpdf(0,0,0.4);
p_hover_smooth = movmean(p_hover, 2/dt); % average over 2 seconds
mask_nohover = boolean(movmax(p_hover_smooth<0.5,3/dt));
segs_nohover = mask_to_segments(mask_nohover);

% Discard segments shorter than 1 second
segs_nohover = segs_nohover(segs_nohover(:,3)*dt > 1,:);


%% Combine no-hover and generally valid data segments
mask_valid = segments_to_mask(segment_times_to_segments(segs_times, bag_smooth(:,1)),length(mask_nohover));
segs = mask_to_segments(logical(mask_valid) & logical(mask_nohover));
segs_times = reshape(bag_smooth(segs(:),1), size(segs));
segs_times(:,3) = segs_times(:,2) - segs_times(:,1);

% Use slightly less strict hover criterion (approx <1 m/s) for offset
% detection in the forces x and y (used at the end)
mask_nearlyhover = boolean(p_hover_smooth>0.1);
segs_nearlyhover = mask_to_segments(mask_nearlyhover & logical(mask_valid));
times_nearlyhover = [bag_smooth(segs_nearlyhover(:,1),1) ...
                     bag_smooth(segs_nearlyhover(:,2),1)];
times_nearlyhover(:,3) = times_nearlyhover(:,2) - times_nearlyhover(:,1);
if sum(mask_nearlyhover) == 0
   times_nearlyhover = zeros(0,3); 
end

%% Pre-Align data using gyro or acc information
t1 = [min(bag_smooth(1,1),log_smooth(1,1)):dt:max(bag_smooth(end,1), log_smooth(end,1))];
if ~ALIGN_ACC
    idx_align = 6;
    s1 = interp1(log_smooth(:,1), log_smooth(:,idx_align), t1, 'linear', 0);
    s2 = interp1(bag_smooth(:,1), bag_smooth(:,idx_align), t1, 'linear', 0);
    delay = finddelay(s1,s2);
    t = delay * dt;               
else
    s1 = interp1(log_smooth(:,1), sum(log_smooth(:,8:10).^2,2), t1, 'linear', 0);
    s2 = interp1(bag_smooth(:,1), sum(bag_smooth(:,12:14).^2,2), t1, 'linear', 0);
    delay = finddelay(s1,s2);
    t = delay * dt;
end
log_smooth(:,1) = log_smooth(:,1) + t;
log_raw(:,2) = log_raw(:,2) + t;

if abs(t) > 10
   fprintf("WARNING: POSSIBLE ALIGNMENT ISSUE! t_offset = %f\n", t); 
end


%% Compute precision alignment using gyro or accelerometer correlation
x = [];
y = [];
w = [];
if ALIGN_RATE
    idx_align = 5;
    [~, ~, ~, x, y, w] = align_data(bag_smooth(:,1), bag_smooth(:,idx_align), ...
                       log_smooth(:,1), log_smooth(:,idx_align));
    idx_align = 6;
    [offset, slope, ~, x, y, w] = align_data(bag_smooth(:,1), bag_smooth(:,idx_align), ...
                       log_smooth(:,1), log_smooth(:,idx_align), [x, y, w], DEBUGPLOTS);
end
if ALIGN_ACC
    [offset, slope] = align_data(bag_smooth(:,1), sum(bag_smooth(:,12:14).^2,2), ...
                   log_smooth(:,1),sum(log_smooth(:,8:10).^2,2), [x, y, w], DEBUGPLOTS);              
end              
              

%% Correct the estimated clock drift and discard non-overlapping parts
correctClockDrift = @(t) t - (offset + slope*t);
log_smooth(:,1) = correctClockDrift(log_smooth(:,1));
log_raw(:,2) = correctClockDrift(log_raw(:,2));

t_start = max(log_smooth(1,1), bag_smooth(1,1));
t_end = min(log_smooth(end,1), bag_smooth(end,1));
log_smooth = log_smooth(log_smooth(:,1) > t_start & log_smooth(:,1) < t_end,:);
log_smooth(:,1) = log_smooth(:,1)-t_start;
log_raw = log_raw(log_raw(:,2) > t_start & log_raw(:,2) < t_end,:);
log_raw(:,2) = log_raw(:,2) - t_start;
idx_start = find(bag_smooth(:,1) > t_start, 1, 'first');
bag_smooth = bag_smooth(bag_smooth(:,1) > t_start & bag_smooth(:,1) < t_end,:);
bag_smooth(:,1) = bag_smooth(:,1)-t_start;
t_end = t_end - t_start;

if HAS_CMD
    commands(:,1)  = commands(:,1) - t_start;
end


%% Interpolate the whole dataset to have constant sampling rate
tref = [1:floor(t_end/dt)-1]'*dt;
data = resample_log(tref, log_smooth(:,1), log_smooth(:,2:end));

data = [data, interp1(bag_smooth(:,1), bag_smooth(:,2:end), tref)];
data = data(all(isfinite(data)'),:);


%% Convert accelerations and velocity to local reference frame
data(:,40:42) = rotateframe(quaternion(data(:,[30, 27:29])), data(:,31:33));
data(:,43:45) = rotateframe(quaternion(data(:,[30, 27:29])), data(:,34:36));

% Use the previously detected segments where the vehicle was nearly in
% hover to subtract static offsets of the accerelation data
if ~isempty(times_nearlyhover)
    mask_nearlyhover = logical(segments_to_mask(segment_times_to_segments(...
        times_nearlyhover-t_start, data(:,1)),size(data,1)));
    motor_max = movmax(max(data(:,11:14)')', 0.5/dt);
    mask_motorhover = (motor_max < 1350) & (motor_max > 800);
    mask_nearlyhover = mask_nearlyhover & mask_motorhover;
         
    log_hover_acc = trimmean(data(mask_nearlyhover, 8:9),0.2);
    bag_hover_acc = trimmean(data(mask_nearlyhover, 40:41),0.2);
    hover_motor = trimmean(data(mask_nearlyhover, 11:14).^2, 0.2);
    hover_motor_avg = mean(hover_motor);
    hover_motor_delta = hover_motor/hover_motor_avg;
    
    % Correct data (for thrust, the term is quadratic)
    data(:,8:9) = data(:,8:9) - log_hover_acc;
    data(:,40:41) = data(:,40:41) - bag_hover_acc;
    data(:,11:14) = sqrt( data(:,11:14).^2./hover_motor_delta );
end

%% Write to file
% RAW DATA STRUCT
% BETA FLIGHT DATA
%	1	time                    11	omega motor 1			
%	2	angular acceleration x	12	omega motor 2			
%	3	angular acceleration y	13	omega motor 3			
%	4	angular acceleration z	14	omega motor 4			
%	5	body rate x             15	pwm omega motor 1			
%	6	body rate y             16	pwm omega motor 2
%	7	body rate z				17  pwm omega motor 3
%	8	body acceleration x		18  pwm omega motor 4
%	9	body acceleration y		19  battery voltage
%	10	body acceleration z		20  battery current			
%                               
%                               						
%	OPTITRACK DATA							
%	21	angular acc body x      31	acceleration x	40	body acceleration x    
%	22	angular acc body y   	32  acceleration y	41	body acceleration y
%	23	angular acc body z   	33	acceleration z	42	body acceleration z
%	24	angular rate body x     34	velocity x		43	body velocity x
%	25	angular rate body y     35	velocity y		44	body velocity y
%	26	angular rate body z     36	velocity z		45	body velocity z
%	27	quaternion x            37	position x      
%	28	quaternion y            38	position y		
%	29	quaternion z            39	position z		
%   30  quaternion w                                                                              


if WRITE
    header = ["t",                              ...
              "beta_bra","beta_bpa","beta_bya", ...
              "beta_brr","beta_bpr","beta_byr", ...
              "beta_ax","beta_ay","beta_az",    ...
              "beta_mm1","beta_mm2","beta_mm3","beta_mm4", ...
              "beta_mm1_des", "beta_mm2_des", "beta_mm3_des", "beta_mm4_des",...
              "bat_v","bat_i", ...
              "Opti_ra","Opti_pa","Opti_ya",    ...
              "Opti_rr","Opti_pr","Opti_yr",    ...
              "Opti_qx","Opti_qy","Opti_qz","Opti_qw",       ...
              "Opti_ax","Opti_ay","Opti_az",    ...
              "Opti_vx","Opti_vy","Opti_vz",    ...
              "Opti_x","Opti_y","Opti_z",       ...
              "OptiB_ax","OptiB_ay","OptiB_az", ...
              "OptiB_vx","OptiB_vy","OptiB_vz"];
    write_csv(basepath + rawoutfile, data, header);
end

%% Combine information from different sensors
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

merged = data(:,1);
merged(:,2:7) = data(:,2:7);      % angular acceleration and rates betaflight
merged(:,8:11)  = data(:,27:30);  % quaternion components
merged(:,12:17) = data(:,40:45);  % accelerations velocities in body frame
merged(:,18:20) = data(:,37:39);  % position
merged(:,21:24) = data(:,11:14);  % motor data

for i = 0:3 % angular acceleration of the motors
   merged(:,25+i) = gradient(data(:,11+i), data(:,1));
end
merged(:,29) = data(:,19);

%% Write segmented data
if WRITE
        header = ["t", "ang acc x" "ang acc y", "ang acc z",    ...
            "ang vel x", "ang vel y", "ang vel z",        ...
            "quat x", "quat y", "quat z", "quat w",       ...
            "acc x", "acc y", "acc z",                    ...
            "vel x", "vel y", "vel z",                    ...
            "pos x", "pos y", "pos z",                    ...
            "mot 1", "mot 2", "mot 3", "mot 4",           ...
            "dmot 1", "dmot 2", "dmot 3", "dmot 4", "vbat"];
end

if SEGMENTS
    segs_merged = zeros(size(segs));
    for i = 1:size(segs,1)
        idx_start = find(segs_times(i,1)-t_start < data(:,1), 1, 'first');
        if isempty(idx_start)
            idx_start = 1;
        end
        idx_end =  find(segs_times(i,2)-t_start < data(:,1), 1, 'first');
        if isempty(idx_end)
            idx_end = size(merged,1);
        end
        duration = merged(idx_end,1) - merged(idx_start,1);
        segs_merged(i,1) = idx_start;
        segs_merged(i,2) = idx_end;
        segs_merged(i,3) = merged(idx_start,1);
        segs_merged(i,4) = merged(idx_end,1);
        segs_merged(i,5) = segs_merged(i,2) - segs_merged(i,1);
        if duration < 0
            fprintf("note: negative segment length encountered\n");
            continue;
        end
        
        if WRITE
            write_csv(basepath + outfile + sprintf("_seg_%d", i),...
                merged(idx_start:idx_end,:), header);
        end
    end
end
if WRITE
    write_csv(basepath + outfile, merged, header);
end

%% Take care of command data for betaflight sysid
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

if HAS_CMD
    commands_raw = commands;
    commands = [zoh(commands(:,1), commands(:,2:end), log_raw(:,2))];
    first_nans = find(~any(isnan(commands'))',1,'first');
    last_nans = find(~any(isnan(commands'))',1,'last');
    commands(1:first_nans-1,:) = repmat(commands(first_nans,:),[first_nans-1,1]);
    commands(last_nans+1:end,:) = repmat(commands(last_nans,:),[size(commands,1)-last_nans,1]);
    commands = [log_raw(:,2) commands log_raw(:,25:27) log_raw(:,3:21) log_raw(:,35:38)];
    
    if WRITE
        command_header = ["t", "cmd_acc_z", ...
            "cmd_omega_x", "cmd_omega_y", "cmd_omega_z", ...
            "omega_x", "omega_y", "omega_z", ...
            "P[0]", "P[1]", "P[2]", "I[0]", "I[1]", "I[2]",  ...
            "D[0]", "D[1]",         "F[0]", "F[1]", "F[2]", ...
            "RC[0]", "RC[1]", "RC[2]", "RC[3]", ...
            "set[0]", "set[1]", "set[2]", "set[3]", ...
            "mot_1", "mot_2", "mot_3", "mot_4"];
        write_csv(basepath + cmdfile, commands, command_header);
        command_raw_header = ["t", "cmd_acc_z", ...
            "cmd_omega_x", "cmd_omega_y", "cmd_omega_z",];
        write_csv(basepath + "raw_" + cmdfile, commands_raw, command_raw_header);
    end
end


%% Plot everything
if ~SEGMENTS
    segs_merged = [];
end
if DATAPLOTS
    figure;
    titles= ["Roll Acc", "Pitch Acc", "Yaw Acc"];
    for i = 1:3
        ax(i) = subplot(3,1,i);
        hold on
        grid on
        title(titles(i));
        plot(data(:,1), data(:,1+i), 'b');
        plot(data(:,1), data(:,20+i), 'r');
        plot(merged(:,1), merged(:,1+i), 'k', 'LineWidth', 0.7);
        xlabel("Time [s]")
        ylabel("Acceleration [rad/s^2]")
        legend("BetaFlight", "OptiTrack", "Merged")
        for j = 1:size(segs_merged,1)
            xline(segs_merged(j,3),'--',sprintf("seg. %d", j),'Fontsize',8,'HandleVisibility','off');
            xline(segs_merged(j,4),'--','HandleVisibility','off');
        end
    end
    linkaxes(ax, 'x');
    if WRITE
        set( gcf,'PaperSize',[16 9], 'PaperPosition',[0 0 16 9])
        print(gcf, "Page1.pdf", '-painters', '-dpdf');
    end
    
    figure;
    titles= ["Rollrate", "Pitchrate", "Yawrate"];
    for i = 1:3
        ax(i) = subplot(3,1,i);
        hold on
        grid on
        title(titles(i));
        plot(data(:,1), data(:,4+i), 'b');
        plot(data(:,1), data(:,23+i), 'r');
        plot(merged(:,1), merged(:,4+i), 'k', 'LineWidth', 0.7);
        xlabel("Time [s]")
        ylabel("Rate [rad/s]")
        legend("BetaFlight", "OptiTrack", "Merged")
        for j = 1:size(segs_merged,1)
            xline(segs_merged(j,3),'--',sprintf("seg. %d", j),'Fontsize',8,'HandleVisibility','off');
            xline(segs_merged(j,4),'--','HandleVisibility','off');
        end
    end
    linkaxes(ax, 'x');
    if WRITE
        set( gcf,'PaperSize',[16 9], 'PaperPosition',[0 0 16 9])
        print(gcf, "Page2.pdf", '-painters', '-dpdf');
    end
    
    figure;
    titles= ["Roll", "Pitch", "Yaw"];
    tmp = quaternion(data(:,[30 27:29]));
    tmp = quat2eul(tmp);
    for i = 1:3
        ax(i) = subplot(3,1,i);
        hold on
        grid on
        title(titles(i));
        plot(data(:,1), tmp(:,4-i), 'r');
        xlabel("Time [s]")
        ylabel("Rate [rad/s]")
        legend("OptiTrack")
        for j = 1:size(segs_merged,1)
            xline(segs_merged(j,3),'--',sprintf("seg. %d", j),'Fontsize',8,'HandleVisibility','off');
            xline(segs_merged(j,4),'--','HandleVisibility','off');
        end
    end
    linkaxes(ax, 'x');
    if WRITE
        set( gcf,'PaperSize',[16 9], 'PaperPosition',[0 0 16 9])
        print(gcf, "Page3.pdf", '-painters', '-dpdf');
    end
    
    figure;
    titles= ["Acceleration X", "Acceleration Y", "Acceleration Z"];
    for i = 1:3
        ax(i) = subplot(3,1,i);
        hold on
        grid on
        title(titles(i));
        plot(data(:,1), data(:,7+i), 'b');
        plot(data(:,1), data(:,39+i), 'r');
        plot(merged(:,1), merged(:,11+i), 'k', 'LineWidth', 0.7);
        xlabel("Time [s]")
        ylabel("Acceleration [m/s^2]")
        legend("BetaFlight", "OptiTrack", "Merged")
        for j = 1:size(segs_merged,1)
            xline(segs_merged(j,3),'--',sprintf("seg. %d", j),'Fontsize',8,'HandleVisibility','off');
            xline(segs_merged(j,4),'--','HandleVisibility','off');
        end
    end
    linkaxes(ax, 'x');
    if WRITE
        set( gcf,'PaperSize',[16 9], 'PaperPosition',[0 0 16 9])
        print(gcf, "Page4.pdf", '-painters', '-dpdf');
    end
    
    figure;
    titles= ["Velocity X", "Velocity Y", "Velocity Z"];
    for i = 1:3
        ax(i) = subplot(3,1,i);
        hold on
        grid on
        title(titles(i));
        plot(merged(:,1), merged(:,14+i), 'r', 'LineWidth', 0.7);
        xlabel("Time [s]")
        ylabel("Velocity [m/s]")
        legend("OptiTrack")
        for j = 1:size(segs_merged,1)
            xline(segs_merged(j,3),'--',sprintf("seg. %d", j),'Fontsize',8,'HandleVisibility','off');
            xline(segs_merged(j,4),'--','HandleVisibility','off');
        end
    end
    linkaxes(ax, 'x');
    if WRITE
        set( gcf,'PaperSize',[16 9], 'PaperPosition',[0 0 16 9])
        print(gcf, "Page5.pdf", '-painters', '-dpdf');
    end

    figure;
    titles= ["Position X", "Position Y", "Position Z"];
    for i = 1:3
        ax(i) = subplot(3,1,i);
        hold on
        grid on
        title(titles(i));
        plot(merged(:,1), merged(:,17+i), 'r', 'LineWidth', 0.7);
        xlabel("Time [s]")
        ylabel("Position [m]")
        legend("OptiTrack")
        for j = 1:size(segs_merged,1)
            xline(segs_merged(j,3),'--',sprintf("seg. %d", j),'Fontsize',8,'HandleVisibility','off');
            xline(segs_merged(j,4),'--','HandleVisibility','off');
        end
    end
    linkaxes(ax, 'x');
    if WRITE
        set( gcf,'PaperSize',[16 9], 'PaperPosition',[0 0 16 9])
        print(gcf, "Page6.pdf", '-painters', '-dpdf');
    end
    
    figure;
    grid on
    hold on
    if SEGMENTS
        for i = 1:size(segs_merged,1)
            plot3(merged(segs_merged(i,1):segs_merged(i,2),18), ...
                  merged(segs_merged(i,1):segs_merged(i,2),19), ...
                  merged(segs_merged(i,1):segs_merged(i,2),20), ...
                  'LineWidth', 1);
        end
    else
        plot3(merged(:,18), ...
              merged(:,19), ...
              merged(:,20), ...
              'LineWidth', 1);
    end
    xlabel('x [m]');
    ylabel('y [m]');
    zlabel('z [m]');
    if WRITE
        set( gcf,'PaperSize',[16 9], 'PaperPosition',[0 0 16 9])
        print(gcf, "Page7.pdf", '-painters', '-dpdf');
    end

    figure;
    hold on;
    title("Motor Plots");
    plot(data(:,1), data(:,11), 'r');
    plot(data(:,1), data(:,12), 'g');
    plot(data(:,1), data(:,13), 'b');
    plot(data(:,1), data(:,14), 'k');
    legend("Motor 1", "Motor 2", "Motor 3", "Motor 4");
    grid on
    xlabel("Time [s]")
    ylabel("Motorspeed [rad/s]");
    if WRITE
        set( gcf,'PaperSize',[16 9], 'PaperPosition',[0 0 16 9])
        print(gcf, "Page8.pdf", '-painters', '-dpdf');
    end
    
    if WRITE
        system("pdftk Page1.pdf Page2.pdf Page3.pdf Page4.pdf Page5.pdf Page6.pdf Page7.pdf Page8.pdf cat output " + basepath + dataset + ".pdf");
        system("rm Page1.pdf Page2.pdf Page3.pdf Page4.pdf Page5.pdf Page6.pdf Page7.pdf Page8.pdf");
    end
end


%% End of the function
% end







