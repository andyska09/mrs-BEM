function cd = calc_cd(alpha)
%calc_cd returns the drag coefficient approximated by a three-term drag polar
% cd = calc_cd(alpha) where
%   alpha       angle of attack
%   cd          drag coefficient
    param = setParam();
    cd = param.cd0 + param.cd1.*alpha + param.cd2.*alpha.^2;
end