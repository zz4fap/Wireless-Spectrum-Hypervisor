% Proof of concept McF-TDMA proposal.
close all; clear all;clc;

rng(1)

% Create a local random stream to be used by random number generators for repeatability.
hStr = RandStream('mt19937ar');

% ----------------------- Definitions --------------------------
SNR = 1000;

% Constellation size = 2^modOrd.
modOrd = 2;

% Create QPSK mod-demod objects.
hMod = modem.pskmod('M', 2^modOrd, 'SymbolOrder', 'gray', 'InputType', 'bit', 'phase', pi/(2^modOrd));
hDemod = modem.pskdemod(hMod);

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

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;

% Baseband FIR for 1.08 MHz Rx at 1.92 MHz,
fir_coef2 = fir1(512, (72*15000)/sampling_rate_1dot4MHz).';
len_fir_half2 = (length(fir_coef2)-1)/2;

numOfvPHYs = 4;
numDataSym = num_sc_6RB*numOfvPHYs;

nBits = 0;



% -------------- Signal #0: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
msg0 = randi(hStr, [0 1], modOrd, num_sc_6RB);
source0 = modulate(hMod, msg0);
signal0_1dot4MHz_6RB = source0.';
freq_sc_1dot4MHz = [0;signal0_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal0_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol0_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #1: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
msg1 = randi(hStr, [0 1], modOrd, num_sc_6RB);
source1 = modulate(hMod, msg1);
signal1_1dot4MHz_6RB = source1.';
freq_sc_1dot4MHz = [0;signal1_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal1_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol1_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #2: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
msg2 = randi(hStr, [0 1], modOrd, num_sc_6RB);
source2 = modulate(hMod, msg2);
signal2_1dot4MHz_6RB = source2.';
freq_sc_1dot4MHz = [0;signal2_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal2_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol2_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% -------------- Signal #3: 1.4MHz LTE TX: 1 CP + 1 OFDM symbol ----------------------
msg3 = randi(hStr, [0 1], modOrd, num_sc_6RB);
source3 = modulate(hMod, msg3);
signal3_1dot4MHz_6RB = source3.';
freq_sc_1dot4MHz = [0;signal3_1dot4MHz_6RB((num_sc_6RB/2)+1:end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); signal3_1dot4MHz_6RB(1:(num_sc_6RB/2))];
ofdm_symbol_1dot4MHz = ifft(freq_sc_1dot4MHz, nfft_1dot4MHz);
ofdm_symbol3_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];

% ------------------------------------------------------------------------
% Generate four 1.4 MHz LTE signals at different channels via frequency domain
% Multiplexing through single 20 MHz LTE transmitter
sc_idx_offset = (freq_offset_1dot4MHz_in_20MHz/sc_spacing);
% subcarrier idx of negative 3*1.92 MHz offset 1.4MHz LTE in 20MHz LTE
sc_sp_n5dot76MHz = (num_sc_100RB/2)-(3*sc_idx_offset)-(num_sc_6RB/2);
sc_ep_n5dot76MHz = (num_sc_100RB/2)-(3*sc_idx_offset)+(num_sc_6RB/2);
% subcarrier idx of negative 1.92 MHz offset 1.4MHz LTE in 20MHz LTE
sc_sp_n1dot92MHz = (num_sc_100RB/2)-sc_idx_offset-(num_sc_6RB/2);
sc_ep_n1dot92MHz = (num_sc_100RB/2)-sc_idx_offset+(num_sc_6RB/2);
% subcarrier idx of positive 1.92 MHz offset 1.4MHz LTE in 20MHz LTE
sc_sp_p1dot92MHz = (num_sc_100RB/2)+sc_idx_offset-(num_sc_6RB/2);
sc_ep_p1dot92MHz = (num_sc_100RB/2)+sc_idx_offset+(num_sc_6RB/2);
% subcarrier idx of positive 3*1.92 MHz offset 1.4MHz LTE in 20MHz LTE
sc_sp_p5dot76MHz = (num_sc_100RB/2)+(3*sc_idx_offset)-(num_sc_6RB/2);
sc_ep_p5dot76MHz = (num_sc_100RB/2)+(3*sc_idx_offset)+(num_sc_6RB/2);

s_20MHz_100RB = zeros(num_sc_100RB,1);
% Frequency domain multiplexing of four 1.4 MHz LTE into subcarrier of 20 MHz LTE.
% Subcarrier mapping.
s_20MHz_100RB(sc_sp_n5dot76MHz:sc_ep_n5dot76MHz) = [signal0_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal0_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_n1dot92MHz:sc_ep_n1dot92MHz) = [signal1_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal1_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_p1dot92MHz:sc_ep_p1dot92MHz) = [signal2_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal2_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];
s_20MHz_100RB(sc_sp_p5dot76MHz:sc_ep_p5dot76MHz) = [signal3_1dot4MHz_6RB(1:num_sc_6RB/2);0;signal3_1dot4MHz_6RB((num_sc_6RB/2)+1:end)];

% -------------- 20 MHz LTE Tx: 1 CP + 1 OFDM symbol ----------------------
freq_sc_20MHz = [s_20MHz_100RB(((num_sc_100RB/2)):end); zeros(nfft_20MHz-num_sc_100RB,1); s_20MHz_100RB(1:(num_sc_100RB/2)-1)];
ofdm_symbol_20MHz = ifft(freq_sc_20MHz,nfft_20MHz).*(nfft_20MHz/nfft_1dot4MHz);
ofdm_symbol_with_cp_20MHz = [ofdm_symbol_20MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_20MHz];

%% ***************************** Channel *****************************
idx = 1;

% Add channel noise power to faded data.
noisy_ofdm_symbol_with_cp_20MHz = awgn(ofdm_symbol_with_cp_20MHz, SNR(idx), 0, hStr);

%% ***************************** Receiver side *****************************
% Simple Channelizer with FFT.
M = (nfft_20MHz/nfft_1dot4MHz);
numIter = length(noisy_ofdm_symbol_with_cp_20MHz)/M;

vchannel = zeros(M,numIter);
for iter=1:1:numIter
    
    rx_signal_channelizer = (fft(noisy_ofdm_symbol_with_cp_20MHz(M*(iter-1)+1:M*(iter)),M)/M);
    
    for vch_idx=1:1:M
        vchannel(vch_idx,iter) = rx_signal_channelizer(vch_idx);
    end
end

enable_filter = 0;
if(enable_filter==1)
    for vch_idx=1:1:M
        aux = conv(vchannel(vch_idx,:), fir_coef2.');
        vchannel(vch_idx,:) = aux((len_fir_half2+1):(end-len_fir_half2));
    end
end

rx_1dot4MHz_ofdm_symbols = vchannel(:,len_cp_1dot4MHz+1:end);

nErrs = 0;
channel_indexes = [2, 4, 10, 12];
demoded_data = zeros(2,num_sc_6RB*numOfvPHYs);
for vch_idx=1:1:length(channel_indexes)
    
    rx_data = (fft(rx_1dot4MHz_ofdm_symbols(channel_indexes(vch_idx),:),nfft_1dot4MHz));
    
    rx_data = [rx_data((nfft_1dot4MHz/2)+1:end), rx_data(1:nfft_1dot4MHz/2)];
    
    plot(abs(rx_data))
    
    subcarrier_indexes = [((nfft_1dot4MHz-num_sc_6RB)/2)+1:((nfft_1dot4MHz-num_sc_6RB)/2)+(num_sc_6RB/2), ((nfft_1dot4MHz-num_sc_6RB)/2)+(num_sc_6RB/2)+2:nfft_1dot4MHz-((nfft_1dot4MHz-num_sc_6RB)/2)+1];
    
    %demoded_data(:,num_sc_6RB*(vch_idx-1)+1:num_sc_6RB*vch_idx) = demodulate(hDemod, rx_data(subcarrier_indexes));
    
    aaa = demodulate(hDemod, rx_data(subcarrier_indexes));
    
    nErrs = nErrs + biterr(msg2, aaa);
end


                

