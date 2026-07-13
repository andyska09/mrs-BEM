function denoised=bag_smoother(data, opt)
%bag_smoother: the function takes data from rosbags and denoises them
% y = bag_smoother(x, opt) takes a data array and returns a denoised
% version of the same size. The opt struct can be used to set the parameters
%  opt.SMOOTHPLOTS  if true, plots of all quantities will be shown
%  opt.SPLINE       if true, a splien based interpolation instead of
%                   Savitzky-Golay will be used. Good for rapidly varying data
%                   and also performs outlier rejection
%  opt.INTERPOLATE  if true (default) the returned array will be uniformly
%                   sampled in time. Only available with SPLINE.
% Note that the column order of data must be
% ROS Bag
%   1   time
%   2	field.header.seq
%   3	field.header.stamp
%   4	field.header.frame_id
%   5	field.pose.position.x
%   6	field.pose.position.y
%   7	field.pose.position.z
%   8   field.pose.orientation.x
%   9	field.pose.orientation.y
%   10	field.pose.orientation.z
%   11	field.pose.orientation.w
%   12  roll
%   13  pitch
%   14  yaw
% The return array has the following column order:
%   1   time (s)
%   2   angular acceleration body x
%   3   angular acceleration body y
%   4   angular acceleration body z
%   5   angular velocity body x
%   6   angular velocity body y
%   7   angular velocity body z
%   8   quaterion q.x 
%   9   quaterion q.y
%   10  quaterion q.z
%   11  quaterion q.w
%   12  acceleration x
%   13  acceleration y
%   14  acceleration z
%   15  velocity x
%   16  velocity y
%   17  velocity z
%   18  position x
%   19  position y
%   20  position z
% The smoothing is done using a savitzky-golay filter of different order if the 
% corresponding parameter is not set to SPLINE.
%   Position: 4th order, window 61
%   Velocity: 3rd order, window 45
%   Acceleration: 2nd order, window 31
%   Attitude: 6th order, window 41
%   Attitude Rates: 4th order, window 31
%   Angular Acceleration: 2nd order, window 31

if nargin<2
    opt = create_options();
end

if opt.INTERPOLATE && ~opt.SPLINE
    disp("Interpolation only available with Splines. Ignoring it.");
    opt.INTERPOLATE = false;
end

dt = 1/round(1/mean(diff(data(:,3))));
if opt.INTERPOLATE
    tstart = min(data(:,3));
    tend = max(data(:,3));
    t = [tstart:dt:tend]';
else
    t = data(:,3);
end

den = {};

if opt.SPLINE
    smooth_0 = 2.5e-9 / dt;
    smooth_0_reject = 1e-7 / dt;
    smooth_1 = 1e-7 / dt;
    smooth_2 = 1e-7 / dt;
end

% acceleration, velocity, position from rosbag
for i = 1:7
    x = data(:,4+i);
    if opt.SPLINE
        res = csaps(data(:,3), x, 1-smooth_0_reject, data(:,3))-x;
        idx_bad = abs(res) > 3*std(res);
        x = csaps(data(~idx_bad,3), x(~idx_bad), 1-smooth_0, t);
        dx = gradient(x, t);
        dx = csaps(t, dx, 1-smooth_1, t);
        d2x = gradient(dx, t);
        d2x = csaps(t, d2x, 1-smooth_2, t);
    else
        x = smooth(t, x, 61, 'sgolay', 4);
        dx = gradient(x, t);
        dx = smooth(t, dx, 45, 'sgolay', 3);
        d2x = gradient(dx, t);
        d2x = smooth(t, d2x, 31, 'sgolay', 2);
    end
    
    if i==3
        d2x = d2x + 9.81;
    end
    den{i}(:,1) = d2x;
    den{i}(:,2) = dx;
    den{i}(:,3) = x;
end


denoised = zeros(size(t,1), 19);
denoised(:,1) = t;

for i = 1:3
    tmp = den{i};
    denoised(:,11 + [i, 3+i, 6+i]) = tmp;
end

q = [];
qd = [];
qdd = [];
for i = 1:4
    tmp = den{i+3};
    q = [q tmp(:,3)];
    qd = [qd tmp(:,2)];
    qdd = [qdd tmp(:,1)];
end
denoised(:,8:11) = q;
qinv = @(q) conj( q )./(q * conj(q));
qmult = @(p, q) p * q;
q   = quaternion(q(:,  [4 1:3]));
norms = norm(q);
q = 1./norms .* q;
qd  = 1./norms .* quaternion(qd(:, [4 1:3]));
qdd = 1./norms .* quaternion(qdd(:,[4 1:3]));
qi  = arrayfun(qinv, q);
omega = 2 * arrayfun(qmult, qi, qd);
omega_dot = 2 * (arrayfun(qmult, qi, qdd) - 1/4*arrayfun(qmult, omega, omega));
[~, denoised(:,2), denoised(:,3), denoised(:,4)] = parts(omega_dot);
[~, denoised(:,5), denoised(:,6), denoised(:,7)] = parts(omega);

if opt.SMOOTHPLOTS
    labels = ["Acc X", "Acc Y", "Acc Z"; ...
              "Velocity X", "Velocity Y", "Velocity Z"; ...
              "Position X", "Position Y", "Position Z"];
    for j = 1:3
        figure;
        for i = 1:3
            subplot(1,3,i);
            hold on
            grid on
            if j == 3
                plot(data(:,3), data(:,4+i), 'k.');
                %plot(data(:,4+i), 'k.');
            end
            plot(denoised(:,1), denoised(:,12+(j-1)*3+(i-1)),'r');
            %plot(denoised(:,11+(j-1)*3+(i-1)),'r');
            if j == 3
                legend("Measurement", "Smoothed");
            end
            title(labels(j,i));  
        end
    end
    labels = ["Angular Acc X", "Angular Acc Y", "Angular Acc Z"; ...
              "Rollrate", "Pitchrate", "Yawrate"; ...
              "Roll", "Pitch", "Yaw"];
    for j = 1:3
        figure;
        for i = 1:3
            subplot(1,3,i);
            hold on
            grid on
            if j == 3
                plot(data(:,3), data(:,11+i), 'k');
                tmp = quat2eul(denoised(:,[11 8:10]));
                plot(denoised(:,1), tmp(:,4-i),'r');
            else
                plot(denoised(:,1), denoised(:,2+(j-1)*3+(i-1)),'r');
            end
            if j == 3
                legend("Measurement", "Smoothed");
            end
            title(labels(j,i));  
        end
    end
end

end
