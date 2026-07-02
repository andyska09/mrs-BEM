function traj_xyz=evaluateTrajectory(time, f, DBG)
%evaluateTrajectory returns an array with evaluated functions
% traj_xyz=evaluateTrajectory(time, f, [DBG]) where
%   time    N x 1   vector of times t where f is evaluated
%   f       K x L   matrix of symbol functions where f(:,l) contains the up to
%                   the K-1 th derivative of f(1,l)
%   [DBG]   bool    optional, default false; generate plot (for 4x3 input f)
% Returns traj_xyz of size K x L x N containing the function from f evaluated at
%   the specified times.
% NOTE: if L=1, the dimension is omitted.

if nargin < 3
    DBG=false;
end

% the array where the functions are evaluated is 4x3xN where
%   K components correspond to position, velocity, acceleration, jerk
%   L components correspond to the cartesian dimensions
%   N is the length of the trajectory (number of samples)
N = length(time);
K = size(f,1);
L = size(f,2);
traj_xyz = zeros([K L N]);
for dim = 1:L
    for deriv = 1:K
        fcn = matlabFunction(f(deriv, dim));
        
        % catch if the derivative is constant
        if nargin(fcn) == 0
            traj_xyz(deriv,dim,:) = fcn();
        else
            traj_xyz(deriv,dim,:) = fcn(time);
        end
    end
end

traj_xyz = squeeze(traj_xyz);

if DBG && K==4 && L ==3
    figure;
    subplot(3,2,[1 3 5]);
    plot3(  squeeze(traj_xyz(1,1,:)), ...
        squeeze(traj_xyz(1,2,:)), ...
        squeeze(traj_xyz(1,3,:)));
    grid on
    title("3D Position");
    xlabel("X [m]");
    ylabel("Y [m]");
    zlabel("Z [m]");
    
    ax(1) = subplot(3,2,2);
    hold on
    plot(time, squeeze(traj_xyz(1,1,:)), 'r');
    plot(time, squeeze(traj_xyz(1,2,:)), 'g');
    plot(time, squeeze(traj_xyz(1,3,:)), 'b');
    legend("x", "y", "z");
    grid on;
    ylabel("Position [m]");
    
    
    ax(2) = subplot(3,2,4);
    hold on
    plot(time, squeeze(traj_xyz(2,1,:)), 'r');
    plot(time, squeeze(traj_xyz(2,2,:)), 'g');
    plot(time, squeeze(traj_xyz(2,3,:)), 'b');
    plot(time, vecnorm(squeeze(traj_xyz(2,1:3,:)),2,1), 'k');
    legend("x", "y", "z", "||.||");
    grid on;
    ylabel("Velocity [m/s]");
    
    
    ax(3) = subplot(3,2,6);
    hold on
    plot(time, squeeze(traj_xyz(3,1,:)), 'r');
    plot(time, squeeze(traj_xyz(3,2,:)), 'g');
    plot(time, squeeze(traj_xyz(3,3,:)), 'b');
    plot(time, vecnorm(squeeze(traj_xyz(3,1:3,:)),2,1), 'k');
    legend("x", "y", "z", "||.||");
    grid on;
    xlabel("Time [s]");
    ylabel("Acceleration [m/s^2]");
    
    linkaxes(ax, 'x');
end

end