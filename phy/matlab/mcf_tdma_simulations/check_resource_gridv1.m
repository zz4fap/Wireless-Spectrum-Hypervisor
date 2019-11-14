clear all;close all;clc

NOF_SLOTS = 2;

NOF_SYMBOLS = 7;

RADIO_NOF_PRB = 100;

RADIO_FFT_LEN = 1536;

VPHY_NOF_PRB = 6;

VPHY_FFT_LEN = 128;

NOF_RE = 12;

resource_grid_size = NOF_SLOTS*NOF_SYMBOLS*RADIO_FFT_LEN;

filename = "/home/zz4fap/work/mcf_tdma/mcf_tdma/build/resource_grid_9.dat";
fileID = fopen(filename);
resource_grid = complex(zeros(1,resource_grid_size),zeros(1,resource_grid_size));
idx = 1;
value = fread(fileID, 2, 'float');
while ~feof(fileID)
    resource_grid(idx) = complex(value(1),value(2));
    value = fread(fileID, 2, 'float');
    idx = idx + 1;
end
fclose(fileID);

symbols = zeros(NOF_SLOTS*NOF_SYMBOLS,RADIO_FFT_LEN);
for i=1:1:NOF_SLOTS*NOF_SYMBOLS
    figure;
    symbols(i,:) = resource_grid(1+(i-1)*RADIO_FFT_LEN:RADIO_FFT_LEN*i);
    plot(0:1:RADIO_FFT_LEN-1,abs(resource_grid(1+(i-1)*RADIO_FFT_LEN:RADIO_FFT_LEN*i)))
    ylim([0 1])
    titleStr = sprintf('Symbol #%d',i-1);
    title(titleStr)
end

