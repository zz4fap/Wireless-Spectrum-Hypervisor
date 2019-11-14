clear all;close all;clc

if(1)
    fs = 30.72e6;
    %fs = 23.04e6;
    
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
    
    filename = '/home/zz4fap/work/mcftdma-NO-windowing-tx-gain-25dB-23dot04MHz-type-short-rx-gain-0dB-dur-0dot5s-scenario-8981-v0.dat';
    %filename = '/home/zz4fap/work/mcftdma_windowing_23dot04_MHz_type_short_gain_0_dur_0dot5_v0.dat';
    %filename = '/home/zz4fap/Downloads/mcf-tdma-tx-gain-25dB-scenario-8981-short-rx-gain-0dB-test-flag-W-23dot04MHz-25072018.dat';
    %filename = '/home/zz4fap/Downloads/mcf-tdma-tx-gain-25dB-scenario-8981-short-rx-gain-0dB-test-flag-WWW-30dot72MHzv3.dat';
    %filename = '/home/zz4fap/Downloads/mcf-tdma-tx-gain-25dB-scenario-8981-short-rx-gain-0dB-test-flag-WWW-23dot04MHz.dat';
    %filename = '/home/zz4fap/Downloads/mcf-tdma-tx-gain-25dB-scenario-8981-short-rx-gain-0dB-test-flag-W-30dot72MHz.dat';
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-null-subcarriers-v7.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-null-subcarriers-v6.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-null-subcarriers-v5.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-null-subcarriers-v4.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-v1.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-3dB-v2.dat";
    
    fileID = fopen(filename);
    
    fseek(fileID,offset,'bof');
    
    signal_buffer = complex(zeros(1,buffer_size),zeros(1,buffer_size));
    idx = 1;
    value = fread(fileID, 2, 'short');
    for sample_idx=1:1:buffer_size
        signal_buffer(idx) = complex(value(1),value(2))/SHORT_MAX_SIZE;
        value = fread(fileID, 2, 'short');
        idx = idx + 1;
    end
    fclose(fileID);
    
    idxs = strfind(filename,'/');
    filenamechar = char(filename);
    strTitle = filenamechar(idxs(length(idxs))+1:end-4);
    h2 = figure('rend','painters','pos',[10 10 1000 900]);
    overlap = 512;
    spectrogram(signal_buffer,kaiser(FFT_LENGTH,5), overlap, FFT_LENGTH, fs ,'centered');
    title(strTitle)
    figureName = strcat(strTitle,'.png');
    %saveas(h2,figureName)
end

%% ******** Look for valid signal start **********
if(0)
    
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-v1.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-3dB-v2.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-null-subcarriers-v4.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-null-subcarriers-v5.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-null-subcarriers-v6.dat";
    %filename = "/home/zz4fap/Downloads/mcf-tdma-tx-gain-31dot5-scenario-8981-short-rx-gain-0dB-null-subcarriers-v7.dat";
    
    fileID = fopen(filename);
    
    value = fread(fileID, 2, 'short');
    cnt = 0;
    while ~feof(fileID)
        
        if(abs(value(1)) > 700)
            fprintf(1,'abs(value) > 100: pos: %d - value: %d\n',cnt, value(1));
        end
        cnt = cnt + 1;
        
        value = fread(fileID, 2, 'short');
    end
    fclose(fileID);
    
end