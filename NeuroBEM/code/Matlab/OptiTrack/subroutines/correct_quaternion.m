function data_bag = correct_quaternion(data_bag, idx_bad, DBG)
%correct_quaternion: the function corrects quaternion flips from rosbags
% data_bag = correct_quaternion(data_bag, idx_bad, DBG)
%  data_bag     N x K array, the rosbag as it is read-in by read_rosbag.
%               The input order is identical to what bag_smoother expects
%               (see its help for more information)
%  idx_bad      optional, N x 1 boolean array. Ignores datapoints (i.e. all
%               zeros should be excluded)
%  DBG          If set to true, debugplots are printed
% Returns
%  data_bag     N x K array with the quaternions unflipped

if nargin<3 
    DBG=false;
end
if nargin<2
    idx_bad = [];
end

v1 = sum(abs(data_bag(:,8:11) - circshift(data_bag(:,8:11),1)),2);
v2 = sum(abs(data_bag(:,8:11) + circshift(data_bag(:,8:11),1)),2);
if isempty(idx_bad)
    flip_idx = (v2 < 0.95*v1);
else
    flip_idx = (v2 < 0.95*v1) .* ~idx_bad;
end
flip_idx = mod(cumsum(flip_idx),2);

data_bag(:,8:11) = data_bag(:,8:11) .* (2*flip_idx-1);

if DBG
    figure;
    hold on;
    plot(data_bag(:,3), v1, 'r');
    plot(data_bag(:,3), v2, 'b');
    plot(data_bag(:,3), flip_idx, 'k');
    grid on
    figure;
    hold on;
    plot(data_bag(:,3), data_bag(:,8), 'r')
    plot(data_bag(:,3), data_bag(:,9),'g');
    plot(data_bag(:,3), data_bag(:,10),'b');
    plot(data_bag(:,3), data_bag(:,11), 'k');
    grid on;
end
end