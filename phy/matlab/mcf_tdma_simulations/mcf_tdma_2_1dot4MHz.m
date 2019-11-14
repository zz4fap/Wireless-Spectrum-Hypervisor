close all; clear all;clc;

rng(1)

addpath('../sync')

% ----------------------- Definitions --------------------------
% Generate PSS signal.
cell_id = 0;
pss = lte_pss_zc(cell_id);
pss = pss.';

sc_spacing = 15e3;
freq_offset_1dot4MHz_in_3MHz = 128*15000; % offset in Hz: 1.92 MHz

len_cp_20MHz_standard = 144;
nfft_20MHz_standard = 1536;
sampling_rate_20MHz_standard = nfft_20MHz_standard*15e3;

num_sc_per_RB = 12;
num_sc_6RB = 6*num_sc_per_RB;
num_sc_15RB = 15*num_sc_per_RB;
num_sc_100RB = 100*num_sc_per_RB;

nfft_3MHz_standard = 256;
len_cp_3MHz_standard = 18; % Fisrt OFDM symbol 160, subsequently ones, 144.
sampling_rate_3MHz_standard = 3.84e6;

nfft_3MHz = 256;
len_cp_3MHz = len_cp_3MHz_standard*nfft_3MHz/nfft_3MHz_standard;
sampling_rate_3MHz = sampling_rate_3MHz_standard*nfft_3MHz/nfft_3MHz_standard;

% Baseband FIR for 1.4MHz Rx,
fir_coef = fir1(512, 1.4e6/sampling_rate_3MHz).';
len_fir_half = (length(fir_coef)-1)/2;
freqz(fir_coef,1,512)

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;

% Baseband FIR for 1.08 MHz Rx at 1.92 MHz,
fir_coef2 = fir1(512, (72*15000)/sampling_rate_1dot4MHz).';
len_fir_half2 = (length(fir_coef2)-1)/2;
%freqz(fir_coef2,1,512)

M = (nfft_3MHz/nfft_1dot4MHz);

% --- PSS signal of 1.4M BW 6RB ----
pss_signal = [zeros(1,5) pss zeros(1,5)];

% ------------------------------------------------------------------------
% Generate 2 x 1.4 MHz LTE signals at different channels via frequency domain

s_3MHz_15RB = zeros(nfft_3MHz,1);
% Frequency domain multiplexing of two 1.4 MHz LTE into subcarrier of 3 MHz LTE.
% Subcarrier mapping.

% Map 1st vPHY: channel 0
s_3MHz_15RB(nfft_3MHz-(num_sc_6RB/2)+1:nfft_3MHz) = pss_signal(1:num_sc_6RB/2);
s_3MHz_15RB(2:(num_sc_6RB/2)+1) = pss_signal(num_sc_6RB/2+1:end);

% Map 1st vPHY: channel 1
s_3MHz_15RB(nfft_1dot4MHz-(num_sc_6RB/2)+1:nfft_1dot4MHz) = pss_signal(1:num_sc_6RB/2);
s_3MHz_15RB(nfft_1dot4MHz+2:nfft_1dot4MHz+(num_sc_6RB/2)+1) = pss_signal(num_sc_6RB/2+1:end);

figure; 
plot(0:1:255,(abs(s_3MHz_15RB))); 
title('Multiplexing four 6 RB into 15 RB for 3 MHz Tx.');
grid on;

% -------------- 3 MHz LTE Tx: 1 CP + 1 OFDM symbol ----------------------
freq_sc_3MHz = fftshift(s_3MHz_15RB);
ofdm_symbol_3MHz = ifft(freq_sc_3MHz,nfft_3MHz).*(nfft_3MHz/nfft_1dot4MHz);
ofdm_symbol_with_cp_3MHz = [ofdm_symbol_3MHz((end-(len_cp_3MHz-1)):end); ofdm_symbol_3MHz];

tx_3MHz_signal = ifftshift(fft(ofdm_symbol_3MHz, nfft_3MHz));
figure
plot(0:1:nfft_3MHz-1, (abs(tx_3MHz_signal/(nfft_3MHz/nfft_1dot4MHz))))
title('Transmitted 3 MHz signal.')
grid on;

%% Receive signal as they were transmitted by individual PHYs.
% --------Two independent 1.4 MHz receiver at +6MHz and -6MHz------------
t_idx = (-len_cp_3MHz:(length(ofdm_symbol_with_cp_3MHz)-len_cp_3MHz-1)).';

% Receiving 1.4 MHz LTE at 0 MHz offset:
rx_n5dot76MHz = ofdm_symbol_with_cp_3MHz.*exp(+1i.*2.*pi.*(0).*t_idx./nfft_3MHz);
rx_n5dot76MHz = conv(rx_n5dot76MHz, fir_coef);
rx_n5dot76MHz = rx_n5dot76MHz((len_fir_half+1):(end-len_fir_half));
rx_n5dot76MHz = rx_n5dot76MHz(1:(nfft_3MHz/nfft_1dot4MHz):end);

% Receiving 1.4 MHz LTE at 1.92 MHz offset:
rx_n3dot84MHz = ofdm_symbol_with_cp_3MHz.*exp(+1i.*2.*pi.*(128).*t_idx./nfft_3MHz);
rx_n3dot84MHz = conv(rx_n3dot84MHz, fir_coef);
rx_n3dot84MHz = rx_n3dot84MHz((len_fir_half+1):(end-len_fir_half));
rx_n3dot84MHz = rx_n3dot84MHz(1:(nfft_3MHz/nfft_1dot4MHz):end);

figure
%plot(10*log10(abs(fft(rx_n3dot84MHz(len_cp_1dot4MHz+1:end),128))))
plot((abs(fft(rx_n3dot84MHz(len_cp_1dot4MHz+1:end),128))))

figure
%plot(10*log10(abs(fft(rx_n3dot84MHz(len_cp_1dot4MHz+1:end),128))))
plot((abs(fft(rx_n5dot76MHz(len_cp_1dot4MHz+1:end),128))))
