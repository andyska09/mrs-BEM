function param=setupKingfisher()
%setupKingfisher returns a struct containing all the parameters for the quad

param = [];
param.mass = 0.752;
param.inertia = [0.00262, 0.00214, 0.0043];
param.J = diag(param.inertia);
param.armLength = 0.13;

l = param.armLength / sqrt(2);
t_BM = [ l -l -l l ;
        -l  l -l l];
rel_cd = 0.022; % relative drag coefficient drag torque = thrust * rel_cd
mot_dir = [-1 -1 1 1];

% compute control allocation matrix
param.CA = [ones(1,4);
            t_BM(2,:);
            -t_BM(1,:);
            rel_cd*mot_dir];

param.thrust_max = 10; % N per motor

end