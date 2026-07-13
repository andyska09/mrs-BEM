function segs = find_consecutive_segments(data_bag)
%find_consecutive_segments analyzes where data_bag contains no missing data
% segs = find_consecutive_segments(data_bag)
%   data_bag an array where the 3rd column is interpreted as the time
%   segs    Kx3 array where 
%               segs(:,1) contains start indices of segments 
%               segs(:,2) contains end indices of segments 
%               segs(:,3) contains length of the segements 
% A segment is defined as points where no datalag > 0.15 seconds was recorded

dt = 1/round(1/mean(diff(data_bag(:,3))));

% Good segments have delta_ts of less than 0.15s
segs = mask_to_segments((circshift(data_bag(:,3),-1) - data_bag(:,3)) < 0.15);

% Discard segments shorter than 1 second
segs = segs(segs(:,3)*dt > 1,:);
fprintf("Found %d segments > 1s\n", size(segs,1));

if isempty(segs) || size(segs,1) < 1
    error('No segment of sufficient length found');
end

end