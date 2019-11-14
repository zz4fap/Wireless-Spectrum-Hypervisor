% Proof of concept McF-TDMA proposal.
close all; clear all;clc;

rng(140281)

% ----------------------- Definitions --------------------------
enable_dc_carrier = true;

sc_spacing = 15e3;
freq_offset_1dot4MHz_in_20MHz = 128*15000; % offset in Hz: 1.92 MHz

num_sc_per_RB = 12;
num_sc_6RB = 6*num_sc_per_RB;
num_sc_100RB = 100*num_sc_per_RB;

nfft_20MHz_standard = 2048;
len_cp_20MHz_standard = 144; % Fisrt OFDM symbol 160, subsequently ones, 144.
sampling_rate_20MHz_standard = 30.72e6;

nfft_20MHz = 1536;
len_cp_20MHz = len_cp_20MHz_standard*nfft_20MHz/nfft_20MHz_standard;
sampling_rate_20MHz = sampling_rate_20MHz_standard*nfft_20MHz/nfft_20MHz_standard;

% Baseband FIR for 1.4MHz Rx,
fir_coef = fir1(512, 1.4e6/sampling_rate_20MHz).';
len_fir_half = (length(fir_coef)-1)/2;
%freqz(fir_coef,1,512)

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;

% Baseband FIR for 1.08 MHz Rx at 1.92 MHz,
fir_coef2 = fir1(512, (72*15000)/sampling_rate_1dot4MHz).';
len_fir_half2 = (length(fir_coef2)-1)/2;
%freqz(fir_coef2,1,512)

M = (nfft_20MHz/nfft_1dot4MHz);

% ---Signal #0: fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
signal0_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% ---Signal #1: fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
signal1_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% ---Signal #2: fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
signal2_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% ---Signal #3: fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
signal3_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% ---Signal #2: fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
signal4_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% ---Signal #3: fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
signal5_1dot4MHz_6RB = sign(randn(num_sc_6RB,1)) + 1i.*sign(randn(num_sc_6RB,1));

% -------------- Signal #0: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
freq_sc_1dot4MHz = [0;signal0_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal0_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol0_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #1: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
freq_sc_1dot4MHz = [0;signal1_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal1_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol1_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #2: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
freq_sc_1dot4MHz = [0;signal2_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal2_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol2_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #3: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
freq_sc_1dot4MHz = [0;signal3_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal3_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol3_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #4: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
freq_sc_1dot4MHz = [0;signal4_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal4_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol4_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #5: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
freq_sc_1dot4MHz = [0;signal5_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal5_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol5_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% ------------------------------------------------------------------------
% Generate four 1.4 MHz LTE signals at different channels via frequency domain
% Multiplexing through single 20 MHz LTE transmitter.

sc_idx_offset = (freq_offset_1dot4MHz_in_20MHz/sc_spacing);
% subcarrier idx for -3*1.92 MHz offset in 20MHz LTE
sc_sp_n5dot76MHz = (num_sc_100RB/2)-(3*sc_idx_offset)-(num_sc_6RB/2);
sc_ep_n5dot76MHz = (num_sc_100RB/2)-(3*sc_idx_offset)+(num_sc_6RB/2);
% subcarrier idx for -2*1.92 MHz offset in 20MHz LTE
sc_sp_n3dot84MHz = (num_sc_100RB/2)-(2*sc_idx_offset)-(num_sc_6RB/2);
sc_ep_n3dot84MHz = (num_sc_100RB/2)-(2*sc_idx_offset)+(num_sc_6RB/2);
% subcarrier idx for -1*1.92 MHz offset in 20MHz LTE
sc_sp_n1dot92MHz = (num_sc_100RB/2)-sc_idx_offset-(num_sc_6RB/2);
sc_ep_n1dot92MHz = (num_sc_100RB/2)-sc_idx_offset+(num_sc_6RB/2);
% subcarrier idx for +1*1.92 MHz offset in 20MHz LTE
sc_sp_p1dot92MHz = (num_sc_100RB/2)+sc_idx_offset-(num_sc_6RB/2);
sc_ep_p1dot92MHz = (num_sc_100RB/2)+sc_idx_offset+(num_sc_6RB/2);
% subcarrier idx for +2*1.92 MHz offset in 20MHz LTE
sc_sp_p3dot84MHz = (num_sc_100RB/2)+(2*sc_idx_offset)-(num_sc_6RB/2);
sc_ep_p3dot84MHz = (num_sc_100RB/2)+(2*sc_idx_offset)+(num_sc_6RB/2);
% subcarrier idx for +3*1.92 MHz offset in 20MHz LTE
sc_sp_p5dot76MHz = (num_sc_100RB/2)+(3*sc_idx_offset)-(num_sc_6RB/2);
sc_ep_p5dot76MHz = (num_sc_100RB/2)+(3*sc_idx_offset)+(num_sc_6RB/2);

% Frequency domain multiplexing of four 1.4 MHz LTE into subcarrier of 20 MHz LTE.
% Subcarrier mapping.
s_20MHz_100RB = zeros(num_sc_100RB,1);
if(enable_dc_carrier)
    s_20MHz_100RB(sc_sp_n5dot76MHz:sc_ep_n5dot76MHz) = [signal4_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal4_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_n3dot84MHz:sc_ep_n3dot84MHz) = [signal0_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal0_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_n1dot92MHz:sc_ep_n1dot92MHz) = [signal1_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal1_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_p1dot92MHz:sc_ep_p1dot92MHz) = [signal2_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal2_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_p3dot84MHz:sc_ep_p3dot84MHz) = [signal3_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal3_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_p5dot76MHz:sc_ep_p5dot76MHz) = [signal5_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal5_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
else
    s_20MHz_100RB(sc_sp_n5dot76MHz:sc_ep_n5dot76MHz) = [signal4_1dot4MHz_6RB(1:num_sc_6RB/2);signal4_1dot4MHz_6RB(1);signal4_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_n3dot84MHz:sc_ep_n3dot84MHz) = [signal0_1dot4MHz_6RB(1:num_sc_6RB/2);signal0_1dot4MHz_6RB(1);signal0_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_n1dot92MHz:sc_ep_n1dot92MHz) = [signal1_1dot4MHz_6RB(1:num_sc_6RB/2);signal1_1dot4MHz_6RB(1);signal1_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_p1dot92MHz:sc_ep_p1dot92MHz) = [signal2_1dot4MHz_6RB(1:num_sc_6RB/2);signal2_1dot4MHz_6RB(1);signal2_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_p3dot84MHz:sc_ep_p3dot84MHz) = [signal3_1dot4MHz_6RB(1:num_sc_6RB/2);signal3_1dot4MHz_6RB(1);signal3_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
    s_20MHz_100RB(sc_sp_p5dot76MHz:sc_ep_p5dot76MHz) = [signal5_1dot4MHz_6RB(1:num_sc_6RB/2);signal5_1dot4MHz_6RB(1);signal5_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
end

figure; 
plot((abs(s_20MHz_100RB))); 
title('Multiplexing four 6 RB into 100 RB for 20 MHz Tx');
grid on;

% -------------- 20 MHz LTE Tx: 1 CP + 1 OFDM symbol ----------------------
freq_sc_20MHz = [s_20MHz_100RB(((num_sc_100RB/2)):end); zeros(nfft_20MHz-num_sc_100RB,1); s_20MHz_100RB(1:(num_sc_100RB/2)-1)];
ofdm_symbol_20MHz = ifft(freq_sc_20MHz,nfft_20MHz).*(nfft_20MHz/nfft_1dot4MHz);
ofdm_symbol_with_cp_20MHz = [ofdm_symbol_20MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_20MHz];

tx_20MHz_signal = fft(ofdm_symbol_20MHz,nfft_20MHz);
figure
plot(0:1:nfft_20MHz-1,10*log10(abs(tx_20MHz_signal/(nfft_20MHz/nfft_1dot4MHz))))
title('Transmitted 20 MHz signal.')
grid on;

figure
plot(0:1:nfft_20MHz-1,10*log10(abs(tx_20MHz_signal/(nfft_20MHz/nfft_1dot4MHz))))
axis([0 (nfft_20MHz-1) -175 -150])
title('Transmitted 20 MHz signal.')
grid on;


distortion_vector = complex(zeros(1,nfft_20MHz - 6*(72 + 1)), zeros(1,nfft_20MHz - 6*(72 + 1)));
signal_position_vector = [sc_sp_n5dot76MHz:sc_ep_n5dot76MHz sc_sp_n3dot84MHz:sc_ep_n3dot84MHz sc_sp_n1dot92MHz:sc_ep_n1dot92MHz sc_sp_p1dot92MHz:sc_ep_p1dot92MHz sc_sp_p3dot84MHz:sc_ep_p3dot84MHz sc_sp_p5dot76MHz:sc_ep_p5dot76MHz];
idx = 0;
for i=1:1:nfft_20MHz
    
    is_empty = find(i == signal_position_vector);
    if(isempty(is_empty)==0)
        a=1;
    end
    if(isempty(is_empty)==1)
        idx = idx + 1;
        value = tx_20MHz_signal(i);
        distortion_vector(idx) = value;
    end
    
end


%% ************** Windowing ***********************
L = length(signal0_1dot4MHz_6RB);
%window = hamming(L);
%window = blackman(L);
%window = window(@kaiser,L,0.1);
%window = kaiser(L, 10.0);
window = hann(L);

signal0_1dot4MHz_6RB_wind = signal0_1dot4MHz_6RB.*window;

signal1_1dot4MHz_6RB_wind = signal1_1dot4MHz_6RB.*window;

signal2_1dot4MHz_6RB_wind = signal2_1dot4MHz_6RB.*window;

signal3_1dot4MHz_6RB_wind = signal3_1dot4MHz_6RB.*window;

signal4_1dot4MHz_6RB_wind = signal4_1dot4MHz_6RB.*window;

signal5_1dot4MHz_6RB_wind = signal5_1dot4MHz_6RB.*window;

% Frequency domain multiplexing of four 1.4 MHz LTE into subcarrier of 20 MHz LTE.
% Subcarrier mapping.
s_20MHz_100RB = zeros(num_sc_100RB,1);
if(enable_dc_carrier)
s_20MHz_100RB(sc_sp_n5dot76MHz:sc_ep_n5dot76MHz) = [signal4_1dot4MHz_6RB_wind(1:num_sc_6RB/2);0;signal4_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_n3dot84MHz:sc_ep_n3dot84MHz) = [signal0_1dot4MHz_6RB_wind(1:num_sc_6RB/2);0;signal0_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_n1dot92MHz:sc_ep_n1dot92MHz) = [signal1_1dot4MHz_6RB_wind(1:num_sc_6RB/2);0;signal1_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_p1dot92MHz:sc_ep_p1dot92MHz) = [signal2_1dot4MHz_6RB_wind(1:num_sc_6RB/2);0;signal2_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_p3dot84MHz:sc_ep_p3dot84MHz) = [signal3_1dot4MHz_6RB_wind(1:num_sc_6RB/2);0;signal3_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_p5dot76MHz:sc_ep_p5dot76MHz) = [signal5_1dot4MHz_6RB_wind(1:num_sc_6RB/2);0;signal5_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
else
s_20MHz_100RB(sc_sp_n5dot76MHz:sc_ep_n5dot76MHz) = [signal4_1dot4MHz_6RB_wind(1:num_sc_6RB/2);signal4_1dot4MHz_6RB_wind(36);signal4_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_n3dot84MHz:sc_ep_n3dot84MHz) = [signal0_1dot4MHz_6RB_wind(1:num_sc_6RB/2);signal0_1dot4MHz_6RB_wind(36);signal0_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_n1dot92MHz:sc_ep_n1dot92MHz) = [signal1_1dot4MHz_6RB_wind(1:num_sc_6RB/2);signal1_1dot4MHz_6RB_wind(36);signal1_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_p1dot92MHz:sc_ep_p1dot92MHz) = [signal2_1dot4MHz_6RB_wind(1:num_sc_6RB/2);signal2_1dot4MHz_6RB_wind(36);signal2_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_p3dot84MHz:sc_ep_p3dot84MHz) = [signal3_1dot4MHz_6RB_wind(1:num_sc_6RB/2);signal3_1dot4MHz_6RB_wind(36);signal3_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_p5dot76MHz:sc_ep_p5dot76MHz) = [signal5_1dot4MHz_6RB_wind(1:num_sc_6RB/2);signal5_1dot4MHz_6RB_wind(36);signal5_1dot4MHz_6RB_wind((num_sc_6RB/2)+1:end)];    
end

figure; 
plot((abs(s_20MHz_100RB))); 
title('Multiplexing four 6 RB into 100 RB for 20 MHz Tx');
grid on;

% -------------- 20 MHz LTE Tx: 1 CP + 1 OFDM symbol ----------------------
freq_sc_20MHz = [s_20MHz_100RB(((num_sc_100RB/2)):end); zeros(nfft_20MHz-num_sc_100RB,1); s_20MHz_100RB(1:(num_sc_100RB/2)-1)];
ofdm_symbol_20MHz = ifft(freq_sc_20MHz,nfft_20MHz).*(nfft_20MHz/nfft_1dot4MHz);
ofdm_symbol_with_cp_20MHz = [ofdm_symbol_20MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_20MHz];

tx_20MHz_signal = fft(ofdm_symbol_20MHz,nfft_20MHz);
figure
plot(0:1:nfft_20MHz-1,10*log10(abs(tx_20MHz_signal/(nfft_20MHz/nfft_1dot4MHz))))
title('Transmitted 20 MHz signal.')
grid on;

figure
plot(0:1:nfft_20MHz-1,10*log10(abs(tx_20MHz_signal/(nfft_20MHz/nfft_1dot4MHz))))
axis([0 (nfft_20MHz-1) -175 -150])
title('Transmitted 20 MHz signal.')
grid on;
