clear all;close all;clc

SNR = -10:2:14;

% modulation order M. (64QAM)
M = 64;

% Constellation size = 2^modOrd.
modOrd = log2(M);

nfft_23dot04MHz = 1536;
num_sc_6RB = 72;

EbNodB =  SNR - ( 10*log10(modOrd) + 10*log10((num_sc_6RB)/nfft_23dot04MHz) );
ber_vector_ofdm_awgn_theoretical = berawgn(EbNodB, 'qam', M);




EbNo = 10.^(EbNodB/10);
k = 6;
M = 64;
x = sqrt(3*k*EbNo/(M-1));
Pb = (4/k)*(1-1/sqrt(M))*(1/2)*erfc(x/sqrt(2));




semilogy(SNR, ber_vector_ofdm_awgn_theoretical);
hold on
semilogy(SNR, Pb);
xlim([SNR(1) SNR(end)])
xticks([-10:4:14])
grid on
legend('Matlab', 'Site')