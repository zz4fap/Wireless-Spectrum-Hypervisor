clear all;close all;clc

sampling_rate_20MHz = 23.04e6;

downsample_factor = 4;

dir_to_work = ?
filename = ?
FFT_LENGTH = ?

[re,im,signal] = get_signal(dir_to_work, filename, FFT_LENGTH);

fir_coef = fir1(512, (sampling_rate_20MHz/downsample_factor)/sampling_rate_20MHz).';
len_fir_half = (length(fir_coef)-1)/2;
%freqz(fir_coef,1,512)

filtered_signal = conv(signal, fir_coef);
filtered_signal = filtered_signal((len_fir_half+1):(end-len_fir_half));
downsampled_signal = filtered_signal(1:downsample_factor:end);