clear all;close all;clc


M = 4;

m = 0:1:M-1;

N = 16;

d = zeros(1, N);
for n = 0:1:N-1
    
    f = exp((-1i.*2.*pi.*n.*m)./M);
    
    d(n+1) = (1/M).*sum(f);
    
end

stem(0:1:N-1, real(d))
