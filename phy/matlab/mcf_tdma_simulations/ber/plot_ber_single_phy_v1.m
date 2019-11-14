clear all;close all;clc

load('mcf_tdma_ber_analysis_single_phy_v3_20190628T141049.mat');
vphy_ber_single = vphy_ber;

EbNodB =  SNR - ( 10*log10(modOrd) + 10*log10((128)/nfft_23dot04MHz) );
ber_vector_ofdm_awgn_theoretical = berawgn(EbNodB, 'qam', M);


load('mcf_tdma_ber_analysis_v8_20190628T141503.mat');


fdee_figure1 = figure('rend','painters','pos',[10 10 600 500]);
semilogy(SNR, vphy_ber_single, 'k', 'LineWidth', 1)
hold on;
for vphy_idx = 1:1:1
    vphy_cnt = 10;
    semilogy(SNR, vphy_ber(:,vphy_cnt), 'b', 'LineWidth', 1)
end
semilogy(SNR, ber_vector_ofdm_awgn_theoretical, 'r', 'LineWidth', 1)

legend('Single PHY', 'Multiple vPHYs: 12', 'Theoretical QAM');
grid on;
hold off;
xlabel('SNR [dB]');
ylabel('Uncoded BER');
ylim([1e-6 0.5])