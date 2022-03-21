clc
clear

x1 = 0.5857864376;
x2 = 3.414213562;
W1 = 0.8535533903;
W2 = 0.1464466092;

% helper functions
I = W1 * f(x1) + W2 * f(x2);
fprintf('Approximate value of the integral for f(x) = x^3 : %0.15f\n', I);
function val = f(x)
    val = x^3;
end