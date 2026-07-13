function [data_bag, t0] = read_bag_csv(filename)
%data_log: reads a rosbag csv
% data = read_log(fname) reads the bag file named fname into the matrix
% data. The first column contains the time stamps in s and the other
% columns are returned as follows:
% Bag File: 
%   1   time
%	2	field.header.seq
%	3	field.header.stamp
%	4	field.header.frame_id
%	5	field.pose.position.x
%	6	field.pose.position.y
%	7	field.pose.position.z
%	8	field.pose.orientation.x
%	9	field.pose.orientation.y
%	10	field.pose.orientation.z
%	11	field.pose.orientation.w
%   12  roll
%   13  pitch
%   14  yaw
% The csv should be generated using the command 
%       rostopic echo -b file.bag -p /topic

data_bag = csvread(filename,1);
t0 = data_bag(1,3);
data_bag(:,1:3) = (data_bag(:,1:3) - data_bag(1,1:3));
data_bag(:,[1 3]) = data_bag(:,[1 3])/1E9;
data_bag(:,14:-1:12) = quat2eul(data_bag(:,[11 8:10]));

end