clear all;close all;clc

center_frequency = 2e9;

BW = 23.04e6;

channel_bw = 1.92e6;

delta_f = 15e3;

% for ch_idx=0:1:11
%     
%     channel_center_freq = center_frequency - (BW/2) + channel_bw*ch_idx;
%     
%     fprintf(1,'ch_idx: %d - channel_center_freq: %1.2f\n',ch_idx,channel_center_freq/1e6);
%     
% end
% 
% fprintf(1,'\n\n');
% 
% for fft_bin=0:1:1536-1
%     
%     fprintf(1,'fft_bin: %d - channel_center_freq: %1.4f\n',fft_bin,((fft_bin*delta_f)+(center_frequency - (BW/2)))/1e6);
%     
% end

for fft_bin=0:1:1536-1
    fprintf(1,'fft_bin: %d - %d\n',fft_bin,mod(fft_bin,128));
end