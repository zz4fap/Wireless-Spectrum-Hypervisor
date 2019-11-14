clear all;close all;clc

rng(12041988)

plot_enabled = false;

% Create a local random stream to be used by random number generators for repeatability.
hStr = RandStream('mt19937ar');

% ----------------------- Definitions --------------------------
SNR = 1000;

% modulation order M. (64QAM)
M = 4; %64;

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

numOfvPHYs = 4;
numDataSym = num_sc_6RB*numOfvPHYs;

% Instantiate channelizer.
numOfvPHYsIn20MHz = nfft_23dot04MHz/nfft_1dot4MHz;
channelizer = dsp.Channelizer(numOfvPHYsIn20MHz);

s_20MHz_100RB = zeros(nfft_23dot04MHz,1);

nBits = 0;

numIter = 1;

for iter=1:1:numIter
    
    msg = randi(hStr, [0 1], modOrd, numOfvPHYs*num_sc_6RB);
    modulated_signal = qammod(msg, M, 'InputType', 'bit', 'UnitAveragePower', true);
    %scatterplot(y);
    
    for vphy_idx = 0:1:numOfvPHYs-1
        
        % Calculate channel indexes for subcarrier mapping.
        channel_center_idx = vphy_idx*nfft_1dot4MHz;
        
        channel_idx_start = mod(channel_center_idx-(num_sc_6RB/2)+1,nfft_23dot04MHz);
        channel_idx_end = mod(channel_center_idx+(num_sc_6RB/2)+1,nfft_23dot04MHz);
        
        indexes = [channel_idx_start:1:channel_idx_start+(num_sc_6RB/2)-1 channel_center_idx+1:1:channel_idx_end];
        
        % Subcarrier mapping: Frequency domain multiplexing of four 1.4 MHz LTE vPHYs into 23.04 MHz BW.
        s_20MHz_100RB(indexes) = [modulated_signal(vphy_idx*num_sc_6RB+1:vphy_idx*num_sc_6RB+(num_sc_6RB/2)), 0, modulated_signal(vphy_idx*num_sc_6RB+(num_sc_6RB/2)+1:vphy_idx*num_sc_6RB + num_sc_6RB)];
        
        % -------------- 23.04 MHz Tx: 1 CP + 1 OFDM symbol ----------------------
        % Create OFDM symbol.
        ofdm_symbol_23dot04MHz = ifft(s_20MHz_100RB, nfft_23dot04MHz).*(nfft_23dot04MHz/nfft_1dot4MHz);
        % Adding CP.
        tx_ofdm_symbol_with_cp_23dot04MHz = [ofdm_symbol_23dot04MHz((end-(len_cp_20MHz-1)):end); ofdm_symbol_23dot04MHz];
        
    end
    
    
    figure;
    tx_23dor04MHz_signal = fft(ofdm_symbol_23dot04MHz,nfft_23dot04MHz);
    plot(0:1:nfft_23dot04MHz-1,20*log10(abs(tx_23dor04MHz_signal/(nfft_23dot04MHz/nfft_1dot4MHz))))
    title('Transmitted 20 MHz signal.')
    grid on;
    
    if(plot_enabled)
        figure(1);
        plot(0:1:nfft_23dot04MHz-1,abs(s_20MHz_100RB));
        xlim([0,nfft_23dot04MHz-1]);
        grid on
        title('Resource Grid (Frequency Domain)')
        
        figure(2)
        sig = (1/(nfft_23dot04MHz/nfft_1dot4MHz))*fft(ofdm_symbol_23dot04MHz,nfft_23dot04MHz );
        plot(0:1:nfft_23dot04MHz-1,abs([sig(((nfft_23dot04MHz/2)+1):end); sig(1:(nfft_23dot04MHz/2))]));
        xlim([0,nfft_23dot04MHz-1]);
        grid on
        title('FFT of the OFDM symbol')
    end
    
    % AWGN Channel.
    
    
    %Reception (Channelizer).
    rx_ofdm_symbol_with_cp_23dot04MHz = channelizer(tx_ofdm_symbol_with_cp_23dot04MHz);
    
    for vphy_idx = 1:1:numOfvPHYs
        
        % Retrieve OFDM symbol.
        rx_vphy_ofdm_symbol_with_cp = rx_ofdm_symbol_with_cp_23dot04MHz(:,vphy_idx);
        
        % Remove CP.
        rx_vphy_ofdm_symbol = rx_vphy_ofdm_symbol_with_cp(len_cp_1dot4MHz+1:end).';
        
        
        plot(abs(fft(rx_vphy_ofdm_symbol,   nfft_1dot4MHz)));
        
        a=1;
        
    end
    
    
    if(0)
        % Receiving vPHY at channel 0:
        n_idx = (-len_cp_20MHz:(length(tx_ofdm_symbol_with_cp_23dot04MHz)-len_cp_20MHz-1)).';
        channel = 0;
        sc_idx_offset = channel*128;
        vphy_rx_0 = tx_ofdm_symbol_with_cp_23dot04MHz.*exp(-1i.*2.*pi.*(sc_idx_offset).*n_idx./nfft_23dot04MHz);
        vphy_rx_0 = conv(vphy_rx_0, fir_coef);
        vphy_rx_0 = vphy_rx_0((len_fir_half+1):(end-len_fir_half));
        vphy_rx_0 = vphy_rx_0(1:(nfft_23dot04MHz/nfft_1dot4MHz):end);
        
        figure;
        plot(abs(fft(vphy_rx_0(len_cp_1dot4MHz+1:end),nfft_1dot4MHz)));
        a=1;
    end
    
    
    
    
    
end

% References

% [1] https://nl.mathworks.com/help/dsp/ref/dsp.channelizer-system-object.html
% [2] https://nl.mathworks.com/help/dsp/ref/dsp.channelsynthesizer-system-object.html
% [3] https://nl.mathworks.com/help/dsp/ref/dsp.channelizer.coeffschannelizersynthesizer.html


