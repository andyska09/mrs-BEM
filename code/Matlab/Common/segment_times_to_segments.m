function segs=segment_times_to_segments(segs_times, time)
%segment_times_to_segments takes a segment arra and returns a binary mask
% segs=segment_times_to_segments(segs_times, time)
%   segs_times Kx3 array where 
%               segs_times(:,1) contains start times of segments
%               segs_times(:,2) contains end times of segments
%               segs_times(:,3) contains length of the segements
%               NOTE: bounds are inclusive!
%   time    Nx1 array with reference times
%   segs    Kx3 array where 
%               segs(:,1) contains start indices of segments where mask is true
%               segs(:,2) contains end indices of segments where mask is true
%               segs(:,3) contains length of the segements where mask is true
%               NOTE: bounds are inclusive and indices taken from time array

if isempty(segs_times)
    segs = [];
    return
end

segs = zeros(size(segs_times));
for i = 1:size(segs_times,1)
    [~, segs(i,1)] = min(abs(segs_times(i,1)-time));
    [~, segs(i,2)] = min(abs(segs_times(i,2)-time));
end
segs(:,3) = segs(:,2) - segs(:,1) + 1;

end