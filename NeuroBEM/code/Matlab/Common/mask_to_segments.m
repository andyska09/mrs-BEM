function segs=mask_to_segments(mask)
%mask_to_segments takes a binary mask and returns segements 'true' segements
% segs=mask_to_segments(mask) 
%   mask    Nx1 array where every value below 0.5 is considered 'true'
%   segs    Kx3 array where 
%               segs(:,1) contains start indices of segments where mask is true
%               segs(:,2) contains end indices of segments where mask is true
%               segs(:,3) contains length of the segements where mask is true
%               NOTE: bounds are inclusive!

% ensure boolean value of mask
mask = logical(mask > 0.5);
segs_end = find(abs(diff(mask))>0.5);

if ~isempty(segs_end)
    % first column: start of segement, second column: end of segment
    segs_start = [1; segs_end+1];
    segs_end = [segs_end; length(mask)];
    segs = [segs_start segs_end segs_end-segs_start+1];
    
    % fourth column contains truth value of the segement
    for i = 1:size(segs,1)
       segs(i,4) = mask(floor((segs_start(i)+segs_end(i))/2));
    end
    
    % Discard 'false' segments
    segs = segs(segs(:,4) > 0.5, 1:3);
else
    segs = [1 length(mask) length(mask)];
end

end