clear all;close all;clc

rng(12041988)

% Create a local random stream to be used by random number generators for repeatability.
hStr = RandStream('mt19937ar');

% ----------------------- Definitions --------------------------
plot_enabled = false;

estimate_channel = true;

% modulation order M. (64QAM)
M = 4;

numOfvPHYs = 12;

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
%fir_coef = fir1(2*512, 1.4e6/sampling_rate_20MHz).';
cuttoff_freq = ((sampling_rate_20MHz/2)/numOfvPHYs)/(sampling_rate_20MHz/2);
fir_coef = fir1(2*1024, cuttoff_freq).';
len_fir_half = (length(fir_coef)-1)/2;

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_20MHz_standard*nfft_1dot4MHz/nfft_20MHz_standard;

numDataSym = num_sc_6RB*numOfvPHYs;

s_20MHz_100RB = zeros(nfft_23dot04MHz, 1);

% QAM Symbol mapper.
dataMapper = comm.RectangularQAMModulator('ModulationOrder', 2^modOrd, 'BitInput', true, 'NormalizationMethod', 'Average power');

% QAM Symbol demapping.
dataDemod = comm.RectangularQAMDemodulator('ModulationOrder', 2^modOrd, 'BitOutput', true, 'NormalizationMethod', 'Average power');

mer = comm.MER('MinimumMEROutputPort', true, 'XPercentileMEROutputPort', true,'XPercentileValue', 90, 'SymbolCountOutputPort', true);

numIter = 1e3;

MERdB_vphy = zeros(1, numOfvPHYs);
signal_error = zeros(1, numOfvPHYs);
for iter=1:1:numIter
    
    msg = randi([0 1], modOrd*num_sc_6RB, 1);
    modulated_signal = dataMapper(msg);
    %scatterplot(modulated_signal);
    
    freq_sc_1dor4MHz = [0; modulated_signal(((num_sc_6RB/2)+1):end); zeros(nfft_1dot4MHz-num_sc_6RB-1,1); modulated_signal(1:(num_sc_6RB/2))];
    ofdm_symbol_1dot4MHz = ifft(freq_sc_1dor4MHz, nfft_1dot4MHz);
    ofdm_symbol_with_cp_1dot4MHz = [ofdm_symbol_1dot4MHz((end-(len_cp_1dot4MHz-1)):end); ofdm_symbol_1dot4MHz];
    
    % For now we send one Pilot symbol and one Data symbol.
    for ofdm_symbol_idx = 1:1:1
        
        for vphy_idx = 0:1:numOfvPHYs-1
            
            %% -------------------------- Tx Side -------------------------
            % Calculate channel indexes for subcarrier mapping.
            channel_center_idx = vphy_idx*nfft_1dot4MHz;
            
            channel_idx_start = mod(channel_center_idx-(num_sc_6RB/2)+1, nfft_23dot04MHz);
            channel_idx_end = mod(channel_center_idx+(num_sc_6RB/2)+1, nfft_23dot04MHz);
            
            indexes = [channel_idx_start:1:channel_idx_start+(num_sc_6RB/2)-1 channel_center_idx+1:1:channel_idx_end];
            
            % Subcarrier mapping: Frequency domain multiplexing of four 1.4 MHz LTE vPHYs into 23.04 MHz BW.
            if(ofdm_symbol_idx==1)
                s_20MHz_100RB(indexes) = [modulated_signal(1:(num_sc_6RB/2)); 0; modulated_signal((num_sc_6RB/2)+1:end)];
            end
        end
        
        % -------------- 23.04 MHz Tx: 1 CP + 1 OFDM symbol ----------------------
        % Create OFDM symbol.
        ofdm_symbol_23dot04MHz = ifft((s_20MHz_100RB), nfft_23dot04MHz).*(nfft_23dot04MHz/nfft_1dot4MHz);
        % Adding CP.
        tx_ofdm_symbol_with_cp_23dot04MHz = [ofdm_symbol_23dot04MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_23dot04MHz];
        
        %% -------------------------- Rx Side -------------------------
        
        for vphy_idx = 1:1:numOfvPHYs
            
            k0 = (vphy_idx-1).*nfft_1dot4MHz;
            n_idx = (-len_cp_20MHz:(length(tx_ofdm_symbol_with_cp_23dot04MHz)-len_cp_20MHz-1)).';
            
            % Retrieve OFDM symbol.
            vphy_rx_aux = tx_ofdm_symbol_with_cp_23dot04MHz.*exp(-1i.*2.*pi.*k0.*n_idx./nfft_23dot04MHz);
            
            %plot(abs(fft(vphy_rx_aux(len_cp_20MHz+1:end))))
            
            vphy_rx_aux = conv(vphy_rx_aux, fir_coef);
            vphy_rx_aux = vphy_rx_aux((len_fir_half+1):(end-len_fir_half));
            rx_vphy_ofdm_symbol_with_cp = vphy_rx_aux(1:(nfft_23dot04MHz/nfft_1dot4MHz):end);
            
            signal_error(vphy_idx) = signal_error(vphy_idx) + (sum(abs(ofdm_symbol_with_cp_1dot4MHz - rx_vphy_ofdm_symbol_with_cp).^2)/length(rx_vphy_ofdm_symbol_with_cp));
            
            
            rxsym = fft(rx_vphy_ofdm_symbol_with_cp(len_cp_1dot4MHz+1:end), nfft_1dot4MHz);
            rxsym = rxsym([93:1:128 2:1:37]);
            
            
            [MERdB, MinMER, PercentileMER, NumSym] = mer(modulated_signal, rxsym);
            MERdB_vphy(vphy_idx) = MERdB_vphy(vphy_idx) + MERdB;
            reset(mer);
        end
        
    end
    
end

for vphy_idx = 1:1:numOfvPHYs
    signal_error(vphy_idx) = signal_error(vphy_idx)./numIter;
    MERdB_vphy(vphy_idx) = MERdB_vphy(vphy_idx)./numIter;
    fprintf(1, 'M: %d - vPHY #%d - MSE: %1.4e - MER: %1.4f [dB]\n', M, vphy_idx, signal_error(vphy_idx), MERdB_vphy(vphy_idx));
end

if(0)
    % Get timestamp for saving files.
    timeStamp = datestr(now,30);
    
    % Save workspace to .mat file.
    fileName = sprintf('mcf_tdma_mse_analysis_v1_%s.mat', timeStamp);
    save(fileName);
end

% References
% [1] https://nl.mathworks.com/help/dsp/ref/dsp.channelizer-system-object.html
% [2] https://nl.mathworks.com/help/dsp/ref/dsp.channelsynthesizer-system-object.html
% [3] https://nl.mathworks.com/help/dsp/ref/dsp.channelizer.coeffschannelizersynthesizer.html


