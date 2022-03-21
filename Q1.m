clc
clear

fprintf('Approximation to the integral of sin(t)/t on the interval [0, 4] : %0.15f\n', Gaussian_quadrature_rule)

% helper functions
function val = Gaussian_quadrature_rule
    val = 5/9*f(-sqrt(3/5)) + 8/9*f(0) + 5/9*f(sqrt(3/5));
end
function val = f(x)
    val = sin(2*(1+x)) / (1+x);
end
