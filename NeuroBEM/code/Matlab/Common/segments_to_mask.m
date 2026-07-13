function mask=segments_to_mask(segs, length)
%segements_to_mask takes a segment arra and returns a binary mask
% mask=segements_to_mask(segs)
%   segs    Kx3 array where 
%               segs(:,1) contains start indices of segments where mask is true
%               segs(:,2) contains end indices of segments where mask is true
%               segs(:,3) contains length of the segements where mask is true
%               NOTE: bounds are inclusive!
%   length  required length of the mask (optional, must be >= segs(end,2))
%   mask    Nx1 array where filled with 0's and 1's according to segs. 
%               returns -1 if segs is empty
if isempty(segs)
    mask = -1;
    return 
end
if nargin < 2
    length = segs(end,2);
end

mask = zeros(length,1);
for i = 1:size(segs,1)
    mask(segs(i,1):segs(i,2)) = 1;
end

end