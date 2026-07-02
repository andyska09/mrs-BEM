function mu=calc_mu(v, Omega)
%calc_mu returns the advance radio v_xy/(Omega * R)
% mu=calc_mu(v, Omega) where 
%   v       free-stream velocity of the propeller in body-frame
%   Omega   angular velocity of the propeller in rad/s
%   mu      advance ratio (|v|/(Omega*R))
    param = setParam();
    mu = sqrt(v(1)^2 + v(2)^2)/(Omega*param.R);
end