close all;clear all;clc

rng(12041988)

use_fix_point = false;

use_window = true;

% test frequency domain multiplexing concept for McF-TDMA

% ----------------------- basic definitions --------------------------
sc_spacing = 15e3;
freq_offset_1dot4MHz_in_20MHz = 6e6;

num_sc_per_RB = 12;
num_sc_6RB = 6*num_sc_per_RB;
num_sc_100RB = 100*num_sc_per_RB;

nfft_20MHz_standard = 2048;
len_cp_20MHz_standard = 144;
sampling_rate_20MHz_standard = 30.72e6;

nfft_20MHz = 1536;
len_cp_20MHz = len_cp_20MHz_standard*(nfft_20MHz/nfft_20MHz_standard);
sampling_rate_20MHz = sampling_rate_20MHz_standard*(nfft_20MHz/nfft_20MHz_standard);

fir_coef = fir1(512, 1.4e6/sampling_rate_20MHz).'; % baseband fir of 1.4 MHz Rx
len_fir_half = (length(fir_coef)-1)/2;
freqz(fir_coef,1,512)

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20MHz_standard*(nfft_1dot4MHz/nfft_20MHz_standard);
sampling_rate_1dot4MHz = sampling_rate_20MHz_standard*(nfft_1dot4MHz/nfft_20MHz_standard);

% ---fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
s_1dot4MHz_6RB = (1/sqrt(2))*sign(randn(num_sc_6RB,1)) + 1i.*(1/sqrt(2))*sign(randn(num_sc_6RB,1));

% ------------------------------------------------------------------------
% Generate two 1.4 MHz LTE signal (at +6 MHz and -6 MHz) via frequency domain
% multiplexing through single 20 MHz LTE transmitter

sc_idx_offset_6MHz = freq_offset_1dot4MHz_in_20MHz/sc_spacing;
% subcarrier idx of negative 6 MHz offset 1.4 MHz LTE in 20 MHz LTE
sc_sp_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
sc_ep_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz+(num_sc_6RB/2);
% subcarrier idx of positive 6 MHz offset 1.4 MHz LTE in 20 MHz LTE
sc_sp_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
sc_ep_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz+(num_sc_6RB/2);

s_20MHz_100RB = zeros(num_sc_100RB,1);
% Frequency domain multiplexing of two 1.4 MHz LTE into subcarrier of 20 MHz LTE
s_20MHz_100RB(sc_sp_n6MHz:sc_ep_n6MHz) = s_1dot4MHz_6RB;
s_20MHz_100RB(sc_sp_p6MHz:sc_ep_p6MHz) = s_1dot4MHz_6RB;

if(use_window)
    L = 72;
    %w = hamming(L);
    w = hann(L);
    %w = kaiser(L,2.5);
    %w = blackman(L);
    %w = bartlett(L);
    %w = chebwin(L);
    %w = zeros(L,1);
    plot(w)
    s_20MHz_100RB(sc_sp_n6MHz:sc_ep_n6MHz) = s_1dot4MHz_6RB.*w;
    s_20MHz_100RB(sc_sp_p6MHz:sc_ep_p6MHz) = s_1dot4MHz_6RB.*w;
end

% Frequency domain multiplexing of two 1.4 MHz LTE into subcarrier of 20 MHz LTE.
figure;
plot(abs(s_20MHz_100RB));
title('Multiplexing two 6 RB into 100 RB for 20 MHz Tx');
grid on;

% -------------- 20 MHz LTE Tx: 1 CP + 1 OFDM symbol ----------------------
freq_sc_20MHz = single([s_20MHz_100RB(((num_sc_100RB/2)+1):end); zeros(nfft_20MHz-num_sc_100RB,1); s_20MHz_100RB(1:(num_sc_100RB/2))]);
ofdm_symbol_20MHz = ifft(freq_sc_20MHz,nfft_20MHz).*(nfft_20MHz/nfft_1dot4MHz);
ofdm_symbol_with_cp_20MHz = [ofdm_symbol_20MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_20MHz];


freq_ofdm_symbol_20MHz = fft(ofdm_symbol_20MHz,nfft_20MHz);
plot(10*log10(abs(freq_ofdm_symbol_20MHz).^2))


oob_vector = [freq_ofdm_symbol_20MHz(1:364); freq_ofdm_symbol_20MHz(437:1100); freq_ofdm_symbol_20MHz(1173:end)];

oob_avg = 10*log10(sum(abs(oob_vector).^2)/length(oob_vector));
fprintf(1,'OOB: %f\n',oob_avg);


% ------------------------ Create a subframe ------------------------------
subframe = [];
for i=1:1:14
    subframe = [subframe; ofdm_symbol_with_cp_20MHz];
end

if(use_fix_point)
    subframe_aux = fi(subframe,1,16,15);
    subframe_aux = subframe_aux.data;
else
    subframe_aux = subframe;
end

% ------------------------- Create Tx signal ----------------------
tx_signal = [];
for i=1:1:10
    tx_signal = [tx_signal; zeros(23040,1); subframe_aux];
end

% ------------------------- Add noise ----------------------
rx_signal = tx_signal + sqrt(0.0000001)*(1/sqrt(2))*complex(randn(length(tx_signal),1),randn(length(tx_signal),1));

if(use_fix_point)
    rx_signal_aux = fi(rx_signal,1,16,15);
    rx_signal_aux = rx_signal_aux.data;
else
    rx_signal_aux = rx_signal;
end

FFT_LENGTH = 2048;
overlap = 512;
h2 = figure('rend','painters','pos',[10 10 1000 900]);
spectrogram(rx_signal_aux, kaiser(FFT_LENGTH,5), overlap, FFT_LENGTH, sampling_rate_20MHz,'centered');
title('Windowing')

