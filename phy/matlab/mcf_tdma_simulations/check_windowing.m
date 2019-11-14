clear all;close all;clc

L = 72;

nof_windows = 7;

w = zeros(nof_windows,L);

w(1,:) = hamming(L);
w(2,:) = hann(L);
w(3,:) = kaiser(L,10);
w(4,:) = blackman(L);
w(5,:) = bartlett(L);
w(6,:) = chebwin(L);
w(7,:) = gausswin(L);

figure;
plot(w(1,:));
hold on
plot(w(2,:));
plot(w(3,:));
plot(w(4,:));
plot(w(5,:));
plot(w(6,:));
plot(w(7,:));
hold off
grid on
legend('Hamming','Hanning','Kaiser','Blackman','Bartlett','Chebwin','Gaussian')

figure;
plot(w(2,:));
hold on
plot(w(3,:));
plot(w(4,:));
plot(w(6,:));
hold off
grid on
legend('Hanning','Kaiser','Blackman','Chebwin')
