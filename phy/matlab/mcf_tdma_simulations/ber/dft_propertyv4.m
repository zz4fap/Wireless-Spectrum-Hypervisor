clear all;close all;clc

N = 1536;

M = 12;

X = randn(1, N);

x = zeros(1, N/M);
for n=0:1:(N/M)-1
    
    nl = n*M;

    for k=0:1:N-1

        f = exp((1i.*2.*pi.*k.*nl)/N);
        
        aux = X(k+1).*f;

        x(n+1) = x(n+1) + ((1/N).*aux);
    
    end

end

figure;
stem(0:1:(N/M)-1, real(x))




Nl = N/M;

xl = zeros(1, Nl);
for n=0:1:Nl-1
    

    for k=0:1:Nl-1

        f = exp((1i.*2.*pi.*k.*n)/(Nl));
        
        aux_var = 0;
        for m=0:1:M-1
            aux_var = aux_var + X((k + m.*(N/M)) + 1);
        end
        
        aux = aux_var.*f;

        xl(n+1) = xl(n+1) + ((1/N).*aux);
    
    end

end

figure;
stem(0:1:Nl-1, real(xl))


error = sum(abs(x-xl).^2)/length(x);

a=1;