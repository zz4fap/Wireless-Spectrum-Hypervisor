clear all;close all;clc

delta_f = 15000;

RADIO_FFT_LEN = 1536;

VPHY_FFT_LEN = 128;

center_frequency = 1e9;

for channel=0:1:(RADIO_FFT_LEN/VPHY_FFT_LEN)

    channel_center_freq = center_frequency - ((RADIO_FFT_LEN*delta_f)/2) + channel*(VPHY_FFT_LEN*delta_f);
    
    fprintf(1,'channel_center_freq: %f\n',channel_center_freq);
    
end