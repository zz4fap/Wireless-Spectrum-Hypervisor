% test frequency domain multiplexing concept of McF-TDMA
% 14M == 1.4M for naming purpose
close all;

% -----------------------basic definitions--------------------------
sc_spacing = 15e3;
freq_offset_14M_in_20M = 6e6;

num_sc_per_RB = 12;
num_sc_6RB = 6*num_sc_per_RB;
num_sc_100RB = 100*num_sc_per_RB;

nfft_20M_standard = 2048;
len_cp_20M_standard = 144;
sampling_rate_20M_standard = 30.72e6;

nfft_20M = 1536;
len_cp_20M = len_cp_20M_standard*nfft_20M/nfft_20M_standard;
sampling_rate_20M = sampling_rate_20M_standard*nfft_20M/nfft_20M_standard;

fir_coef = fir1(2*512, 1.4e6/sampling_rate_20M).'; % baseband fir of 1.4M rx
%fir_coef = fir1(2*512, 1.08e6/sampling_rate_20M).'; % baseband fir of 1.4M rx
%fir_coef = fir1(2*512, 1.92e6/sampling_rate_20M).'; % baseband fir of 1.4M rx
len_fir_half = (length(fir_coef)-1)/2;

nfft_14M = 128;
len_cp_14M = len_cp_20M_standard*nfft_14M/nfft_20M_standard;
sampling_rate_14M = sampling_rate_20M_standard*nfft_14M/nfft_20M_standard;

% ---fake frequency subcarrier(data/pilot/pss/sss/etc) of 1.4M BW 6RB----
% modulation order M. (64QAM)
M = 64;
% Constellation size = 2^modOrd.
modOrd = log2(M);
% QAM Symbol mapper.
dataMapper = comm.RectangularQAMModulator('ModulationOrder', 2^modOrd, 'BitInput', true, 'NormalizationMethod', 'Average power');

numIter = 100000;

error_s1 = 0;
error_s2 = 0;
for iter=1:1:numIter
    
    msg = randi([0 1], modOrd*num_sc_6RB, 1);
    s_14M_6RB = dataMapper(msg);
    
    % --------------1.4MHz LTE tx: 1 CP + 1 OFDM symbol----------------------
    freq_sc_14M = [s_14M_6RB(((num_sc_6RB/2)+1):end); zeros(nfft_14M-num_sc_6RB,1); s_14M_6RB(1:(num_sc_6RB/2))];
    ofdm_symbol_14M = ifft(freq_sc_14M, nfft_14M);
    ofdm_symbol_with_cp_14M = [ofdm_symbol_14M((end-(len_cp_14M-1)):end); ofdm_symbol_14M];
    
    % ------------------------------------------------------------------------
    % Generate two 1.4MHz LTE signal (at +6MHz and -6MHz) via frequency domain
    % multiplexing through single 20MHz LTE transmitter
    
    sc_idx_offset_6MHz = freq_offset_14M_in_20M/sc_spacing;
    % subcarrier idx of negative 6MHz offset 1.4MHz LTE in 20M LTE
    sc_sp_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
    sc_ep_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz+(num_sc_6RB/2);
    % subcarrier idx of positive 6MHz offset 1.4MHz LTE in 20M LTE
    sc_sp_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
    sc_ep_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz+(num_sc_6RB/2);
    
    s_20M_100RB = zeros(num_sc_100RB,1);
    % frequency domain multiplexing of two 1.4M LTE into subcarrier of 20M LTE
    s_20M_100RB(sc_sp_n6MHz:sc_ep_n6MHz) = s_14M_6RB;
    s_20M_100RB(sc_sp_p6MHz:sc_ep_p6MHz) = s_14M_6RB;
    
    % --------------20MHz LTE tx: 1 CP + 1 OFDM symbol----------------------
    freq_sc_20M = [s_20M_100RB(((num_sc_100RB/2)+1):end); zeros(nfft_20M-num_sc_100RB,1); s_20M_100RB(1:(num_sc_100RB/2))];
    ofdm_symbol_20M = ifft(freq_sc_20M,nfft_20M).*(nfft_20M/nfft_14M);
    ofdm_symbol_with_cp_20M = [ofdm_symbol_20M((end-(len_cp_20M-1)):end); ofdm_symbol_20M];
    
    % --------two independent 1.4MHz receiver at +6MHz and -6MHz------------
    t_idx = (-len_cp_20M:(length(ofdm_symbol_with_cp_20M)-len_cp_20M-1)).';
    
    % receiving 1.4M LTE at -6MHz offset:
    rx_n6MHz = ofdm_symbol_with_cp_20M.*exp(+1i.*2.*pi.*freq_offset_14M_in_20M.*t_idx./sampling_rate_20M);
    rx_n6MHz = conv(rx_n6MHz, fir_coef);
    rx_n6MHz = rx_n6MHz((len_fir_half+1):(end-len_fir_half));
    rx_n6MHz = rx_n6MHz(1:(nfft_20M/nfft_14M):end);
    
    % receiving 1.4M LTE at +6MHz offset:
    rx_p6MHz = ofdm_symbol_with_cp_20M.*exp(-1i.*2.*pi.*freq_offset_14M_in_20M.*t_idx./sampling_rate_20M);
    rx_p6MHz = conv(rx_p6MHz, fir_coef);
    rx_p6MHz = rx_p6MHz((len_fir_half+1):(end-len_fir_half));
    rx_p6MHz = rx_p6MHz(1:(nfft_20M/nfft_14M):end);
    
    error_s1 = error_s1 + (sum(abs(ofdm_symbol_with_cp_14M - rx_n6MHz).^2)/length(rx_n6MHz));

    error_s2 = error_s2 + (sum(abs(ofdm_symbol_with_cp_14M - rx_p6MHz).^2)/length(rx_p6MHz));
    
end


error_s1 = error_s1/numIter

error_s2 = error_s2/numIter

fprintf(1, 'Error signal 1: %1.4e - Error signal 2: %1.4e\n', error_s1, error_s2);
