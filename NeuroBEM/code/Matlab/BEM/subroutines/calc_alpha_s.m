function alpha_s = calc_alpha_s(v)
%calc_alpha_s calculates the angle of attack of the propeller 
% alpha_s = calc_alpha_s(v) where
%   v       free-stream velocity of the propeller in body-frame
%   alpha_s angle of attack: alpha_s = atan2(v(3), norm(v(1:2)))
    alpha_s = atan2(v(3), norm(v(1:2)));
end