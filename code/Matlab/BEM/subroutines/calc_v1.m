function v1=calc_v1(v, Omega)
%calc_v1 calculates the induced velocity
% v1=calc_v1(v, Omega) where v is the free-stream velocity in body frame and
% Omega is the angular speed of the propeller. v1 is the induced velocity.
% To calculate the induced velocity, the approach presented by W. Prouty is used
% to some extend. Given the induced velocity, one can calculate the thrust using
% blade-element theory. Momentum theory however dictates a certain induced
% velocity, given a thrust. This function numerically finds the induced velocity
% such that both blade-element theory and momentum theory are satisfied.
    param = setParam();
    vel = norm(v);
    K = param.K;
    mu = calc_mu(v, Omega);
    vi_init = 8;
    
    fun = @(vi) vi - sqrt(-vel.^2/2 + sqrt((vel.^4)/4 ...
        + calc_v_hover(calc_T_numInt(v,K,mu,Omega,vi,0,0,0)).^4));
    v1=fzero(fun,vi_init,param.opt_fzero);
    
    if (v(3)/v1 >= 0)
        k0=1;
        k1=-1.125;
        k2=-1.372;
        k3=-1.718;
        k4=-0.655;
        vz = -v(3);
        v(3) = 0;
        vel = norm(v(1:2));
        fun = @(vi) vi - sqrt(-vel.^2/2 + sqrt((vel.^4)/4 ...
                   + calc_v_hover(calc_T_numInt(v,K,mu,Omega,vi,0,0,0)).^4));
        vh=fzero(fun,vi_init,param.opt_fzero);
        v2 = vh*(k0 + k1*(vz/vh) + k2*(vz/vh)^2 + k3*(vz/vh)^3 + k4*(vz/vh)^4);
        v1 = max(v1,v2);
    end
end


