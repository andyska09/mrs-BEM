function data_res = resample_log(tref, t, data)
%resample_log: The function resamples nun-uniform data
% data = resample_log(tref, t, x) takes a dataset x (of size NxM) and a
% corresponding array of measurement times t (of size Nx1) and resamples it
% according to the equally spaced times passed in tref (of size Kx1). The
% function returns a K by M+1 array where the first column contains the
% time vector tref.

dt = mean(diff(tref));

% find which indices are the start of the segments of t_ref
idx = zeros(size(tref));
for i=1:length(idx)
    tmp = find(t > tref(i)-dt/2, 1, 'first');
    if ~isempty(tmp)
        idx(i) = tmp;
    else
        idx(i) = size(tref,1);
    end
end

% the segement ends when the next one start (second column contains ends)
idx(:,2) = circshift(idx,-1);
idx(end,2) = length(t);

% sometimes there was no data during a time segement. then make the
% segment longer
err_idx = find(idx(:,2)-idx(:,1) == 0);
for ei = err_idx'
   idx(ei,2) = find(idx(:,2) > idx(ei,2), 1, 'first');
end

% calculate resampled data
data_res = zeros(length(tref), size(data,2)+1);
for i=1:length(tref)
    data_res(i,:) = [tref(i), mean(data(idx(i,1):idx(i,2),:),1)];
end

end
