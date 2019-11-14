clear all;close all;clc

M = 64;
bitsPerSym = log2(M);

x = randi([0 1],100000*bitsPerSym,1);

%y = qammod(x,M,'bin','InputType','bit');

y = qammod(x, M, 'bin', 'InputType', 'bit', 'UnitAveragePower', true);


%z = qamdemod(y,M,'bin','OutputType','bit');

z = qamdemod(y,M,'bin','OutputType','bit', 'UnitAveragePower', true);





s = isequal(x,double(z))