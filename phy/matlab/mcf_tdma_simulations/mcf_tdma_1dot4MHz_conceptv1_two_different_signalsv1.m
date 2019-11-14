% test frequency domain multiplexing concept of McF-TDMA
% 14M == 1.4M for naming purpose
close all; clear all;clc;

%
N = 99;
[LPAnalysis, HPAnalysis, LPSynthsis, HPSynthesis] = firpr2chfb(N, 0.45);

fvt = fvtool(LPAnalysis,1, HPAnalysis,1, LPSynthsis,1, HPSynthesis,1);
fvt.Color = [1,1,1];
legend(fvt,'Hlp Lowpass Decimator','Hhp Highpass Decimator',...
    'Glp Lowpass Interpolator','Ghp Highpass Interpolator');

% -----------------------basic definitions--------------------------
sc_spacing = 15e3;
freq_offset_1dot4MHz_in_20MHz = 6e6;

num_sc_per_RB = 12;
num_sc_6RB = 6*num_sc_per_RB;
num_sc_100RB = 100*num_sc_per_RB;

nfft_20MHz_standard = 2048;
len_cp_20MHz_standard = 144;
sampling_rate_20MHz_standard = 30.72e6;

nfft_20MHz = 1536;
len_cp_20MHz = len_cp_20MHz_standard*nfft_20MHz/nfft_20MHz_standard;
sampling_rate_20MHz = sampling_rate_20MHz_standard*nfft_20MHz/nfft_20MHz_standard;

fir_coef = fir1(512, 1.4e6/sampling_rate_20MHz).'; % baseband fir of 1.4M rx
len_fir_half = (length(fir_coef)-1)/2;
freqz(fir_coef,1,512)

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;

% ---Signal #1: fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
signal1_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% ---Signal #2: fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
signal2_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% -------------- Signal #1: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
% NOTE: Modulation symbols (data/pilot/pss/sss/etc.) are mapped to the subcarriers around DC, including it.
freq_sc_1dot4MHz = [zeros((nfft_1dot4MHz-num_sc_6RB)/2,1); signal1_1dot4MHz_6RB; zeros((nfft_1dot4MHz-num_sc_6RB)/2,1)];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol1_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #2: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
% NOTE: Modulation symbols (data/pilot/pss/sss/etc.) are mapped to the subcarriers around DC, including it.
freq_sc_1dot4MHz = [zeros((nfft_1dot4MHz-num_sc_6RB)/2,1); signal2_1dot4MHz_6RB; zeros((nfft_1dot4MHz-num_sc_6RB)/2,1)];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol2_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% ------------------------------------------------------------------------
% Generate two 1.4 MHz LTE signal (at +6MHz and -6MHz) via frequency domain
% Multiplexing through single 20MHz LTE transmitter

sc_idx_offset_6MHz = freq_offset_1dot4MHz_in_20MHz/sc_spacing;
% subcarrier idx of negative 6MHz offset 1.4MHz LTE in 20MHz LTE
sc_sp_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
sc_ep_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz+(num_sc_6RB/2);
% subcarrier idx of positive 6MHz offset 1.4MHz LTE in 20MHz LTE
sc_sp_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
sc_ep_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz+(num_sc_6RB/2);

s_20MHz_100RB = zeros(num_sc_100RB,1);
% Frequency domain multiplexing of two 1.4MHz LTE into subcarrier of 20MHz LTE
s_20MHz_100RB(sc_sp_n6MHz:sc_ep_n6MHz) = signal1_1dot4MHz_6RB;
s_20MHz_100RB(sc_sp_p6MHz:sc_ep_p6MHz) = signal2_1dot4MHz_6RB;

figure; 
plot(abs(s_20MHz_100RB)); 
title('Multiplexing two 6 RB into 100 RB for 20 MHz Tx'); 
grid on;

% --------------20MHz LTE Tx: 1 CP + 1 OFDM symbol----------------------
freq_sc_20MHz = [s_20MHz_100RB(((num_sc_100RB/2)+1):end); zeros(nfft_20MHz-num_sc_100RB,1); s_20MHz_100RB(1:(num_sc_100RB/2))];
ofdm_symbol_20MHz = ifft(freq_sc_20MHz,nfft_20MHz).*(nfft_20MHz/nfft_1dot4MHz);
ofdm_symbol_with_cp_20MHz = [ofdm_symbol_20MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_20MHz];

% --------Two independent 1.4 MHz receiver at +6MHz and -6MHz------------
t_idx = (-len_cp_20MHz:(length(ofdm_symbol_with_cp_20MHz)-len_cp_20MHz-1)).';

% Receiving 1.4 MHz LTE at -6 MHz offset:
rx_n6MHz = ofdm_symbol_with_cp_20MHz.*exp(+1i.*2.*pi.*freq_offset_1dot4MHz_in_20MHz.*t_idx./sampling_rate_20MHz);
rx_n6MHz = conv(rx_n6MHz, fir_coef);
rx_n6MHz = rx_n6MHz((len_fir_half+1):(end-len_fir_half));
rx_n6MHz = rx_n6MHz(1:(nfft_20MHz/nfft_1dot4MHz):end);

% Receiving 1.4 MHz LTE at +6 MHz offset:
rx_p6MHz = ofdm_symbol_with_cp_20MHz.*exp(-1i.*2.*pi.*freq_offset_1dot4MHz_in_20MHz.*t_idx./sampling_rate_20MHz);
rx_p6MHz = conv(rx_p6MHz, fir_coef);
rx_p6MHz = rx_p6MHz((len_fir_half+1):(end-len_fir_half));
rx_p6MHz = rx_p6MHz(1:(nfft_20MHz/nfft_1dot4MHz):end);

%% ------------------------ Plotting figures ------------------------------
figure
imag_ofdm_symbol1_with_cp_1dot4MHz = imag(ofdm_symbol1_with_cp_1dot4MHz);
imag_rx_n6MHz = imag(rx_n6MHz);
imag_rx_p6MHz = imag(rx_p6MHz);
plot(imag_ofdm_symbol1_with_cp_1dot4MHz,'k')
grid on
hold on
plot(imag_rx_n6MHz,'r.-')
plot(imag_rx_p6MHz,'bs-')
legend('Signal #1 - 1.4 MHz Tx by 1.92 Msps LTE Tx', '1.4 MHz Rx #0 from 20 MHz LTE Tx', '1.4 MHz Rx #1 from 20 MHz LTE Tx')
title('Time domain imaginary part of 1.4MHz LTE signal #1')
hold off

figure
imag_ofdm_symbol2_with_cp_1dot4MHz = imag(ofdm_symbol2_with_cp_1dot4MHz);
imag_rx_n6MHz = imag(rx_n6MHz);
imag_rx_p6MHz = imag(rx_p6MHz);
plot(imag_ofdm_symbol2_with_cp_1dot4MHz,'k')
grid on
hold on
plot(imag_rx_n6MHz,'r.-')
plot(imag_rx_p6MHz,'bs-')
legend('Signal #2 - 1.4 MHz Tx by 1.92 Msps LTE Tx', '1.4 MHz Rx #0 from 20 MHz LTE Tx', '1.4 MHz Rx #1 from 20 MHz LTE Tx')
title('Time domain imaginary part of 1.4MHz LTE signal #1')
hold off

figure
real_ofdm_symbol1_with_cp_1dot4MHz = real(ofdm_symbol1_with_cp_1dot4MHz);
real_rx_n6MHz = real(rx_n6MHz);
real_rx_p6MHz = real(rx_p6MHz);
plot(real_ofdm_symbol1_with_cp_1dot4MHz,'k')
grid on
hold on
plot(real_rx_n6MHz,'r.-')
plot(real_rx_p6MHz,'bs-')
legend('1.4 MHz Tx by 1.92 Msps LTE Tx', '1.4 MHz Rx #0 from 20 MHz LTE Tx', '1.4 MHz Rx #1 from 20 MHz LTE Tx')
title('Time domain real part of 1.4MHz LTE signal')
hold off

figure
real_ofdm_symbol2_with_cp_1dot4MHz = real(ofdm_symbol2_with_cp_1dot4MHz);
real_rx_n6MHz = real(rx_n6MHz);
real_rx_p6MHz = real(rx_p6MHz);
plot(real_ofdm_symbol2_with_cp_1dot4MHz,'k')
grid on
hold on
plot(real_rx_n6MHz,'r.-')
plot(real_rx_p6MHz,'bs-')
legend('1.4 MHz Tx by 1.92 Msps LTE Tx', '1.4 MHz Rx #0 from 20 MHz LTE Tx', '1.4 MHz Rx #1 from 20 MHz LTE Tx')
title('Time domain real part of 1.4MHz LTE signal')
hold off
