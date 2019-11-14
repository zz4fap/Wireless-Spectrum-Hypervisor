clear all;close all;clc

NFFT = 1536;

%% ----------- Modulate with zeros -------------------
signal_zeros = zeros(NFFT,1);
signal_zeros(NFFT/2) = 1;
% Convert signal into single floating point.
signal_single_zeros = single(signal_zeros);

time_signal_single_zeros = ifft(signal_single_zeros,NFFT);

%% ----------- Modulate with Gaussian -------------------
signal_gaussian = 1e-10*(1/sqrt(2))*complex(randn(NFFT,1),randn(NFFT,1)); 

% Convert signal into single floating point.
signal_single_gaussian = single(signal_gaussian);

time_signal_single_gaussian = ifft(signal_single_gaussian,NFFT);

%% ----------------- Comparison -----------------------
% Plot power spectral density (PSD)
hFig1 = figure;
[psd1,f1] = periodogram(time_signal_single_zeros, rectwin(length(time_signal_single_zeros)), NFFT, 1, 'centered');
psd_db = 10*log10(psd1);
plot(f1,psd_db);
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title('Modulated with zeros')

hFig2 = figure;
[psd2,f2] = periodogram(time_signal_single_gaussian, rectwin(length(time_signal_single_gaussian)), NFFT, 1, 'centered');
plot(f2,10*log10(psd2));
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title('Modulated with Gaussian')

