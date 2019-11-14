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


for vphy_idx = 1:1:12
    vphy_cnt = vphy_idx;
    semilogy(SNR, vphy_ber(:,vphy_cnt), 'b', 'LineWidth', 1)
    if(vphy_idx==1)
       hold on; 
    end
end
semilogy(SNR, vphy_ber_single, 'ks--', 'LineWidth', 1)


ld = legend('vPHY #0', 'vPHY #1', 'vPHY #2', 'vPHY #3', 'vPHY #4', 'vPHY #5', 'vPHY #6', 'vPHY #7', 'vPHY #8', 'vPHY #9', 'vPHY #10', 'vPHY #11', 'Single PHY');
ld.FontSize = 10;
grid on;
hold off;
xlabel('SNR [dB]');
ylabel('Uncoded BER');
ylim([1e-6 0.5])
