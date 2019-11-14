clear all;close all;clc

clear all;close all;clc

rng(12041988)

% Create a local random stream to be used by random number generators for repeatability.
hStr = RandStream('mt19937ar');

% ----------------------- Definitions --------------------------
plot_enabled = true;

estimate_channel = true;

SNR = -10:2:20;

sigPowerdB = -45.166; % Average signal power in dB for a single vPHY transmitting.
sigPower_linear = 10^(sigPowerdB/10);
reqSNR = 10.^(SNR./10);
noisePower = sigPower_linear./reqSNR;

% modulation order M. (64QAM)
M = 4;

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

% Baseband FIR for 1.4MHz Rx.
fir_coef = fir1(512, 1.4e6/sampling_rate_20MHz).';
len_fir_half = (length(fir_coef)-1)/2;

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;

% Baseband FIR for 1.08 MHz Rx at 1.92 MHz.
fir_coef2 = fir1(512, (72*15000)/sampling_rate_1dot4MHz).';
len_fir_half2 = (length(fir_coef2)-1)/2;

numOfvPHYs = 12;
numDataSym = num_sc_6RB*numOfvPHYs;

% Instantiate channelizer.
numOfvPHYsIn20MHz = nfft_23dot04MHz/nfft_1dot4MHz;
channelizer = dsp.Channelizer(numOfvPHYsIn20MHz, 'StopbandAttenuation', 1*80, 'NumTapsPerBand', 10*12);
freqz(channelizer,'Fs', 23.04e6)
%fvtool(channelizer)
%c = coeffs(channelizer);
%f = centerFrequencies(channelizer, 23.04e6);

spectrumAnalyzer =  dsp.SpectrumAnalyzer('ShowLegend', true, 'NumInputPorts', 1, 'ChannelNames',{'Output'}, 'Title', 'Output of QMF');

% QAM Symbol mapper.
dataMapper = comm.RectangularQAMModulator('ModulationOrder', 2^modOrd, 'BitInput', true, 'NormalizationMethod', 'Average power');

% QAM Symbol demapping.
dataDemod = comm.RectangularQAMDemodulator('ModulationOrder', 2^modOrd, 'BitOutput', true, 'NormalizationMethod', 'Average power');

fdee_figure = figure('rend','painters','pos',[10 10 800 700]);
Fs = 23.04e6;         % Sampling frequency                          
L = nfft_23dot04MHz;             % Length of signal
f = Fs*(-((L/2)):1:(L/2)-1)   /L;
f = f/1e6;

s_20MHz_100RB_aux = zeros(nfft_23dot04MHz, 1);
for vphy_idx = 0:1:numOfvPHYs-1
    
    s_20MHz_100RB = zeros(nfft_23dot04MHz, 1);
    
    %% -------------------------- Tx Side -------------------------
    msg = randi([0 1], modOrd*numOfvPHYs*num_sc_6RB, 1);
    modulated_signal = dataMapper(msg);
    %scatterplot(modulated_signal);
    
    % Calculate channel indexes for subcarrier mapping.
    channel_center_idx = vphy_idx*nfft_1dot4MHz;
    
    channel_idx_start = mod(channel_center_idx-(num_sc_6RB/2)+1,nfft_23dot04MHz);
    channel_idx_end = mod(channel_center_idx+(num_sc_6RB/2)+1,nfft_23dot04MHz);
    
    indexes = [channel_idx_start:1:channel_idx_start+(num_sc_6RB/2)-1 channel_center_idx+1:1:channel_idx_end];
    
    % Subcarrier mapping: Frequency domain multiplexing of four 1.4 MHz LTE vPHYs into 23.04 MHz BW.
    s_20MHz_100RB(indexes) = [modulated_signal(vphy_idx*num_sc_6RB+1:vphy_idx*num_sc_6RB+(num_sc_6RB/2)); 0; modulated_signal(vphy_idx*num_sc_6RB+(num_sc_6RB/2)+1:vphy_idx*num_sc_6RB + num_sc_6RB)];
    
    s_20MHz_100RB_aux(indexes) = [modulated_signal(vphy_idx*num_sc_6RB+1:vphy_idx*num_sc_6RB+(num_sc_6RB/2)); 0; modulated_signal(vphy_idx*num_sc_6RB+(num_sc_6RB/2)+1:vphy_idx*num_sc_6RB + num_sc_6RB)];
    
    % -------------- 23.04 MHz Tx: 1 CP + 1 OFDM symbol ----------------------
    % Create OFDM symbol.
    ofdm_symbol_23dot04MHz = ifft((s_20MHz_100RB), nfft_23dot04MHz);
    % Adding CP.
    tx_ofdm_symbol_with_cp_23dot04MHz = [ofdm_symbol_23dot04MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_23dot04MHz];
    
    tx_23dor04MHz_signal = fftshift(fft(ofdm_symbol_23dot04MHz, nfft_23dot04MHz));
    plot(f, 20*log10(abs(tx_23dor04MHz_signal).^2), 'LineWidth', 1);
    %plot(0:1:nfft_23dot04MHz-1, 20*log10(abs(tx_23dor04MHz_signal/(nfft_23dot04MHz/nfft_1dot4MHz))), 'LineWidth', 1);
    if(vphy_idx == 0)
        hold on
    end

end

% -------------- 23.04 MHz Tx: 1 CP + 1 OFDM symbol ----------------------
% Create OFDM symbol.
ofdm_symbol_23dot04MHz_aux = ifft((s_20MHz_100RB_aux), nfft_23dot04MHz);
% Adding CP.
tx_ofdm_symbol_with_cp_23dot04MHz_aux = [ofdm_symbol_23dot04MHz_aux((end-(len_cp_20MHz-1)):end); ofdm_symbol_23dot04MHz_aux];


xlim([f(1), f(end)]);
grid on;
hold off
xlabel('Frequency [MHz]')
ylabel('20*log10(|Y|^2)');
legend('vPHY #0','vPHY #1','vPHY #2','vPHY #3','vPHY #4','vPHY #5','vPHY #6','vPHY #7','vPHY #8','vPHY #9','vPHY #10','vPHY #11');

%% -------------------------- Rx Side -------------------------
rx_ofdm_symbol_with_cp_23dot04MHz = channelizer(tx_ofdm_symbol_with_cp_23dot04MHz_aux);

for vphy_idx = 1:1:numOfvPHYs
    
    % Retrieve OFDM symbol.
    rx_vphy_ofdm_symbol_with_cp = rx_ofdm_symbol_with_cp_23dot04MHz(:,vphy_idx);
    
    % Remove CP.
    rx_vphy_ofdm_symbol = rx_vphy_ofdm_symbol_with_cp(len_cp_1dot4MHz+1:end);
    
    %plot(0:1:nfft_1dot4MHz-1, abs(fft(rx_vphy_ofdm_symbol, nfft_1dot4MHz)));
    
    if(plot_enabled)
            figure(vphy_idx);
            plot(0:1:nfft_1dot4MHz-1, abs(fft(rx_vphy_ofdm_symbol, nfft_1dot4MHz)));
            grid on;
            title(sprintf('OFDM symbol in vPHY #%d',vphy_idx-1))
    end
    
    rx_freq_domain_ofdm_symbol = (fft(rx_vphy_ofdm_symbol, nfft_1dot4MHz));
    
    % Demapping data symbols.
    rx_data_indexes = [93:1:128 2:1:37];
    rx_data_symbols = rx_freq_domain_ofdm_symbol(rx_data_indexes);


    
    
    
end












% References
% [1] https://nl.mathworks.com/help/dsp/ref/dsp.channelizer-system-object.html
% [2] https://nl.mathworks.com/help/dsp/ref/dsp.channelsynthesizer-system-object.html
% [3] https://nl.mathworks.com/help/dsp/ref/dsp.channelizer.coeffschannelizersynthesizer.html


