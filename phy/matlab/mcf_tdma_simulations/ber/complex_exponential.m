clear all;close all;clc


numOfvPHYs = 12;

% modulation order M. (64QAM)
M = 64;

% Constellation size = 2^modOrd.
modOrd = log2(M);

sc_spacing = 15e3;
freq_offset_1dot4MHz_in_20MHz = 128*15000; % vPHY frequency offset in Hz: 1.92 MHz

num_sc_per_RB = 12;
num_sc_6RB = 6*num_sc_per_RB;
num_sc_100RB = 100*num_sc_per_RB;

nfft_20MHz_standard = 2048;
len_cp_20MHz_standard = 144; % Fisrt OFDM symbol 160, subsequently ones, 144.
sampling_rate_20MHz_standard = 30.72e6;

nfft_23dot04MHz = 1536;
len_cp_20MHz = len_cp_20MHz_standard*nfft_23dot04MHz/nfft_20MHz_standard;
sampling_rate_20MHz = sampling_rate_20MHz_standard*nfft_23dot04MHz/nfft_20MHz_standard;

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;



k0 = nfft_1dot4MHz/2;
n_idx = 0:1:nfft_23dot04MHz-1;   %(-len_cp_20MHz:(length(noisy_tx_ofdm_symbol_with_cp_23dot04MHz_shifted)-len_cp_20MHz-1)).';
noisy_tx_ofdm_symbol_with_cp_23dot04MHz = exp(-1i.*2.*pi.*k0.*n_idx./nfft_23dot04MHz).';


%signal = real([noisy_tx_ofdm_symbol_with_cp_23dot04MHz noisy_tx_ofdm_symbol_with_cp_23dot04MHz]);
%plot(0:1:length(signal)-1, signal)





n_idx = 0:1:nfft_23dot04MHz-1;   %(-len_cp_20MHz:(length(noisy_tx_ofdm_symbol_with_cp_23dot04MHz_shifted)-len_cp_20MHz-1)).';
fundamental_wave = exp(1i.*2.*pi.*n_idx./nfft_23dot04MHz).';

figure(1);
plot(0:1:length(fundamental_wave)-1, real(fundamental_wave))


shifted_wave = zeros(nfft_23dot04MHz,1);
nl_idx = 0;
for n_idx=0:1:23040-1
    
    
    
    pos(n_idx + 1) = mod(nl_idx, nfft_23dot04MHz) + 1;
    shifted_wave(n_idx + 1) = fundamental_wave(pos(n_idx + 1));
    
    nl_idx = nl_idx + (-nfft_1dot4MHz/2);
end


figure(2);
plot(0:1:length(shifted_wave)-1, real(shifted_wave))

figure(3);
plot(0:1:length(shifted_wave)-1, imag(shifted_wave))

error = (abs(noisy_tx_ofdm_symbol_with_cp_23dot04MHz - shifted_wave));

%find(error>8e-14)
%find(error>0.1)
