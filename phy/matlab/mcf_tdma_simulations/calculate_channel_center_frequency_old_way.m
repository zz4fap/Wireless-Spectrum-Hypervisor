clear all;close all;clc

PHY_BW = 15*12*15000;

center_frequency = 2.4e9;

competiton_bw = 20e6;

for channel=0:1:10

    channel_center_freq = center_frequency - (competiton_bw/2) + (PHY_BW/2) + channel*PHY_BW;
    
    fprintf(1,'channel_center_freq: %f\n',channel_center_freq);
    
end