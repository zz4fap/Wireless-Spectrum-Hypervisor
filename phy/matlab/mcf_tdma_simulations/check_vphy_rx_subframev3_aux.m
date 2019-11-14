clear all;close all;clc

%filename = '/home/zz4fap/work/mcf_tdma/mcf_tdma/build/wrong_decoding_subframe_vphy_id_1_cnt_0_v0.dat';

%filename = '/home/zz4fap/work/mcf_tdma/mcf_tdma/build/vphy_rx_subframe_vphy_id_0_cqi_12_cnt_0.dat';

if(1)

filename = '/home/zz4fap/work/mcf_tdma/mcf_tdma/build/vphy_decoding_0_0.dat';

fileID = fopen(filename);

plot_constellation = false;

phy_bw_idx_to_be_used = 1;

SUBFRAME_LENGTH = [1920 3840 5760 11520 15360 23040];
numFFT = [128 256 384 768 1024 1536];       % Number of FFT points
deltaF = 15000;                             % Subcarrier spacing
numRBs = [6 15 25 50 75 100];               % Number of resource blocks
rbSize = 12;                                % Number of subcarriers per resource block
cpLen_1st_symb  = [10 20 30 60 80 120];     % Cyclic prefix length in samples only for very 1st symbol.
cpLen_other_symbs = [9 18 27 54 72 108];

signal_buffer = complex(zeros(1,SUBFRAME_LENGTH(phy_bw_idx_to_be_used)),zeros(1,SUBFRAME_LENGTH(phy_bw_idx_to_be_used)));

value = fread(fileID, 2, 'float');
idx = 0;
while ~feof(fileID)
    
    idx = idx + 1;
    signal_buffer(idx) = complex(value(1),value(2));
    
    value = fread(fileID, 2, 'float');
end
fclose(fileID);

symbol0 = signal_buffer(cpLen_1st_symb(phy_bw_idx_to_be_used)+1:cpLen_1st_symb(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used));
ofdm_symbol0 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol0,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol0)),'b-')
title('Symbol #0')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol0(29:101)),imag(ofdm_symbol0(29:101)),'b*')
    title('Constellation Symbol #0')
end

symbol1 = signal_buffer(cpLen_1st_symb(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+1:cpLen_1st_symb(phy_bw_idx_to_be_used)+2*numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used));
ofdm_symbol1 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol1,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol1)),'b-')
title('Symbol #1')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol1(29:101)),imag(ofdm_symbol1(29:101)),'b*')
    title('Constellation Symbol #1')
end

sym2_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+1;
sym2_end = cpLen_1st_symb(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used);
symbol2 = signal_buffer(sym2_start:sym2_end);
ofdm_symbol2 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol2,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol2)),'b-')
title('Symbol #2')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol2(29:101)),imag(ofdm_symbol2(29:101)),'b*')
    title('Constellation Symbol #2')
end

sym3_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+3*cpLen_other_symbs(phy_bw_idx_to_be_used)+3*numFFT(phy_bw_idx_to_be_used)+1;
sym3_end = cpLen_1st_symb(phy_bw_idx_to_be_used)+3*cpLen_other_symbs(phy_bw_idx_to_be_used)+4*numFFT(phy_bw_idx_to_be_used);
symbol3 = signal_buffer(sym3_start:sym3_end);
ofdm_symbol3 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol3,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol3)),'b-')
title('Symbol #3')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol3(29:101)),imag(ofdm_symbol3(29:101)),'b*')
    title('Constellation Symbol #3')
end

sym4_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+4*cpLen_other_symbs(phy_bw_idx_to_be_used)+4*numFFT(phy_bw_idx_to_be_used)+1;
sym4_end = cpLen_1st_symb(phy_bw_idx_to_be_used)+4*cpLen_other_symbs(phy_bw_idx_to_be_used)+5*numFFT(phy_bw_idx_to_be_used);
symbol4 = signal_buffer(sym4_start:sym4_end);
ofdm_symbol4 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol4,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol4)),'b-')
title('Symbol #4')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol4(29:101)),imag(ofdm_symbol4(29:101)),'b*')
    title('Constellation Symbol #4')
end

sym5_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+5*cpLen_other_symbs(phy_bw_idx_to_be_used)+5*numFFT(phy_bw_idx_to_be_used)+1;
sym5_end = cpLen_1st_symb(phy_bw_idx_to_be_used)+5*cpLen_other_symbs(phy_bw_idx_to_be_used)+6*numFFT(phy_bw_idx_to_be_used);
symbol5 = signal_buffer(sym5_start:sym5_end);
ofdm_symbol5 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol5,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol5)),'b-')
title('Symbol #5')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol5(29:101)),imag(ofdm_symbol5(29:101)),'b*')
    title('Constellation Symbol #5')
end

sym6_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+6*cpLen_other_symbs(phy_bw_idx_to_be_used)+6*numFFT(phy_bw_idx_to_be_used)+1;
sym6_end =   cpLen_1st_symb(phy_bw_idx_to_be_used)+6*cpLen_other_symbs(phy_bw_idx_to_be_used)+7*numFFT(phy_bw_idx_to_be_used);
symbol6 = signal_buffer(sym6_start:sym6_end);
ofdm_symbol6 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol6,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol6)),'b-')
title('Symbol #6')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol6(29:101)),imag(ofdm_symbol6(29:101)),'b*')
    title('Constellation Symbol #6')
end

% -------------------------------------------------------------------------
% Second Slot
% -------------------------------------------------------------------------

sym7_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+6*cpLen_other_symbs(phy_bw_idx_to_be_used)+7*numFFT(phy_bw_idx_to_be_used)+1;
sym7_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+6*cpLen_other_symbs(phy_bw_idx_to_be_used)+8*numFFT(phy_bw_idx_to_be_used);
symbol7 = signal_buffer(sym7_start:sym7_end);
ofdm_symbol7 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol7,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol7)),'b-')
title('Symbol #7')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol7(29:101)),imag(ofdm_symbol7(29:101)),'b*')
    title('Constellation Symbol #7')
end

sym8_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+7*cpLen_other_symbs(phy_bw_idx_to_be_used)+8*numFFT(phy_bw_idx_to_be_used)+1;
sym8_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+7*cpLen_other_symbs(phy_bw_idx_to_be_used)+9*numFFT(phy_bw_idx_to_be_used);
symbol8 = signal_buffer(sym8_start:sym8_end);
ofdm_symbol8 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol8,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol8)),'b-')
title('Symbol #8')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol8(29:101)),imag(ofdm_symbol8(29:101)),'b*')
    title('Constellation Symbol #8')
end

sym9_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+8*cpLen_other_symbs(phy_bw_idx_to_be_used)+9*numFFT(phy_bw_idx_to_be_used)+1;
sym9_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+8*cpLen_other_symbs(phy_bw_idx_to_be_used)+10*numFFT(phy_bw_idx_to_be_used);
symbol9 = signal_buffer(sym9_start:sym9_end);
ofdm_symbol9 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol9,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol9)),'b-')
title('Symbol #9')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol9(29:101)),imag(ofdm_symbol9(29:101)),'b*')
    title('Constellation Symbol #9')
end

sym10_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+9*cpLen_other_symbs(phy_bw_idx_to_be_used)+10*numFFT(phy_bw_idx_to_be_used)+1;
sym10_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+9*cpLen_other_symbs(phy_bw_idx_to_be_used)+11*numFFT(phy_bw_idx_to_be_used);
symbol10 = signal_buffer(sym10_start:sym10_end);
ofdm_symbol10 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol10,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol10)),'b-')
title('Symbol #10')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol10(29:101)),imag(ofdm_symbol10(29:101)),'b*')
    title('Constellation Symbol #10')
end

sym11_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+10*cpLen_other_symbs(phy_bw_idx_to_be_used)+11*numFFT(phy_bw_idx_to_be_used)+1;
sym11_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+10*cpLen_other_symbs(phy_bw_idx_to_be_used)+12*numFFT(phy_bw_idx_to_be_used);
symbol11 = signal_buffer(sym11_start:sym11_end);
ofdm_symbol11 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol11,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol11)),'b-')
title('Symbol #11')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol11(29:101)),imag(ofdm_symbol11(29:101)),'b*')
    title('Constellation Symbol #11')
end

sym12_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+11*cpLen_other_symbs(phy_bw_idx_to_be_used)+12*numFFT(phy_bw_idx_to_be_used)+1;
sym12_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+11*cpLen_other_symbs(phy_bw_idx_to_be_used)+13*numFFT(phy_bw_idx_to_be_used);
symbol12 = signal_buffer(sym12_start:sym12_end);
ofdm_symbol12 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol12,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol12)),'b-')
title('Symbol #12')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol12(29:101)),imag(ofdm_symbol12(29:101)),'b*')
    title('Constellation Symbol #12')
end

sym13_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+12*cpLen_other_symbs(phy_bw_idx_to_be_used)+13*numFFT(phy_bw_idx_to_be_used)+1;
sym13_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+12*cpLen_other_symbs(phy_bw_idx_to_be_used)+14*numFFT(phy_bw_idx_to_be_used);
symbol13 = signal_buffer(sym13_start:sym13_end);
ofdm_symbol13 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol13,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol13)),'b-')
title('Symbol #13')

if(plot_constellation)
    figure;
    plot(real(ofdm_symbol13(29:101)),imag(ofdm_symbol13(29:101)),'b*')
    title('Constellation Symbol #13')
end

end




if(1)


filename = '/home/zz4fap/work/mcf_tdma/mcf_tdma/build/vphy_decoding_0_0_correct.dat';

fileID = fopen(filename);

plot_constellation = false;

phy_bw_idx_to_be_used = 1;

SUBFRAME_LENGTH = [1920 3840 5760 11520 15360 23040];
numFFT = [128 256 384 768 1024 1536];       % Number of FFT points
deltaF = 15000;                             % Subcarrier spacing
numRBs = [6 15 25 50 75 100];               % Number of resource blocks
rbSize = 12;                                % Number of subcarriers per resource block
cpLen_1st_symb  = [10 20 30 60 80 120];     % Cyclic prefix length in samples only for very 1st symbol.
cpLen_other_symbs = [9 18 27 54 72 108];

signal_buffer = complex(zeros(1,SUBFRAME_LENGTH(phy_bw_idx_to_be_used)),zeros(1,SUBFRAME_LENGTH(phy_bw_idx_to_be_used)));

value = fread(fileID, 2, 'float');
idx = 0;
while ~feof(fileID)
    
    idx = idx + 1;
    signal_buffer(idx) = complex(value(1),value(2));
    
    value = fread(fileID, 2, 'float');
end
fclose(fileID);

symbol0 = signal_buffer(cpLen_1st_symb(phy_bw_idx_to_be_used)+1:cpLen_1st_symb(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used));
correct_ofdm_symbol0 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol0,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol0)),'b-')
title('Correct Symbol #0')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol0(29:101)),imag(correct_ofdm_symbol0(29:101)),'b*')
    title('Constellation Symbol #0')
end

symbol1 = signal_buffer(cpLen_1st_symb(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+1:cpLen_1st_symb(phy_bw_idx_to_be_used)+2*numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used));
correct_ofdm_symbol1 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol1,numFFT(phy_bw_idx_to_be_used))).';

% figure;
% plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(ofdm_symbol1)),'b-')
% title('Symbol #1')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol1(29:101)),imag(correct_ofdm_symbol1(29:101)),'b*')
    title('Constellation Symbol #1')
end

sym2_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+1;
sym2_end = cpLen_1st_symb(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used)+cpLen_other_symbs(phy_bw_idx_to_be_used)+numFFT(phy_bw_idx_to_be_used);
symbol2 = signal_buffer(sym2_start:sym2_end);
correct_ofdm_symbol2 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol2,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol2)),'b-')
title('Correct Symbol #2')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol2(29:101)),imag(correct_ofdm_symbol2(29:101)),'b*')
    title('Constellation Symbol #2')
end

sym3_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+3*cpLen_other_symbs(phy_bw_idx_to_be_used)+3*numFFT(phy_bw_idx_to_be_used)+1;
sym3_end = cpLen_1st_symb(phy_bw_idx_to_be_used)+3*cpLen_other_symbs(phy_bw_idx_to_be_used)+4*numFFT(phy_bw_idx_to_be_used);
symbol3 = signal_buffer(sym3_start:sym3_end);
correct_ofdm_symbol3 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol3,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol3)),'b-')
title('Symbol #3')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol3(29:101)),imag(correct_ofdm_symbol3(29:101)),'b*')
    title('Constellation Symbol #3')
end

sym4_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+4*cpLen_other_symbs(phy_bw_idx_to_be_used)+4*numFFT(phy_bw_idx_to_be_used)+1;
sym4_end = cpLen_1st_symb(phy_bw_idx_to_be_used)+4*cpLen_other_symbs(phy_bw_idx_to_be_used)+5*numFFT(phy_bw_idx_to_be_used);
symbol4 = signal_buffer(sym4_start:sym4_end);
correct_ofdm_symbol4 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol4,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol4)),'b-')
title('Symbol #4')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol4(29:101)),imag(correct_ofdm_symbol4(29:101)),'b*')
    title('Constellation Symbol #4')
end

sym5_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+5*cpLen_other_symbs(phy_bw_idx_to_be_used)+5*numFFT(phy_bw_idx_to_be_used)+1;
sym5_end = cpLen_1st_symb(phy_bw_idx_to_be_used)+5*cpLen_other_symbs(phy_bw_idx_to_be_used)+6*numFFT(phy_bw_idx_to_be_used);
symbol5 = signal_buffer(sym5_start:sym5_end);
correct_ofdm_symbol5 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol5,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol5)),'b-')
title('Symbol #5')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol5(29:101)),imag(correct_ofdm_symbol5(29:101)),'b*')
    title('Constellation Symbol #5')
end

sym6_start = cpLen_1st_symb(phy_bw_idx_to_be_used)+6*cpLen_other_symbs(phy_bw_idx_to_be_used)+6*numFFT(phy_bw_idx_to_be_used)+1;
sym6_end =   cpLen_1st_symb(phy_bw_idx_to_be_used)+6*cpLen_other_symbs(phy_bw_idx_to_be_used)+7*numFFT(phy_bw_idx_to_be_used);
symbol6 = signal_buffer(sym6_start:sym6_end);
correct_ofdm_symbol6 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol6,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol6)),'b-')
title('Symbol #6')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol6(29:101)),imag(correct_ofdm_symbol6(29:101)),'b*')
    title('Constellation Symbol #6')
end

% -------------------------------------------------------------------------
% Second Slot
% -------------------------------------------------------------------------

sym7_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+6*cpLen_other_symbs(phy_bw_idx_to_be_used)+7*numFFT(phy_bw_idx_to_be_used)+1;
sym7_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+6*cpLen_other_symbs(phy_bw_idx_to_be_used)+8*numFFT(phy_bw_idx_to_be_used);
symbol7 = signal_buffer(sym7_start:sym7_end);
correct_ofdm_symbol7 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol7,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol7)),'b-')
title('Symbol #7')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol7(29:101)),imag(correct_ofdm_symbol7(29:101)),'b*')
    title('Constellation Symbol #7')
end

sym8_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+7*cpLen_other_symbs(phy_bw_idx_to_be_used)+8*numFFT(phy_bw_idx_to_be_used)+1;
sym8_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+7*cpLen_other_symbs(phy_bw_idx_to_be_used)+9*numFFT(phy_bw_idx_to_be_used);
symbol8 = signal_buffer(sym8_start:sym8_end);
correct_ofdm_symbol8 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol8,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol8)),'b-')
title('Symbol #8')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol8(29:101)),imag(correct_ofdm_symbol8(29:101)),'b*')
    title('Constellation Symbol #8')
end

sym9_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+8*cpLen_other_symbs(phy_bw_idx_to_be_used)+9*numFFT(phy_bw_idx_to_be_used)+1;
sym9_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+8*cpLen_other_symbs(phy_bw_idx_to_be_used)+10*numFFT(phy_bw_idx_to_be_used);
symbol9 = signal_buffer(sym9_start:sym9_end);
correct_ofdm_symbol9 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol9,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol9)),'b-')
title('Symbol #9')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol9(29:101)),imag(correct_ofdm_symbol9(29:101)),'b*')
    title('Constellation Symbol #9')
end

sym10_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+9*cpLen_other_symbs(phy_bw_idx_to_be_used)+10*numFFT(phy_bw_idx_to_be_used)+1;
sym10_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+9*cpLen_other_symbs(phy_bw_idx_to_be_used)+11*numFFT(phy_bw_idx_to_be_used);
symbol10 = signal_buffer(sym10_start:sym10_end);
correct_ofdm_symbol10 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol10,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol10)),'b-')
title('Symbol #10')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol10(29:101)),imag(correct_ofdm_symbol10(29:101)),'b*')
    title('Constellation Symbol #10')
end

sym11_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+10*cpLen_other_symbs(phy_bw_idx_to_be_used)+11*numFFT(phy_bw_idx_to_be_used)+1;
sym11_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+10*cpLen_other_symbs(phy_bw_idx_to_be_used)+12*numFFT(phy_bw_idx_to_be_used);
symbol11 = signal_buffer(sym11_start:sym11_end);
correct_ofdm_symbol11 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol11,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol11)),'b-')
title('Symbol #11')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol11(29:101)),imag(correct_ofdm_symbol11(29:101)),'b*')
    title('Constellation Symbol #11')
end

sym12_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+11*cpLen_other_symbs(phy_bw_idx_to_be_used)+12*numFFT(phy_bw_idx_to_be_used)+1;
sym12_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+11*cpLen_other_symbs(phy_bw_idx_to_be_used)+13*numFFT(phy_bw_idx_to_be_used);
symbol12 = signal_buffer(sym12_start:sym12_end);
correct_ofdm_symbol12 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol12,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol12)),'b-')
title('Symbol #12')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol12(29:101)),imag(correct_ofdm_symbol12(29:101)),'b*')
    title('Constellation Symbol #12')
end

sym13_start = 2*cpLen_1st_symb(phy_bw_idx_to_be_used)+12*cpLen_other_symbs(phy_bw_idx_to_be_used)+13*numFFT(phy_bw_idx_to_be_used)+1;
sym13_end =   2*cpLen_1st_symb(phy_bw_idx_to_be_used)+12*cpLen_other_symbs(phy_bw_idx_to_be_used)+14*numFFT(phy_bw_idx_to_be_used);
symbol13 = signal_buffer(sym13_start:sym13_end);
correct_ofdm_symbol13 = (1/sqrt(numFFT(phy_bw_idx_to_be_used)))*fftshift(fft(symbol13,numFFT(phy_bw_idx_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_idx_to_be_used)-1,10*log10(abs(correct_ofdm_symbol13)),'b-')
title('Symbol #13')

if(plot_constellation)
    figure;
    plot(real(correct_ofdm_symbol13(29:101)),imag(correct_ofdm_symbol13(29:101)),'b*')
    title('Constellation Symbol #13')
end

end