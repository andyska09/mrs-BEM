function denoised = log_smoother(data, opt)
%log_smoother: the function takes data from beta logs and denoises them
% y = log_smoother(x, opt) takes a data array and returns a denoised
% version of the same size. The opt struct can be used to produce plots of
% all quantities. Only savitzky-golay smoothing is available.
% Note that the column order must be
%   1	 loopIteration
%   2	 time (s)
%   3    axis P[0]
%   4    axis P[1]
%   5    axis P[2]
%   6    axis I[0]
%   7    axis I[1]
%   8    axis I[2]
%   9    axis D[0]
%   10   axis D[1]
%   11   axis F[0]
%   12   axis F[1]
%   13   axis F[2]
%   14   rc Command [0]
%   15   rc Command [1]
%   16   rc Command [2]
%   17   rc Command [3]
%   18   setpoint [0]
%   19   setpoint [1]
%   20   setpoint [2]
%   21   setpoint [3]
%   22	 vbatLatest (V)
%   23	 amperageLatest (A)
%   25	 gyroADC[0] (rad/s)
%   26	 gyroADC[1] (rad/s)
%   27	 gyroADC[2] (rad/s)
%   28	 accSmooth[0] (m/s/s)
%   29	 accSmooth[1] (m/s/s)
%   30	 accSmooth[2] (m/s/s)
%   31	 debug[0]
%   32	 debug[1]
%   33	 debug[2]
%   34	 debug[3]
%   35	 motor[0]
%   36	 motor[1]
%   37	 motor[2]
%   38	 motor[3]
% The returned array has the following column order
%   1    time (s)
%   2    angular acceleration roll
%   3    angular acceleration pitch
%   4    angular acceleration yaw
%   5    rollrate
%   6    pitchrate
%   7    yawrate
%   8    acceleration x
%   9    acceleration y
%   10   acceleration z
%   11   motor 1 measured speed [rad/s]
%   12   motor 2 measured speed [rad/s]
%   13   motor 3 measured speed [rad/s]
%   14   motor 4 measured speed [rad/s]
%   15   motor 1 commanded speed [pwm units]
%   16   motor 2 commanded speed [pwm units]
%   17   motor 3 commanded speed [pwm units]
%   18   motor 4 commanded speed [pwm units]
%   19   battery voltage
%   20   battery current
% The smoothing is done using a savitzky-golay filter of different order
%   Attitude Rates: 4th order, 525 window
%   Acceleration:   4th order, 525 window
%   Motor Data:     two pass: first 2nd order, 121 window then 4/31
%   Battery Data:   Moving Average, Window 725

if nargin<2
    opt=create_options();
end

if opt.INTERPOLATE && ~opt.SPLINE
    disp("Interpolation only available with Splines. Ignoring it.");
    opt.INTERPOLATE = false;
end

dt = 1/round(1/mean(diff(data(:,2))));

if opt.INTERPOLATE
    tstart = min(data(:,2));
    tend = max(data(:,2));
    t = [tstart:dt:tend]';
else
    t = data(:,2);
end

if opt.SPLINE
    smooth_1 = 3e-10 / dt;
    smooth_2 = 1e-9 / dt;
end

denoised(:,1) = t;

% attitude rate (25-27)
for i = 25:27
    if opt.SPLINE
        dx = csaps(data(:,2), data(:,i), 1-smooth_1, t);
        d2x = gradient(dx, t);
        d2x = csaps(t, d2x, 1-smooth_2, t);
    else
        dx = smooth(data(:,i), 120, 'sgolay', 6);
        d2x = gradient(dx, denoised(:,1));
        d2x = smooth(d2x, 31, 'sgolay', 2);
    end
    denoised(:,i-23) = d2x;
    denoised(:,i-20) = dx;
end

% acceleration data
smooth_2 = 1e-7 / dt;
for i = 28:30
    if opt.SPLINE
        denoised(:,i-20) = csaps(data(:,2), data(:,i), 1-smooth_2, t);
    else
        denoised(:,i-20) = smooth(data(:,i), 500, 'sgolay', 2);
    end
end

% motor data from logs
% the data is smoothed two times with a short window to capture the rpm
% oscillations the motors see but still reduce the noise significantly
for i=31:34
    if opt.SPLINE
        [b, a] = butter(4, (2/0.033)*dt);
        denoised(:,i-20) = interp1(data(:,2), data(:,i), t) *  14*pi/30;
        denoised(:,i-20) = filtfilt(b,a,denoised(:,i-20) );
%         denoised(:,i-20) = csaps(data(:,2), data(:,i), 1-smooth_1, t) * 14*pi/30;
    else
        mot_s = smooth(data(:,i), 241, 'sgolay', 2);
        mot_ss = smooth(mot_s, 61, 'sgolay', 4);
        denoised(:,i-20) = mot_ss * 14*pi/30; % from eRPM to rad/s
    end
end


if opt.SMOOTHPLOTS
    labels = ["Roll Acc", "Pitch Acc", "Yaw Acc", "Roll Rate", "Pitch Rate", "Yaw Rate", "Acc X", "Acc Y", "Acc Z"];
    % Angular Accelerations
    figure;
    for i = 1:3
        subplot(1,3,i)
        hold on
        grid on
        plot(t, denoised(:,i+1), 'r', 'LineWidth', 1);
        title(labels(i));
    end
    % Angular Rates
    figure;
    for i = 1:3
        subplot(1,3,i)
        hold on
        grid on
        plot(data(:,2), data(:,i+24), 'k');
        plot(t, denoised(:,i+4), 'r', 'LineWidth', 1);
        title(labels(i+3));
    end
    % Linear Accelerations
    figure;
    for i = 1:3
        subplot(1,3,i)
        hold on
        grid on
        plot(data(:,2), data(:,i+27), 'k');
        plot(t, denoised(:,i+7), 'r', 'LineWidth', 1);
        title(labels(i+6));
    end
    % Motor Data
    figure;
    subplot(2,2,1);
    hold on
    title("Motor 1 Plot");
    plot(data(:,2), data(:,31)* 14*pi/30, 'k');
    plot(t, denoised(:,11), 'r', 'LineWidth', 1);
    
    subplot(2,2,2);
    hold on
    title("Motor 2 Plot");
    plot(data(:,2), data(:,32)* 14*pi/30, 'k');
    plot(t, denoised(:,12), 'r', 'LineWidth', 1);
    
    subplot(2,2,3);
    hold on
    title("Motor 3 Plot");
    plot(data(:,2), data(:,33)* 14*pi/30, 'k');
    plot(t, denoised(:,13), 'r', 'LineWidth', 1);
    
    subplot(2,2,4);
    hold on
    title("Motor 4 Plot");
    plot(data(:,2), data(:,34)* 14*pi/30, 'k');
    plot(t, denoised(:,14), 'r', 'LineWidth', 1);
end

% copy motor commands and interpolate maybe
if opt.INTERPOLATE
    denoised(:,15:18) = interp1(data(:,2), data(:,35:38), t);
else
     denoised(:,15:18) = data(:,35:38);
end

% battery data from logs
for i=22:23
    if opt.SPLINE
        denoised(:,i-3) = csaps(data(:,2), data(:,i), 0.99, t);
    else
        denoised(:,i-3) = smooth(data(:,i), 725);
    end
end

if opt.SMOOTHPLOTS
   figure; 
   hold on;
   title("Battery Voltage");
   plot(data(:,2), data(:,22), 'k');
   plot(t, denoised(:,19), 'k');
end

end