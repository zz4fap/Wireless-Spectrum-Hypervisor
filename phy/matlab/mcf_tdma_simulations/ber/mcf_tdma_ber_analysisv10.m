clear all;close all;clc

rng(12041988)

% Create a local random stream to be used by random number generators for repeatability.
hStr = RandStream('mt19937ar');

% ----------------------- Definitions --------------------------
plot_enabled = false;

estimate_channel = true;

SNR = 10000; %-10:2:20;

sigPowerdB = -45.166; % Average signal power in dB for a single vPHY transmitting.
sigPower_linear = 10^(sigPowerdB/10);
reqSNR = 10.^(SNR./10);
noisePower = sigPower_linear./reqSNR;

% modulation order M. (64QAM)
M = 64;

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
channelizer = dsp.Channelizer(numOfvPHYsIn20MHz, 'StopbandAttenuation', 80, 'NumTapsPerBand', 12);
%freqz(channelizer,'Fs', 23.04e6)
%fvtool(channelizer)
%c = coeffs(channelizer);
%f = centerFrequencies(channelizer, 23.04e6);

%spectrumAnalyzer =  dsp.SpectrumAnalyzer('ShowLegend', true, 'NumInputPorts', 1, 'ChannelNames',{'Output'}, 'Title', 'Output of QMF');

s_20MHz_100RB = zeros(nfft_23dot04MHz, 1);

% Generate BPSK Pilots.
pilot_msg = randi(hStr, [0 1], num_sc_6RB, log2(2));
pilotMod = comm.RectangularQAMModulator('ModulationOrder', 2, 'BitInput', true, 'NormalizationMethod', 'Average power');
pilotDemod = comm.RectangularQAMDemodulator('ModulationOrder', 2, 'BitOutput', true, 'NormalizationMethod', 'Average power');
pilot_signal = pilotMod(pilot_msg);
%scatterplot(pilot_signal);

% QAM Symbol mapper.
dataMapper = comm.RectangularQAMModulator('ModulationOrder', 2^modOrd, 'BitInput', true, 'NormalizationMethod', 'Average power');

% QAM Symbol demapping.
dataDemod = comm.RectangularQAMDemodulator('ModulationOrder', 2^modOrd, 'BitOutput', true, 'NormalizationMethod', 'Average power');

numIter_vector = 1e3*ones(1, 16);

vphy_ber = zeros(length(SNR),numOfvPHYs);
for snr_idx=1:1:length(SNR)
    
    numIter = numIter_vector(snr_idx);
    
    fprintf(1,'SNR: %1.2f [dB] - numIter: %d\n', SNR(snr_idx), numIter);
    
    channel_estimate = zeros(num_sc_6RB, numOfvPHYs);
    nBits = 0;
    for iter=1:1:numIter
        
        msg = randi([0 1], modOrd*numOfvPHYs*num_sc_6RB, 1);
        modulated_signal = dataMapper(msg);
        %scatterplot(modulated_signal);
        
        % Number of bits sent over one vPHY.
        nBits = nBits + modOrd*num_sc_6RB;
        
        % For now we send one Pilot symbol and one Data symbol.
        for ofdm_symbol_idx = 1:1:2
            
            for vphy_idx = 0:1:numOfvPHYs-1
                
                %% -------------------------- Tx Side -------------------------
                % Calculate channel indexes for subcarrier mapping.
                channel_center_idx = vphy_idx*nfft_1dot4MHz;
                
                channel_idx_start = mod(channel_center_idx-(num_sc_6RB/2)+1,nfft_23dot04MHz);
                channel_idx_end = mod(channel_center_idx+(num_sc_6RB/2)+1,nfft_23dot04MHz);
                
                indexes = [channel_idx_start:1:channel_idx_start+(num_sc_6RB/2)-1 channel_center_idx+1:1:channel_idx_end];
                
                % Subcarrier mapping: Frequency domain multiplexing of four 1.4 MHz LTE vPHYs into 23.04 MHz BW.
                if(ofdm_symbol_idx==1)
                    s_20MHz_100RB(indexes) = [pilot_signal(1:num_sc_6RB/2); 0; pilot_signal((num_sc_6RB/2)+1:end)];
                else
                    s_20MHz_100RB(indexes) = [modulated_signal(vphy_idx*num_sc_6RB+1:vphy_idx*num_sc_6RB+(num_sc_6RB/2)); 0; modulated_signal(vphy_idx*num_sc_6RB+(num_sc_6RB/2)+1:vphy_idx*num_sc_6RB + num_sc_6RB)];
                end
                
                % -------------- 23.04 MHz Tx: 1 CP + 1 OFDM symbol ----------------------
                % Create OFDM symbol.
                ofdm_symbol_23dot04MHz = ifft((s_20MHz_100RB), nfft_23dot04MHz);
                % Adding CP.
                tx_ofdm_symbol_with_cp_23dot04MHz = [ofdm_symbol_23dot04MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_23dot04MHz];
                
            end
            
            if(plot_enabled && ofdm_symbol_idx>1)
                spectrumAnalyzer(tx_ofdm_symbol_with_cp_23dot04MHz);
            end
            if(plot_enabled)
                if(ofdm_symbol_idx>1)
                    figure(1);
                    plot(0:1:nfft_23dot04MHz-1, abs(s_20MHz_100RB));
                    xlim([0, nfft_23dot04MHz-1]);
                    grid on
                    title('Resource Grid (Frequency Domain)')
                    
                    figure(2);
                    tx_23dor04MHz_signal = fft(ofdm_symbol_23dot04MHz,nfft_23dot04MHz);
                    plot(0:1:nfft_23dot04MHz-1, 20*log10(abs(tx_23dor04MHz_signal/(nfft_23dot04MHz/nfft_1dot4MHz))))
                    title('FFT of the 23.04 MHz OFDM symbol');
                    xlim([0, nfft_23dot04MHz-1]);
                    grid on;
                end
            end
            
            %% --------------------- Wireless Channel ---------------------
            % AWGN Channel.
            noise = sqrt(noisePower(snr_idx)/2)* (randn(size(tx_ofdm_symbol_with_cp_23dot04MHz)) + 1i*randn(size(tx_ofdm_symbol_with_cp_23dot04MHz)));
            noisy_tx_ofdm_symbol_with_cp_23dot04MHz = tx_ofdm_symbol_with_cp_23dot04MHz + noise;
            
            %% -------------------------- Rx Side -------------------------
            % Reception (Channelizer).
            rx_ofdm_symbol_with_cp_23dot04MHz = channelizer(noisy_tx_ofdm_symbol_with_cp_23dot04MHz);
            
            if(plot_enabled && ofdm_symbol_idx>1)
                spectrumAnalyzer(rx_ofdm_symbol_with_cp_23dot04MHz);
            end
            
            for vphy_idx = 1:1:numOfvPHYs
                
                % Retrieve OFDM symbol.
                rx_vphy_ofdm_symbol_with_cp = rx_ofdm_symbol_with_cp_23dot04MHz(:,vphy_idx);
                
                % Remove CP.
                rx_vphy_ofdm_symbol = rx_vphy_ofdm_symbol_with_cp(len_cp_1dot4MHz+1:end);
                
                %plot(0:1:nfft_1dot4MHz-1, abs(fft(rx_vphy_ofdm_symbol, nfft_1dot4MHz)));
                
                if(plot_enabled)
                    if(ofdm_symbol_idx > 1)
                        figure(3);
                        plot(0:1:nfft_1dot4MHz-1, abs(fft(rx_vphy_ofdm_symbol, nfft_1dot4MHz)));
                        grid on;
                        title(sprintf('OFDM symbol in vPHY #%d',vphy_idx-1))
                    end
                end
                
                rx_freq_domain_ofdm_symbol = (fft(rx_vphy_ofdm_symbol, nfft_1dot4MHz));
                
                % Demapping data symbols.
                rx_data_indexes = [93:1:128 2:1:37];
                rx_data_symbols = rx_freq_domain_ofdm_symbol(rx_data_indexes);
                
                % Estimate channel response.
                if(ofdm_symbol_idx==1)
                    
                    if(estimate_channel)
                        channel_estimate(:,vphy_idx) = rx_data_symbols./(pilot_signal);
                    else
                        channel_estimate(:,vphy_idx) = ones(num_sc_6RB, 1);
                    end
                    
                    
                    
                    
                    %                     figure(1)
                    %                     plot(abs(channel_estimate(:,vphy_idx)))
                    %                     figure(2)
                    %                     plot(phase(channel_estimate(:,vphy_idx)))
                    
                end
                
                % Equalize signal.
                eq_rx_data_symbols = rx_data_symbols./channel_estimate(:,vphy_idx);
                %scatterplot(eq_rx_data_symbols);
                
                % Demodulate pilot signal.
                if(ofdm_symbol_idx == 1)
                    demodulated_pilot = pilotDemod(eq_rx_data_symbols);
                    %isequal(demodulated_pilot,pilot_msg);
                else
                    % Demodulate data signal.
                    demodulated_data = dataDemod(eq_rx_data_symbols);
                    data_msg = msg((vphy_idx-1)*num_sc_6RB*log2(M)+1:vphy_idx*num_sc_6RB*log2(M));
                    vphy_ber(snr_idx, vphy_idx) = vphy_ber(snr_idx, vphy_idx) + biterr(demodulated_data,data_msg);
                end
                
            end
            
        end
        
        if(mod(iter,100) == 0)
            fprintf(1, 'Multi-PHYs - SNR: %1.2f [dB] - iter: %d\n', SNR(snr_idx), iter);
        end        
        
    end
    
end

EbNodB =  SNR - ( 10*log10(modOrd) + 10*log10((128)/nfft_23dot04MHz) );
ber_vector_ofdm_awgn_theoretical = berawgn(EbNodB, 'qam', M);

for snr_idx=1:1:length(SNR)
    fprintf(1,'---------------- SNR: %1.2f [dB] ----------------\n', SNR(snr_idx));
    for vphy_idx = 1:1:numOfvPHYs
        vphy_ber(snr_idx, vphy_idx) = vphy_ber(snr_idx, vphy_idx)/nBits;
        fprintf(1,'\tvPHY #%d - Simu BER: %1.4e - Theory BER: %1.4e\n', vphy_idx-1, vphy_ber(snr_idx, vphy_idx), ber_vector_ofdm_awgn_theoretical(snr_idx));
    end
    fprintf(1,'-------------------------------------------------\n');
end

if(1)
    % Get timestamp for saving files.
    timeStamp = datestr(now,30);
    
    % Save workspace to .mat file.
    fileName = sprintf('mcf_tdma_ber_analysis_v10_%s.mat', timeStamp);
    save(fileName);
end

if(0)
    fdee_figure1 = figure('rend','painters','pos',[10 10 600 500]);
    for vphy_idx = 1:1:numOfvPHYs
        semilogy(SNR,vphy_ber(:,vphy_idx).', 'b', 'LineWidth', 1)
        if(vphy_idx == 1)
            hold on;
        end
    end
    semilogy(SNR, ber_vector_ofdm_awgn_theoretical, 'r', 'LineWidth', 1)
    grid on;
    hold off;
    xlabel('SNR [dB]');
    ylabel('Uncoded BER');
    ylim([1e-6 0.5])
end

% References
% [1] https://nl.mathworks.com/help/dsp/ref/dsp.channelizer-system-object.html
% [2] https://nl.mathworks.com/help/dsp/ref/dsp.channelsynthesizer-system-object.html
% [3] https://nl.mathworks.com/help/dsp/ref/dsp.channelizer.coeffschannelizersynthesizer.html


