function f=calculateDerivatives(f, order)
%calculateDerivatives calculates the derivatives of the given input functions
% f=calculateDerivatives(f, order) 
%   Input:
%    f        array of length L containing the functions to be differentiated
%    [order]  optional, default=3; order of the highest derivative
%   Output:
%    f       K x L   matrix of symbol functions where f(:,l) contains the up to
%                    the K-1 th derivative of f(1,l)

if nargin < 2
    order=3;
end

f = reshape(f,1,[]);

% calculate all derivatives of specified function and store them into f
% f(i,j) where i = [1..3] (cartesian dimension) and j = [1..4] (derivative + 1)
L = length(f);
for dim = 1:L
    for deriv = 1:order
        f(deriv+1,dim) = diff(f(deriv,dim));
    end
end

end