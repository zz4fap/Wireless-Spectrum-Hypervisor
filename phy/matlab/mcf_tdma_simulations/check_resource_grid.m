clear all;close all;clc

NOF_SLOTS = 2;

NOF_SYMBOLS = 7;

NOF_PRB = 100;

NOF_RE = 12;

resource_grid_size = NOF_SLOTS*NOF_SYMBOLS*NOF_PRB*NOF_RE;

filename = "/home/zz4fap/work/mcf_tdma/scatter/build/resource_grid.dat";
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

for i=1:1:NOF_SLOTS*NOF_SYMBOLS
    figure;
    plot(0:1:NOF_PRB*NOF_RE-1,abs(resource_grid(1+(i-1)*NOF_PRB*NOF_RE:NOF_PRB*NOF_RE*i)))
    titleStr = sprintf('Symbol #%d',i-1);
    title(titleStr)
end