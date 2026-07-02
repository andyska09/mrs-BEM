function [data_bag, t0] = read_bag(filename, topicName)
%data_log: reads a rosbag csv
% data = read_log(fname) reads the bag file named fname into the matrix
% data. The first column contains the time stamps in s and the other
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
% Bag File (command): (pass name of the topic as second argument)
%   1   time [s]
%   2   mass normalized total thrust [m/s^2]
%   3   body rates x [rad/s]
%   4   body rates y [rad/s]
%   5   body rates z [rad/s]
if nargin < 2
    topicName = "vicon";
end

f = convertStringsToChars(filename);
if (f(end-2:end) == "csv")
    [data_bag, t0] = read_bag_csv(filename);
elseif (f(end-2:end) == "bag")
    bag = rosbag(filename);
    topics = string(bag.AvailableTopics.Properties.RowNames);
    
    % Find start reference time from vicon data
    tmp = select(bag,'Topic',topics(find(contains(topics, "vicon"), 1, 'first')));
    messages = readMessages(tmp,'DataFormat','struct');
    t0 = double(messages{1}.Header.Stamp.Sec) + double(messages{1}.Header.Stamp.Nsec) / 1e9;
    
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
    
    if contains(topics(idx_topic), "vicon")
        data_bag = zeros(size(messages,1),14);
        for i = 1:size(data_bag,1)
           data_bag(i, 3) =  double(messages{i}.Header.Stamp.Sec) ...
                             + double(messages{i}.Header.Stamp.Nsec) / 1e9;
           data_bag(i,5) = messages{i}.Pose.Position.X;
           data_bag(i,6) = messages{i}.Pose.Position.Y;
           data_bag(i,7) = messages{i}.Pose.Position.Z;
           data_bag(i,8) = messages{i}.Pose.Orientation.X;
           data_bag(i,9) = messages{i}.Pose.Orientation.Y;
           data_bag(i,10) = messages{i}.Pose.Orientation.Z;
           data_bag(i,11) = messages{i}.Pose.Orientation.W;
        end
        data_bag(:,3) = data_bag(:,3) - t0;
        data_bag(:,14:-1:12) = quat2eul(data_bag(:,[11 8:10]));
    elseif contains(topics(idx_topic), "command")
        data_bag = zeros(size(messages,1), 5);
        for i = 1:size(data_bag,1)
           data_bag(i,1) = messages{i}.T - t0;
           if messages{i}.IsSingleRotorThrust
               data_bag(i,2) = sum(messages{i}.Thrusts);
           else
               data_bag(i,2) = messages{i}.CollectiveThrust;
           end
           data_bag(i,3) = messages{i}.Bodyrates.X;
           data_bag(i,4) = messages{i}.Bodyrates.Y;
           data_bag(i,5) = messages{i}.Bodyrates.Z;
        end
        data_bag = data_bag(data_bag(:,1) > 0,:);
    end
    
else
    fprintf("Filename %s not recognized\n", filename);
end

end