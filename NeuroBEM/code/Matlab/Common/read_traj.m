function data_traj = read_traj(filename, bodyframe)
%data_traj: reads a trajectory file
% data = read_traj(fname, B) reads the trajectory named fname into the matrix
% data. The velocity and acceleration are transformed into the body frame,
% unless the boolean B is set to false. Default: true
% The column order is as in agilicious, i.e.
%   1  time
%   2  position x
%   3  position y
%   4  position z
%   5  quaterion q.w 
%   6  quaterion q.x
%   7  quaterion q.y
%   8  quaterion q.z
%   9  velocity x
%  10  velocity y
%  11  velocity z
%  12  angular velocity body x
%  13  angular velocity body y
%  14  angular velocity body z
%  15  acceleration x
%  16  acceleration y
%  17  acceleration z
%  18  angular acceleration body x
%  19  angular acceleration body y
%  20  angular acceleration body z

if nargin < 2
    bodyframe = true;
end

data_traj = csvread(filename,1);

% Transform velocity and acceleration from world to body frame
fun_rotate_vel = @(x) [x(:,1:8),rotateframe(quaternion(x(:,[5:8])), x(:,9:11)), x(:,12:end)];
fun_rotate_acc = @(x) [x(:,1:14), rotateframe(quaternion(x(:,[5:8])), x(:,15:17)), x(:,18:end)];

data_traj = fun_rotate_vel(data_traj);
data_traj = fun_rotate_acc(data_traj);
end