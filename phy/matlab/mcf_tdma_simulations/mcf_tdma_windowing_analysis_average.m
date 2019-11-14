close all;clear all;clc

rng(12041988)

use_fix_point = false;

use_window = true;

% Window size.
L = 72;

window_type = ["none"; "hanning"; "hamming"; "kaiser"; "blackman"; "bartlett"; "chebwin"];

numIter = 10000;

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

oob_avg = zeros(1,length(window_type));
for winType=1:1:length(window_type)
    
    window_type_str = window_type(winType);
    
    for iter=1:1:numIter
        
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
            switch(window_type_str)
                case 'none'
                    w = ones(L,1);
                case 'hanning'
                    w = hann(L);
                case 'hamming'
                    w = hamming(L);
                case 'kaiser'
                    w = kaiser(L,2.5);
                case 'blackman'
                    w = blackman(L);
                case 'bartlett'
                    w = bartlett(L);
                case 'chebwin'
                    w = chebwin(L);
                case 'zeros'
                    w = zeros(L,1);
                otherwise
                    w = zeros(L,1);
            end
            
            s_20MHz_100RB(sc_sp_n6MHz:sc_ep_n6MHz) = s_1dot4MHz_6RB.*w;
            s_20MHz_100RB(sc_sp_p6MHz:sc_ep_p6MHz) = s_1dot4MHz_6RB.*w;
        end
        
        % -------------- 20 MHz LTE Tx: 1 CP + 1 OFDM symbol ----------------------
        freq_sc_20MHz = single([s_20MHz_100RB(((num_sc_100RB/2)+1):end); zeros(nfft_20MHz-num_sc_100RB,1); s_20MHz_100RB(1:(num_sc_100RB/2))]);
        ofdm_symbol_20MHz = ifft(freq_sc_20MHz,nfft_20MHz).*(nfft_20MHz/nfft_1dot4MHz);
        ofdm_symbol_with_cp_20MHz = [ofdm_symbol_20MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_20MHz];
        
        freq_ofdm_symbol_20MHz = fft(ofdm_symbol_20MHz,nfft_20MHz);
        
        
        if(use_fix_point)
            freq_ofdm_symbol_20MHz_aux = fi(freq_ofdm_symbol_20MHz,1,16,15);
            freq_ofdm_symbol_20MHz_aux = freq_ofdm_symbol_20MHz_aux.data;
        else
            freq_ofdm_symbol_20MHz_aux = freq_ofdm_symbol_20MHz;
        end
        
        
        oob_vector = [freq_ofdm_symbol_20MHz_aux(1:364); freq_ofdm_symbol_20MHz_aux(437:1100); freq_ofdm_symbol_20MHz_aux(1173:end)];
        
        oob_avg(1,winType) = oob_avg(1,winType) + (10*log10(sum(abs(oob_vector).^2)/length(oob_vector)));
        
    end
    
    fprintf(1,'window_type: %s - OOB: %1.2f [dB]\n',window_type_str,(oob_avg(1,winType)/numIter));
    
end
