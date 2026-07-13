function segs = find_segments(idx)
%find_segments finds segments in index-data
% segs = find_segments(idx) return an N by 3 array which contains all
% intervals where the idx variable (of size K by 1) was increasing by 1
% between to consecutive elements. The returned array is of the form
%   [start end length]
% where each line corresponds to one segment of continuous increase of idx.

seg_start = idx(idx - circshift(idx,1) ~= 1);
seg_end = idx(idx - circshift(idx,-1) ~= -1);
segs = [seg_start seg_end seg_end-seg_start];
segs = segs(2:end,:);
segs = segs(segs(:,3)>0,:);
end