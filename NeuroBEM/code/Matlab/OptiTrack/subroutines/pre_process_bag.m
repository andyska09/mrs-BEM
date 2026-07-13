function data_bag = pre_process_bag(data_bag)
%pre_process_bag takes a read-in Rosbag and removes errornous
%data together with quaterion flips.
% data_bag = pre_process_bag(data_bag) 
% Note that the column order must follow what is specified in bag_smoother,
% see its help for details

% Remove Error-Data where the vehicle was not airborne
z0 = min(min(data_bag(data_bag(:,7) ~= 0,7)), 0.2);      % limit zero height to 20cm
idx_good = logical((data_bag(:,7) > z0 + 0.1) ...        % not sitting on ground
           .* (vecnorm(data_bag(:,8:11),2,2) > 0.8));    % valid quaternion
           
idx_bad = ~idx_good;

% Dilatate the filtering process by two samples 
idx_bad = movmax(idx_bad, 3);
idx_good = ~idx_bad;


% Correct Rollover of Quaternions
data_bag = correct_quaternion(data_bag, idx_bad, false);

% Extract Indices where the quaternions are very noisy
idx_bad_new = sum(abs(data_bag(:,8:11)-movmean(data_bag(:,8:11),10)),2) > 0.5;
idx_bad_new = movmax(idx_bad_new, 3);
idx_bad = idx_bad | idx_bad_new;
idx_good = ~idx_bad;
data_bag = data_bag(idx_good, :);

data_bag = correct_quaternion(data_bag, [], false);
end