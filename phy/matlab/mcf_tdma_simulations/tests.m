clear all;close all;clc

num_sc_6RB = 72;

Ncp = 144;

k = 164:235;
k = k.';

N = 1536;

X1 = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

n = N-Ncp:N-1;
x_N_minus_1 = sum(X1.*exp((1i*2*pi*k*n)/N));

n = -Ncp:-1;
x_minus_1 = sum(X1.*exp((1i*2*pi*k*n)/N));


error = sum(abs(x_N_minus_1-x_minus_1))/Ncp;

