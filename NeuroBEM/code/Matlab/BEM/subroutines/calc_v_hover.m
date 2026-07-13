function v_hover = calc_v_hover(T)
%calc_v_hover calculates the induced velocity at hover conditions given a
% thrust T
% v_hover = calc_v_hover(T)
    param = setParam();
    v_hover = sqrt(T/(2*param.rho*param.A));
end