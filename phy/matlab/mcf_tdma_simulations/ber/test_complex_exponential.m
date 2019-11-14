clear all;close all;clc

N = 128;

M = 16;

n = 1;

f = zeros(1, N);
for k = 0:1:N-1

    f(k+1) = exp((-1i.*2.*pi.*n.*k)./(N./M));

end

plot(round(real(f)), round(imag(f)), '*')

N = 128;
for k = 0:1:N-1
    
    fprintf(1, '%d\n', mod(-(N/2)+  1 + k, 1536));
    
end