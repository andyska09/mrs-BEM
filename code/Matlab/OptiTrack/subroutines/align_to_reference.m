function data = align_to_reference(data, ref, dataidx, refidx, resample_trim)
%align_to_reference aligns two datasets w.r.t. some columns
% data = align_to_reference(data, ref, dataidx, refidx, resample_trim)
% where
%   data    N x L array of datapoints that need to be aligned. The first
%           column must contain the time stamps for the other columns.
%   ref     M x K array of datapoints that serve as a reference. The first
%           column must contain the time stamps for the other columns.
%   dataidx Indices of the columns that are used for the time-alignment in
%           the array data.
%   dataidx Indices of the columns that are used for the time-alignment in
%           the array ref.
%   resample_trim optional, boolean that determines whether the returned
%           dataset is resample and trimmed to match the reference.
if (nargin < 5)
    resample_trim = false;
end


if (length(dataidx) ~= length(refidx))
    message("Wrong length provided");
    return;
end



dt = min(mean(diff(ref(:,1))), mean(diff(data(:,1))));

t  = [min(ref(1,1), data(1,1)) ... 
      :dt: ...
      max(ref(end,1), data(end,1)) ...
      ];

delta = 0;
for i = 1:length(dataidx)
    reference = ref(:,refidx(i)) - mean(ref(:,refidx(i)));
    datastream = data(:,dataidx(i)) - mean(data(:,dataidx(i)));
    
    reference = interp1(ref(:,1), reference, t, 'linear', 0);
    reference(isnan(sum(reference,2)),:) = [];
    datastream = interp1(data(:,1), datastream, t, 'linear', 0); 
    delta = delta + finddelay(reference, datastream);
end
delta = abs(floor(delta/length(dataidx))+1);

t = delta * mean(dt);
data(:,1) = data(:,1) - t;

if resample_trim
   data = interp1(data(:,1), data, ref(:,1), 'linear', NaN);
   data(any(isnan(data),2), :) = [];
end

end