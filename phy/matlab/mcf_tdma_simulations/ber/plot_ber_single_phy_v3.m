clear all;close all;clc

load('mcf_tdma_ber_analysis_single_phy_v5_20190628T213134.mat')
vphy_ber_single = vphy_ber;
SNR_single = SNR;

load('mcf_tdma_ber_analysis_v10_20190629T092030.mat');
vphy_ber_aux = vphy_ber;
SNR_aux = SNR;


load('mcf_tdma_ber_analysis_v10_20190629T093449.mat');

vphy_ber = [vphy_ber_aux; vphy_ber];
SNR = [SNR_aux SNR ];



fdee_figure1 = figure('rend','painters','pos',[10 10 700 600]);
semilogy(SNR, vphy_ber_single, 'k', 'LineWidth', 1)
hold on;
for vphy_idx = 1:1:12
    vphy_cnt = vphy_idx;
    semilogy(SNR, vphy_ber(:,vphy_cnt), 'b', 'LineWidth', 1)
end


ld = legend('Single PHY', 'Multiple vPHYs: 12', 'Theoretical QAM');
ld.FontSize = 10;
grid on;
hold off;
xlabel('SNR [dB]');
ylabel('Uncoded BER');
ylim([1e-6 0.5])
