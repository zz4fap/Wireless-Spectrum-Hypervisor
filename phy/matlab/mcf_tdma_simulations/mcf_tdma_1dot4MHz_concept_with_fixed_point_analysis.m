close all;clear all;clc

rng(12041988)

% test frequency domain multiplexing concept of McF-TDMA
% 14M == 1.4M for naming purpose

% -----------------------basic definitions--------------------------
sc_spacing = 15e3;
freq_offset_1dot4MHz_in_20MHz = 6e6;

num_sc_per_RB = 12;
num_sc_6RB = 6*num_sc_per_RB;
num_sc_100RB = 100*num_sc_per_RB;

nfft_20MHz_standard = 2048;
len_cp_20M_standard = 144;
sampling_rate_20M_standard = 30.72e6;

nfft_20MHz = 1536;
len_cp_20MHz = len_cp_20M_standard*nfft_20MHz/nfft_20MHz_standard;
sampling_rate_20MHz = sampling_rate_20M_standard*nfft_20MHz/nfft_20MHz_standard;

fir_coef = fir1(512, 1.4e6/sampling_rate_20MHz).'; % baseband fir of 1.4M rx
len_fir_half = (length(fir_coef)-1)/2;
freqz(fir_coef,1,512)

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20M_standard*nfft_1dot4MHz/nfft_20MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_20M_standard*nfft_1dot4MHz/nfft_20MHz_standard;

% ---fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
s_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% -------------- 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
freq_sc_1dot4MHz = [s_1dot4MHz_6RB(((num_sc_6RB/2)+1):end); zeros(nfft_1dot4MHz-num_sc_6RB,1); s_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% ------------------------------------------------------------------------
% Generate two 1.4 MHz LTE signal (at +6MHz and -6MHz) via frequency domain
% multiplexing through single 20MHz LTE transmitter

sc_idx_offset_6MHz = freq_offset_1dot4MHz_in_20MHz/sc_spacing;
% subcarrier idx of negative 6MHz offset 1.4MHz LTE in 20MHz LTE
sc_sp_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
sc_ep_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz+(num_sc_6RB/2);
% subcarrier idx of positive 6MHz offset 1.4MHz LTE in 20MHz LTE
sc_sp_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
sc_ep_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz+(num_sc_6RB/2);

%s_20MHz_100RB = zeros(num_sc_100RB,1);
s_20MHz_100RB = 1e-10*(1/sqrt(2))*complex(randn(num_sc_100RB,1),randn(num_sc_100RB,1)); 
% Frequency domain multiplexing of two 1.4M LTE into subcarrier of 20M LTE
s_20MHz_100RB(sc_sp_n6MHz:sc_ep_n6MHz) = s_1dot4MHz_6RB;
s_20MHz_100RB(sc_sp_p6MHz:sc_ep_p6MHz) = s_1dot4MHz_6RB;

if(0)
    L = 72;
    %w = hamming(L);
    %w = hann(L);
    w = kaiser(L,2.5);
    %w = blackman(L);
    %w = bartlett(L);
    %w = chebwin(L);
    plot(w)
    s_20MHz_100RB(sc_sp_n6MHz:sc_ep_n6MHz) = s_1dot4MHz_6RB.*w;
    s_20MHz_100RB(sc_sp_p6MHz:sc_ep_p6MHz) = s_1dot4MHz_6RB.*w;
end

% Frequency domain multiplexing of two 1.4 MHz LTE into subcarrier of 20 MHz LTE.
figure; 
plot(abs(s_20MHz_100RB)); 
title('Multiplexing two 6 RB into 100 RB for 20MHz Tx'); 
grid on;

% --------------20MHz LTE Tx: 1 CP + 1 OFDM symbol----------------------
freq_sc_20MHz = single([s_20MHz_100RB(((num_sc_100RB/2)+1):end); zeros(nfft_20MHz-num_sc_100RB,1); s_20MHz_100RB(1:(num_sc_100RB/2))]);
ofdm_symbol_20MHz = ifft(freq_sc_20MHz,nfft_20MHz).*(nfft_20MHz/nfft_1dot4MHz);
ofdm_symbol_with_cp_20MHz = [ofdm_symbol_20MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_20MHz];

% Plot power spectral density (PSD) per subband
hFig1 = figure;
[psd1,f1] = periodogram(ofdm_symbol_20MHz, rectwin(length(ofdm_symbol_20MHz)), nfft_20MHz, 1, 'centered');
psd1_pwr = 10*log10(psd1);
diff_inf = find(psd1_pwr(1:360) ~= -Inf);
avg_power1 = sum((psd1_pwr(diff_inf)))/length(diff_inf);
plot(f1,psd1_pwr);
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
titleStr1 = sprintf('Single precision - Avg. noise floor: %1.2f [dBW/Hz]',avg_power1);
title(titleStr1)

% Plot power spectral density (PSD) per subband
fi_signal_single = fi(ofdm_symbol_20MHz,1,16,15);
hFig2 = figure;
[psd2,f2] = periodogram(fi_signal_single.data, rectwin(length(fi_signal_single)), nfft_20MHz, 1, 'centered');
psd2_pwr = 10*log10(psd2);
avg_power2 = sum((psd2_pwr(1:360)))/360;
plot(f2,psd2_pwr);
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
titleStr2 = sprintf('Fixed point Q16.15 - Avg. noise floor: %1.2f [dBW/Hz]',avg_power2);
title(titleStr2)
grid on

if(0)
    h1=figure;
    double_precision_fft = fft(ofdm_symbol_20MHz,nfft_20MHz);
    avg_power = 10*log10(sum(abs(double_precision_fft(1:360)))/360);
    plot(10*log10(abs(double_precision_fft)))
    titleStr = sprintf('Double precision: %f',avg_power);
    title(titleStr)
    xlabel('IFFT points')
    ylabel('10*log10(|Y|)')
    %saveas(h1,'double_precision.png')
    
    fi_signal = fi(ofdm_symbol_20MHz,1,16,15);
    h2=figure;
    plot(10*log10(abs(fft(fi_signal.data,nfft_20MHz))))
    title('Fixed point Q16.15 (from double precision)')
    xlabel('IFFT points')
    ylabel('10*log10(|Y|)')
    %saveas(h2,'fixed_point.png')
    
    freq_sc_20MHz_single = single(freq_sc_20MHz);
    ofdm_symbol_20MHz_single = ifft(freq_sc_20MHz_single,nfft_20MHz).*(nfft_20MHz/nfft_1dot4MHz);
    ofdm_symbol_with_cp_20MHz_single = [ofdm_symbol_20MHz_single((end-(len_cp_20MHz-1)):end); ofdm_symbol_20MHz_single];
    
    h3=figure;
    plot(10*log10(abs(fft(ofdm_symbol_20MHz_single,nfft_20MHz))))
    title('Single precision')
    xlabel('IFFT points')
    ylabel('10*log10(|Y|)')
    %saveas(h3,'double_precision.png')
    
    fi_signal_single = fi(ofdm_symbol_20MHz_single,1,16,15);
    h4=figure;
    fft_signal_fixed_point = fft(fi_signal_single.data,nfft_20MHz);
    avg_power = 10*log10(sum(abs(fft_signal_fixed_point(1:360)))/360);
    plot(10*log10(abs(fft_signal_fixed_point)))
    titleStr = sprintf('Fixed point Q16.15 (from single precision): %f',avg_power);
    title(titleStr)
    xlabel('IFFT points')
    ylabel('10*log10(|Y|)')
    %saveas(h2,'fixed_point.png')
end

% --------Two independent 1.4 MHz receiver at +6MHz and -6MHz------------
t_idx = (-len_cp_20MHz:(length(ofdm_symbol_with_cp_20MHz)-len_cp_20MHz-1)).';

% receiving 1.4M LTE at -6MHz offset:
rx_n6MHz = ofdm_symbol_with_cp_20MHz.*exp(+1i.*2.*pi.*freq_offset_1dot4MHz_in_20MHz.*t_idx./sampling_rate_20MHz);
rx_n6MHz = conv(rx_n6MHz, fir_coef);
rx_n6MHz = rx_n6MHz((len_fir_half+1):(end-len_fir_half));
rx_n6MHz = rx_n6MHz(1:(nfft_20MHz/nfft_1dot4MHz):end);

% receiving 1.4M LTE at +6MHz offset:
rx_p6MHz = ofdm_symbol_with_cp_20MHz.*exp(-1i.*2.*pi.*freq_offset_1dot4MHz_in_20MHz.*t_idx./sampling_rate_20MHz);
rx_p6MHz = conv(rx_p6MHz, fir_coef);
rx_p6MHz = rx_p6MHz((len_fir_half+1):(end-len_fir_half));
rx_p6MHz = rx_p6MHz(1:(nfft_20MHz/nfft_1dot4MHz):end);

figure
imag_ofdm_symbol_with_cp_1dot4MHz = imag(ofdm_symbol_with_cp_1dot4MHz);
imag_rx_n6MHz = imag(rx_n6MHz);
imag_rx_p6MHz = imag(rx_p6MHz);
plot(imag_ofdm_symbol_with_cp_1dot4MHz,'k')
grid on
hold on
plot(imag_rx_n6MHz,'r.')
plot(imag_rx_p6MHz,'bs')
legend('1.4 MHz Tx by 1.92 Msps LTE Tx', '1.4 MHz Rx #0 from 20 MHz LTE Tx', '1.4 MHz Rx #1 from 20 MHz LTE Tx')
title('Time domain imaginary part of 1.4MHz LTE signal')
hold off

figure
real_ofdm_symbol_with_cp_1dot4MHz = real(ofdm_symbol_with_cp_1dot4MHz);
real_rx_n6MHz = real(rx_n6MHz);
real_rx_p6MHz = real(rx_p6MHz);
plot(real_ofdm_symbol_with_cp_1dot4MHz,'k')
grid on
hold on
plot(real_rx_n6MHz,'r.')
plot(real_rx_p6MHz,'bs')
legend('1.4 MHz Tx by 1.92 Msps LTE Tx', '1.4 MHz Rx #0 from 20 MHz LTE Tx', '1.4 MHz Rx #1 from 20 MHz LTE Tx')
title('Time domain real part of 1.4MHz LTE signal')
hold off
