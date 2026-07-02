function omega=diffQuaternionToOmega(q, time)
%diffQuaternionToOmega calculates the angular velocity (body frame)
% omega=diffQuaternionToOmega(q, time)
%   q       Nx1 quaternion array
%   time    optional; Nx1 array
% if time is not provided, then a constant timestep of 1 is assumed.
% Returns the angular rate in body frame

if nargin < 2
    time = [1:length(q)]';
end

qd = zeros([length(q), 4]);
qarr = compact(q);

% v1 = sum(abs(qarr - circshift(qarr,1)),2);
% v2 = sum(abs(qarr + circshift(qarr,1)),2);
% flip_idx = (v2 < 0.95*v1);
% flip_idx = mod(cumsum(flip_idx),2);
% 
% qarr = qarr .* (2*flip_idx-1);
% figure;
% plot(qarr)

for i = 1:4
    qd(:,i) = gradient(qarr(:,i),time);
end
qd = quaternion(qd);

% Functions for better parallelization
qinv = @(q) conj( q )./(q * conj(q));
qmult = @(p, q) p * q;

qi  = arrayfun(qinv, q);
omega = compact(2 * arrayfun(qmult, qi, qd));
omega = omega(:, 2:end);
end
