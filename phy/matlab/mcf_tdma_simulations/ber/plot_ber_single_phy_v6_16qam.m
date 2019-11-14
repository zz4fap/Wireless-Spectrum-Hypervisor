clear all;close all;clc

load('mcf_tdma_ber_analysis_single_phy_v8_20190705T201131.mat');
vphy_ber_single1 = vphy_ber;
SNR_single1 = SNR;



load('mcf_tdma_ber_analysis_v11_20190706T181808.mat'); % -30 a -16
vphy_ber_aux1 = vphy_ber;
SNR_aux1 = SNR;

load('mcf_tdma_ber_analysis_v11_20190706T072236.mat'); % -14 a -6
vphy_ber_aux2 = vphy_ber;
SNR_aux2 = SNR;

load('mcf_tdma_ber_analysis_v11_20190706T184324.mat'); % -4 a 10
vphy_ber_aux3 = vphy_ber;
SNR_aux3 = SNR;

vphy_ber_single = [vphy_ber_single1];
SNR_single = [SNR_single1];

vphy_ber = [vphy_ber_aux1; vphy_ber_aux2; vphy_ber_aux3];
SNR = [SNR_aux1 SNR_aux2 SNR_aux3];


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
semilogy(SNR_single, vphy_ber_single, 'ks--', 'LineWidth', 1)
semilogy(SNR, ber_vector_ofdm_awgn_theoretical, 'r.--', 'LineWidth', 1)


ld = legend('vPHY #0', 'vPHY #1', 'vPHY #2', 'vPHY #3', 'vPHY #4', 'vPHY #5', 'vPHY #6', 'vPHY #7', 'vPHY #8', 'vPHY #9', 'vPHY #10', 'vPHY #11', 'Single PHY', 'Theoretical', 'Location', 'southwest');
ld.FontSize = 10;
grid on;
hold off;
xlabel('SNR [dB]');
ylabel('Uncoded BER');
ylim([1e-6 0.6])
xlim([-30 10])
txt = '16QAM';
text(3.5,2e-1,txt,'FontSize',12)

