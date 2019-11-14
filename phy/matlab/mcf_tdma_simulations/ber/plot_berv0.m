clear all;close all;clc



load('mcf_tdma_ber_analysis_v5_single_phy_20190624T164159.mat');
vphy_ber_single_phy = vphy_ber;



load('mcf_tdma_ber_analysis_v5_20190624T154947.mat');
for vphy_idx = 1:1:numOfvPHYs
    vphy_ber_aux(:,vphy_idx) = vphy_ber(:,vphy_idx);
end


load('mcf_tdma_ber_analysis_v5_20190624T102841.mat');
for vphy_idx = 1:1:numOfvPHYs
    for snr_idx = 1:1:length(SNR)
        snr_value = SNR(snr_idx);
        snr_cnt = 1;
        if(snr_value == (22 + (snr_cnt-1)*2) && snr_cnt <= 3)
            vphy_ber(snr_idx, vphy_idx) = vphy_ber_aux(snr_cnt,vphy_idx);
            snr_cnt = snr_cnt + 1;
        end
    end
end


% bitsPerSubCarrier = 6;
% M = 2^bitsPerSubCarrier;    % Modulation alphabet
% subbandSize = 72;
% numFFT = 1536;
% EbNodB =  SNR - ( 10*log10(bitsPerSubCarrier) + 10*log10((numOfvPHYs*subbandSize)/numFFT) );
% ber_vector_ofdm_awgn_theoretical = berawgn(EbNodB, 'qam', M);




fdee_figure1 = figure('rend','painters','pos',[10 10 600 500]);
for vphy_idx = 1:1:numOfvPHYs
    semilogy(SNR, vphy_ber(:,vphy_idx), 'LineWidth', 1)
    if(vphy_idx == 1)
        hold on;
    end
end
semilogy(SNR, vphy_ber_single_phy, 'r', 'LineWidth', 1)

grid on;
hold off;
xlabel('SNR [dB]');
ylabel('Uncoded BER');
ylim([1e-6 0.5])