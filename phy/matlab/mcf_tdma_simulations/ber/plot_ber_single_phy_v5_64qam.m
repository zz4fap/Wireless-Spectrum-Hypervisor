clear all;close all;clc

load('mcf_tdma_ber_analysis_single_phy_v8_20190702T194158.mat');
vphy_ber_single = vphy_ber;
SNR_single = SNR;

load('mcf_tdma_ber_analysis_v11_20190703T051130.mat');
vphy_ber_aux1 = vphy_ber;
SNR_aux1 = SNR;

load('mcf_tdma_ber_analysis_v11_20190703T012522.mat');
vphy_ber_aux2 = vphy_ber;
SNR_aux2 = SNR;

load('mcf_tdma_ber_analysis_v11_20190703T020227.mat');
vphy_ber_aux3 = vphy_ber;
SNR_aux3 = SNR;

load('mcf_tdma_ber_analysis_v11_20190703T070753.mat');
vphy_ber_aux4 = vphy_ber;
SNR_aux4 = SNR;

load('mcf_tdma_ber_analysis_v11_20190702T195317.mat');
vphy_ber_aux5 = vphy_ber;
SNR_aux5 = SNR;

load('mcf_tdma_ber_analysis_v11_20190703T071159.mat');
vphy_ber_aux6 = vphy_ber;
SNR_aux6 = SNR;

vphy_ber = [vphy_ber_aux1; vphy_ber_aux2; vphy_ber_aux3; vphy_ber_aux4; vphy_ber_aux5; vphy_ber_aux6];
SNR = [SNR_aux1 SNR_aux2 SNR_aux3 SNR_aux4 SNR_aux5 SNR_aux6];


EbNodB =  SNR - ( 10*log10(modOrd) + 10*log10((num_sc_6RB)/nfft_23dot04MHz) );
ber_vector_ofdm_awgn_theoretical = berawgn(EbNodB, 'qam', M);



fdee_figure1 = figure('rend','painters','pos',[10 10 700 600]);
for vphy_idx = 1:1:12
    vphy_cnt = vphy_idx;
    semilogy(SNR, vphy_ber(:,vphy_cnt), 'b', 'LineWidth', 1)
    if(vphy_idx==1)
       hold on; 
    end
end
semilogy(SNR, vphy_ber_single, 'ks--', 'LineWidth', 1)
semilogy(SNR, ber_vector_ofdm_awgn_theoretical, 'r.--', 'LineWidth', 1)


ld = legend('vPHY #0', 'vPHY #1', 'vPHY #2', 'vPHY #3', 'vPHY #4', 'vPHY #5', 'vPHY #6', 'vPHY #7', 'vPHY #8', 'vPHY #9', 'vPHY #10', 'vPHY #11', 'Single PHY', 'Theoretical', 'Location', 'southwest');
ld.FontSize = 10;
grid on;
hold off;
xlabel('SNR [dB]');
ylabel('Uncoded BER');
ylim([1e-6 0.5])
txt = '64QAM';
text(11,2e-1,txt,'FontSize',12)

