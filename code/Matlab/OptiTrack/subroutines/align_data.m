function [offset, slope, R, x, y, w] = align_data(t_ref, ref, ty, y, add_corr, DBG)
% align_data precisely align a dataset with a given reference
% [offset, slope, R, x, y, w] = align_data(t_ref, ref, ty, y, add_corr, DBG)
%   t_ref       N by 1 array of reference times
%   ref         N by 1 array of reference values at times t_ref
%   ty          M by 1 array of times to be aligned
%   y           M by 1 array of values at times ty
%   add_corr    K by 3 array of values [x,y,w] from previous analysis, optional
%   DBG         set to true to produce debug plots, optional
% Returns 
%   offset      constant time offset w.r.t reference times
%   slope       slope of clock skew w.r.t reference
%   R           R^2 measure of the correlation for the offset / slope fit
%   x           time-stamps of the points used to fit offset/slope
%   y           shifts that maximised the correlation of ref and y at times x
%   w           weight used during fitting process

if nargin < 5 || isempty(add_corr) || size(add_corr,2) ~= 3
    ADD = false;
else 
    ADD = true;
end
if nargin < 6
    DBG = false;
end

% calculate mean dt of reference time data and calculate overlapping time
mean_dt = mean(diff(t_ref));
t_start = max(t_ref(1), ty(1));
t_final = min(t_ref(end), ty(end));

% generate uniformly spaced grid for correlation calculation and
% interpolate reference and data values
correl_data = [t_start:mean_dt:t_final]';
correl_data(:,2) = interp1(t_ref, ref, correl_data(:,1));
correl_data(:,3) = interp1(ty, y, correl_data(:,1));

% write seg start and seg end in unit 'seconds' to column 1 and 2
% write segment midpoint to column 3
ts_spacing     = 5; 
ts_corr_length = 1;
segs_corr = [t_start+ts_corr_length:ts_spacing:t_final-ts_corr_length-ts_spacing]';
segs_corr(:,2) = segs_corr(:,1)+ts_spacing;
segs_corr(:,3) = (segs_corr(:,1)+segs_corr(:,2))/2;

% write seg start and seg end in unit 'index' to columns 4 and 5
for i=1:size(segs_corr,1)
    segs_corr(i,4) = find(correl_data(:,1) > segs_corr(i,1),1,'first');
    segs_corr(i,5) = find(correl_data(:,1) < segs_corr(i,2),1,'last');
end

% set up correlation calculation:
%  corr_length is the length (in index units) of the correlation window
%  corr_lags is the number of lags (in index units)
corr_length = floor(ts_corr_length/mean_dt);
corr_lags = -floor(corr_length/2):floor(corr_length/2);

% perform the actual calculation of the correlation in every window defined
% in the segs_corr table. Note that we use a zero-mean normalized
% correlation here to be independent of data scaling.
for i=1:size(segs_corr,1)
    istart = segs_corr(i,4);
    iend = segs_corr(i,5);
    x = correl_data(istart:iend,2);
    x = x - mean(x);
    for j=corr_lags
        y = correl_data(istart+j:iend+j,3);
        y = y - mean(y);
        segs_corr(i,8+floor(corr_length/2)+j) = max(x'*y/sqrt(x'*x * y'*y), 0);
    end
end

if DBG
    figure;
    plot(segs_corr(:,8:end)');
end

% find which time index had the highest correlation and store the index in
% column 8 of segs_corr. The corresponding correlation is stored in the 
% column 7.
[segs_corr(:, 6), segs_corr(:, 7)] = max(segs_corr(:,8:end),[],2);
segs_corr(:, 7) = corr_lags(segs_corr(:, 7));

% now fit the index of the highest correlation against the segmenet time
% midpoint. Perform a weighted least-square fit where the weights are given
% by the correlation value.
% this fits the line in a maximum a-prior probablity setting
mdl = fitlm(segs_corr(:,3),segs_corr(:,7),'Weights', segs_corr(:,6));
coeff_corr = table2array(mdl.Coefficients(:,1))';
fit_idxshift = @(idx) idx*coeff_corr(2) + coeff_corr(1);

% based on the new information from the initial fit, we can improve the fit
% by using an a-posterior estimate. The probability that the index deviates
% from the fit is assumed to be gaussian.
% if there is data from another previous fit, this can be incorporated here
% to make it possible to fit multiple axis at once
sigma = 30;
corr_prob = @(idx, shift) exp(-(fit_idxshift(idx)-shift).^2/(2*sigma^2));
[segs_corr(:, 6), segs_corr(:, 7)] = ...
      max(segs_corr(:,8:end).*corr_prob(segs_corr(:,1), corr_lags),[],2);
segs_corr(:, 7) = corr_lags(segs_corr(:, 7))*mean_dt;

x = segs_corr(:,3);
y = segs_corr(:,7);
w = segs_corr(:,6);
if ADD
   add_len = floor(size(add_corr,1)/size(segs_corr, 1));
   x = [x; add_corr(:,1)];
   y = [y; add_corr(:,2)];
   w = [w; add_corr(:,3)];
end

mdl = fitlm(x, y,'Weights', w);
coeff_corr = table2array(mdl.Coefficients(:,1))';
fit_idxshift = @(idx) idx*coeff_corr(2) + coeff_corr(1);

slope = coeff_corr(2);
offset = coeff_corr(1);
R = mdl.Rsquared.Ordinary;

% if the debug function is active, plot the clockdrift
if DBG
    figure;
    hold on
    scatter(x, y, 50*w, 'b', 'filled');
    plot(x, fit_idxshift(x),'r');
    grid on
    title("Clock Drift");
    xlabel("Time [s]");
    ylabel("Drift [s]");
    legend("", "Drift: " + num2str(100*slope, "%5.3f") + " %");
end
end