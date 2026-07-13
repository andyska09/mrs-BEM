function [data_bag, t0] = read_rotors_bag(filename)
%data_log: reads a rosbag from RotorS
% data = read_rotors_bag(fname) reads the bag file named fname into the 
% matrix data. The first column contains the time stamps in s and the other
% columns are returned as follows:
% Bag File or CSV file (vicon): 
%   1   time
%   2	field.header.seq
%   3	field.header.stamp
%   4	field.header.frame_id
%   5	field.pose.position.x
%   6	field.pose.position.y
%   7	field.pose.position.z
%   8	field.pose.orientation.x
%   9	field.pose.orientation.y
%   10	field.pose.orientation.z
%   11	field.pose.orientation.w
%   12  roll
%   13  pitch
%   14  yaw


topicName = "pose";

bag = rosbag(filename);
topics = string(bag.AvailableTopics.Properties.RowNames);
    
% Find desired topic
idx_topic = find(contains(topics, topicName), 1, 'first');
if isempty(idx_topic)
    fprintf("Topic not found. Available topics are\n")
    topics
    t0 = 0;
    data_bag = 0;
    return
end
topic = select(bag,'Topic',topics(idx_topic));
messages = readMessages(topic,'DataFormat','struct');
t0 = 0;

data_bag = zeros(size(messages,1),14);
for i = 1:size(data_bag,1)
    data_bag(i,3) =  i * 0.01;
    data_bag(i,5) = messages{i}.Position.X;
    data_bag(i,6) = messages{i}.Position.Y;
    data_bag(i,7) = messages{i}.Position.Z;
    data_bag(i,8) = messages{i}.Orientation.X;
    data_bag(i,9) = messages{i}.Orientation.Y;
    data_bag(i,10) = messages{i}.Orientation.Z;
    data_bag(i,11) = messages{i}.Orientation.W;
end
data_bag(:,3) = data_bag(:,3) - t0;
data_bag(:,14:-1:12) = quat2eul(data_bag(:,[11 8:10]));


end