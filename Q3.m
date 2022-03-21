clc
clear

X = 1.2:0.2:2;

for h = [0.1, 0.01]
    iters = 1;
    i = 1;
    if h == 0.1
        P = 2:2:10;
        fprintf('\t\t\t\t\t\t Table 1: With h = 0.1\n');
    else
        P = 20:20:100;
        fprintf('\n\t\t\t\t\t\t Table 2: With h = 0.01\n');
    end
    fprintf(' x \t\t\t EM \t\t Error(EM) \t\t\t RK2 \t\t Error(RK2) \t\t RK4 \t\t Error(RK4)\n');
    x_prev = 1;
    y1_prev = 2;
    y2_prev = 2;
    y3_prev = 2;

    while x_prev < 2
        x = x_prev + h;
        % EM
        y1 = y1_prev + h * f(x_prev, y1_prev);
    
        % RK2
        k1 = h * f(x_prev, y2_prev);
        k2 = h * f(x_prev + h, y2_prev + k1);
        y2 = y2_prev + (k1 + k2) / 2;
    
        % RK4
        k1 = h * f(x_prev, y3_prev);
        k2 = h * f(x_prev + h/2, y3_prev + k1/2);
        k3 = h * f(x_prev + h/2, y3_prev + k2/2);
        k4 = h * f(x_prev + h, y3_prev + k3);
        y3 = y3_prev + (k1 + 2*k2 + 2*k3 + k4) / 6;
        
        y = sol(x);
        if iters == P(i)
            fprintf('%0.1f \t %0.8f \t %0.8f \t %0.8f \t %0.8f \t %0.8f \t %0.8f\n', x, y1, abs(y1-y), y2, abs(y2-y), y3, abs(y3-y));
            i = i + 1;
        end
    
        x_prev = x;
        y_prev1 = y1;
        y_prev2 = y2;
        y_prev3 = y3;
        iters = iters + 1;
    end
end

% helper functions
function val = f(x, y) 
    val = (x*y - y^2) / (x^2);
end
function val = sol(x) 
    val = x / (0.5 + log(x));
end