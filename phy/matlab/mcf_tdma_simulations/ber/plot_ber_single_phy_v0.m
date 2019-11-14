clear all;close all;clc

load('mcf_tdma_ber_analysis_v5_single_phy_v1_20190627T104926.mat')
vphy_ber_singlev1 = vphy_ber;
SNR_ = SNR;

load('mcf_tdma_ber_analysis_v5_single_phy_20190624T164159.mat');




bitsPerSubCarrier = 6;
M = 2^bitsPerSubCarrier;    % Modulation alphabet
subbandSize = 72;
numFftTx = 1536;
numFftRx = 128;
EbNodB =  SNR - ( 10*log10(bitsPerSubCarrier) + 10*log10((numOfvPHYs*subbandSize)/numFftTx) - 10*log10((numOfvPHYs*subbandSize)/numFftRx) );
ber_vector_ofdm_awgn_theoretical = berawgn(EbNodB, 'qam', M);




fdee_figure1 = figure('rend','painters','pos',[10 10 600 500]);
for vphy_idx = 1:1:numOfvPHYs
    semilogy(SNR, vphy_ber(:,vphy_idx), 'k', 'LineWidth', 1)
    if(vphy_idx == 1)
        hold on;
    end
end
semilogy(SNR_, vphy_ber_singlev1, 'g', 'LineWidth', 1)
semilogy(SNR, ber_vector_ofdm_awgn_theoretical, 'r', 'LineWidth', 1)

legend('Single PHY', 'Single PHY v1', 'Theoretical OFDM');
grid on;
hold off;
xlabel('SNR [dB]');
ylabel('Uncoded BER');
ylim([1e-6 0.5])