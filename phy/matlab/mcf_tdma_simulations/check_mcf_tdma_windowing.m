clear all;close all;clc

fs = 23.04e6;

SHORT_MAX_SIZE = 32768;

FFT_LENGTH = 2048;%1536;

buffer_size = 1000000;

% Offset for 3dB gain v2.
%offset = (89827703+7.5e5)*4; % For short we read 2 * 2 bytes each time;

% Offset for 0dB gain v1.
%offset = (98778975)*4;

%Offset for 0 dB gain and v6.
%offset = 34000000*4; %1*(40000*4); %45729*4; %35904222

offset = 225395*4;


%% ---------------------------- No Windowing ------------------------------
filename = '/home/zz4fap/work/mcftdma-NO-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v0.dat';

fileID = fopen(filename);

fseek(fileID,offset,'bof');

signal_buffer_no_windowing = complex(zeros(1,buffer_size),zeros(1,buffer_size));
idx = 1;
value = fread(fileID, 2, 'short');
for sample_idx=1:1:buffer_size
    signal_buffer_no_windowing(idx) = complex(value(1),value(2))/SHORT_MAX_SIZE;
    value = fread(fileID, 2, 'short');
    idx = idx + 1;
end
fclose(fileID);

%% ---------------------------- Windowing ------------------------------
filename = '/home/zz4fap/work/mcftdma-hanning-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v0.dat';
%filename = '/home/zz4fap/work/mcftdma-blackmanharris7-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v0.dat';
%filename = '/home/zz4fap/work/mcftdma-blackmanharris-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v0.dat';
%filename = '/home/zz4fap/work/mcftdma-flattop-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v0.dat';
%filename = '/home/zz4fap/work/mcftdma-hamming-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v0.dat';
%filename = '/home/zz4fap/work/mcftdma-kaiser-beta-2dot5-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v0.dat';
%filename = '/home/zz4fap/work/mcftdma-liquid-rcostaper-windowf-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v1.dat';

fileID = fopen(filename);

fseek(fileID,offset,'bof');

signal_buffer_windowing = complex(zeros(1,buffer_size),zeros(1,buffer_size));
idx = 1;
value = fread(fileID, 2, 'short');
for sample_idx=1:1:buffer_size
    signal_buffer_windowing(idx) = complex(value(1),value(2))/SHORT_MAX_SIZE;
    value = fread(fileID, 2, 'short');
    idx = idx + 1;
end
fclose(fileID);

overlap = 512;
h2 = figure('rend','painters','pos',[10 10 1000 900]);
subplot(1,2,1)
spectrogram(signal_buffer_no_windowing,kaiser(FFT_LENGTH,5), overlap, FFT_LENGTH, fs ,'centered');
title('No windowing')

subplot(1,2,2)
spectrogram(signal_buffer_windowing,kaiser(FFT_LENGTH,5), overlap, FFT_LENGTH, fs ,'centered');
title('Hanning windowing')

saveas(h2,'comparison_hanning.png')
