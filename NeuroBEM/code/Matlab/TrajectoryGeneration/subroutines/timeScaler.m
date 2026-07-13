function tadj = timeScaler(duration, scale, vel_scale, DBG)
%timeScaler scales the time to generate smooth starts/ends of trajectories
% tadj = timeScaler(duration, scale, vel_scale, DBG) where
%   duration    double, duration of the experiment
%   scale       string, can be "none", "linear", "sqrt"
%   vel_scale   double, maximum velocity scaling that will be reached
%   DBG         optional, outputs plot of time and velocity profile scale
%   tadj        function handle

if nargin < 4
    DBG = false;
end


tdescale = @(t) duration * t/2;
tscale = @(t) 2*(t/duration - 0.5);
if scale == "none"
    tadj = @(t) t;
elseif scale == "constant"
    tadj = @(t) vel_scale*t;
elseif scale == "linear"
    syms a
    tadj = @(t) tdescale(vel_scale*int(1-sqrt(a^2 + 1E-4), a, -1, tscale(t)));
elseif scale == "sqrt"
    syms a
    tadj = @(t) tdescale(int(vel_scale*(1-a^2), a, -1, tscale(t)));
elseif scale == "aggr"
    syms a
    tadj = @(t) tdescale(int(vel_scale*(1-a^8), a, -1, tscale(t)));
end

if DBG
    syms x
    tadj_fcn = matlabFunction(tadj(x));
    tarr = linspace(0,duration,500);
    tscaled = tadj_fcn(tarr);
    figure;
    subplot(2,1,1)
    hold on
    grid on
    title("Time Scaling");
    plot(tarr, tscaled, 'b');
    xlabel("Original Time [s]");
    ylabel("Scaled Time [s]");
    
    subplot(2,1,2);
    title("Time Scaling");
    plot(tarr, gradient(tscaled,tarr), 'b');
    xlabel("Original Time [s]");
    ylabel("Velocity Scaling Factor");
end

end