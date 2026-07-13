function T = calc_T_numInt(v, K, mu, Omega, v1, a0, a1s, b1s)
%calc_T_numInt calculates thrust using numerical integration
% T = calc_T_numInt(v, K, mu, Omega, v1, a0, a1s, b1s)  where
%   v       free-stream velocity of the propeller in body-frame
%   K       thrust-distortion factor (usually 0)
%   mu      advance ratio (|v|/(Omega*R))
%   Omega   angular velocity of the propeller in rad/s
%   v1      induced velocity
%   a0      coning angle
%   a1s     lateral flapping angle
%   b1s     longitudinal flapping angle
% Returns
%   T       numerically integrated thrust using blade-element theory
    param = setParam();
    f_int_T = @(r, Psi) diffThrust(r,Psi);
    T = param.b * param.rho / (4*pi) * ...
        integral2(f_int_T, 0, param.R, 0, 2*pi);
    
    function dT = diffThrust(r, Psi)
        sPsi = sin(Psi);
        cPsi = cos(Psi);
        beta = a0 -  a1s.*cPsi - b1s.*sPsi;
        U_T = Omega .* (r + param.R.*mu.*sPsi);
        U_P = v(3) - v1*(1+K*r./param.R.*cos(Psi)) ...
             - r.*Omega.*(a1s*sPsi + b1s*cPsi) ...
             - v(3)*beta.*cPsi;
        phi = atan2(U_P,U_T);
        alpha = param.theta_0 + param.theta_1 .* r./param.R + phi; 
        cl = param.cl.*sin(alpha).*cos(alpha);
%         c = param.c;
        c = param.ci + r/param.R * (param.co - param.ci);
        cd = param.cd .* sin(alpha).^2;
        U2 = U_T.^2 + U_P.^2;
        dL = U2 .* cl .* c;
        dD = U2 .* cd .* c;
        dT = dL .* cos(phi) + dD .* sin(phi); 
    end
end


