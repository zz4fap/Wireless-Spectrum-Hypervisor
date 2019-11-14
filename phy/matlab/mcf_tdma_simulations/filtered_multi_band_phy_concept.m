clear all;close all;clc

rng(12041988)

% Create a local random stream to be used by random number generators for repeatability.
hStr = RandStream('mt19937ar');

plot_figures = false;

% Constellation size = 2^modOrd.
modOrd = 2;
% Create QPSK mod-demod objects.
hMod = modem.pskmod('M', 2^modOrd, 'SymbolOrder', 'gray', 'InputType', 'bit', 'phase', pi/(2^modOrd));
hDemod = modem.pskdemod(hMod);

subcarrier_spacing = 15e3;
freq_offset_1dot4MHz_in_23dot04MHz = 6e6;

num_sc_per_RB = 12;
num_sc_6RB = 6*num_sc_per_RB;
num_sc_100RB = 100*num_sc_per_RB;

nfft_30dot72MHz_standard = 2048;
len_cp_30dot72MHz_standard = 144; % nornmal CP for symbols different from 0.
sampling_rate_30dot72MHz_standard = 30.72e6;

nfft_23dot04MHz = 1536;
len_cp_23dot04MHz = len_cp_30dot72MHz_standard*nfft_23dot04MHz/nfft_30dot72MHz_standard;
sampling_rate_23dot04MHz = sampling_rate_30dot72MHz_standard*nfft_23dot04MHz/nfft_30dot72MHz_standard;

nfft_1dot4MHz = 128;
len_cp_1dot4MHz = len_cp_30dot72MHz_standard*nfft_1dot4MHz/nfft_30dot72MHz_standard;
sampling_rate_1dot4MHz = sampling_rate_30dot72MHz_standard*nfft_1dot4MHz/nfft_30dot72MHz_standard;

% Baseband FIR for 1.4 MHz
filter_type = 1;
if(filter_type == 1)
    toneOffset = 2.5;        % Tone offset or excess bandwidth (in subcarriers)
    L = 129;                 % Filter length (=filterOrder+1), odd

    numDataCarriers = 72;    % number of data subcarriers in subband
    halfFilt = floor(L/2);
    n = -halfFilt:halfFilt;
    
    % Sinc function prototype filter
    pb = sinc((numDataCarriers+2*toneOffset).*n./nfft_23dot04MHz);
    
    % Sinc truncation window
    w = (0.5*(1+cos(2*pi.*n/(L-1)))).^0.6;
    
    % Normalized lowpass filter coefficients
    fnum = (pb.*w)/sum(pb.*w);
    
    % Filter impulse response
    h = fvtool(fnum, 'Analysis', 'impulse', 'NormalizedFrequency', 'off', 'Fs', sampling_rate_23dot04MHz);
    h.CurrentAxes.XLabel.String = 'Time (\mus)';
    h.FigureToolbar = 'off';
    
    % Use dsp filter objects for filtering
    filter = dsp.FIRFilter('Structure', 'Direct form symmetric', 'Numerator', fnum);
    
    prototypeFilter = filter.Numerator.';
    prototypeFilter_length = length(prototypeFilter);
    prototypeFilter_half_length = (length(prototypeFilter)-1)/2;
else
    prototypeFilter = fir1(len_cp_23dot04MHz, 1.4e6/sampling_rate_23dot04MHz).';
    prototypeFilter_length = length(prototypeFilter);
    prototypeFilter_half_length = (length(prototypeFilter)-1)/2;
    %wvtool(prototypeFilter)
end

% --- Symbols frequency to modulate subcarrier (data/pilot/pss/sss/etc) of 1.4MHz BW = 6RB ----
nof_subbands = 2;

msg1 = randi(hStr, [0 1], modOrd, num_sc_6RB);
signal1_1dot4MHz_6RB = modulate(hMod, msg1).';
signal1_1dot4MHz_6RB = nof_subbands*signal1_1dot4MHz_6RB;

msg2 = randi(hStr, [0 1], modOrd, num_sc_6RB);
signal2_1dot4MHz_6RB = modulate(hMod, msg2).';
signal2_1dot4MHz_6RB = nof_subbands*signal2_1dot4MHz_6RB;

% ------------------------------------------------------------------------
% Generate two 1.4 MHz LTE signals (at +6 MHz and -6 MHz) through frequency domain multiplexing as a single 20 MHz LTE transmitter.
sc_idx_offset_6MHz = freq_offset_1dot4MHz_in_23dot04MHz/subcarrier_spacing;
% subcarrier idx of negative 6 MHz offset 1.4 MHz LTE in 20 MHz LTE
sc_sp_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
sc_ep_n6MHz = (num_sc_100RB/2)-sc_idx_offset_6MHz+(num_sc_6RB/2);
% subcarrier idx of positive 6 MHz offset 1.4 MHz LTE in 20 MHz LTE
sc_sp_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz-(num_sc_6RB/2)+1;
sc_ep_p6MHz = (num_sc_100RB/2)+sc_idx_offset_6MHz+(num_sc_6RB/2);

%% -------------- First Subband modulation ------------------------
% Frequency domain multiplexing of the 1st 1.4 MHz LTE into subcarrier of 20 MHz LTE
signal_23dot04MHz_100RB = zeros(num_sc_100RB,1);
signal_23dot04MHz_100RB(sc_sp_n6MHz:sc_ep_n6MHz) = signal1_1dot4MHz_6RB;

% Frequency domain multiplexing of two 1.4 MHz LTE into subcarrier of 20 MHz LTE
figure; 
plot(abs(signal_23dot04MHz_100RB)); 
title('1st 6 RB into 100 RB for 20 MHz Tx'); 
grid on;

freq_sc_23dot04MHz = single([signal_23dot04MHz_100RB(((num_sc_100RB/2)+1):end); zeros(nfft_23dot04MHz-num_sc_100RB,1); signal_23dot04MHz_100RB(1:(num_sc_100RB/2))]);
ofdm1_symbol_23dot04MHz = ifft(freq_sc_23dot04MHz,nfft_23dot04MHz).*(nfft_23dot04MHz/nfft_1dot4MHz);
ofdm1_symbol_with_cp_23dot04MHz = [ofdm1_symbol_23dot04MHz((end-(len_cp_23dot04MHz-1)):end); ofdm1_symbol_23dot04MHz];

ko = -1*sc_idx_offset_6MHz;
n = (0:prototypeFilter_length-1).';
bandFilter = prototypeFilter.*exp(1i.*2.*pi.*ko.*n./nfft_23dot04MHz);
signal1_filterOut = conv(bandFilter,ofdm1_symbol_with_cp_23dot04MHz);
%wvtool(imag(ofdm1_symbol_23dot04MHz))
%wvtool(imag(bandFilter))
wvtool(imag(signal1_filterOut))

h1 = figure;
single_precision_fft = fft(ofdm1_symbol_23dot04MHz,nfft_23dot04MHz);
avg_power = 10*log10(sum(abs(single_precision_fft(1:360)))/360);
plot(10*log10(abs(single_precision_fft)))
titleStr = sprintf('1st SB Single precision - Avg. noise floor %1.2f [dBW]',avg_power);
title(titleStr)
xlabel('IFFT points')
ylabel('10*log10(|Y|)')
grid on
%saveas(h1,'single_precision.png')

figure;
plot(10*log10(abs(fft(bandFilter,nfft_23dot04MHz))))
title('1st Subband'); 
xlabel('IFFT points')
ylabel('10*log10(|Y|)')
grid on

%% -------------- Second Subband modulation ------------------------
% Frequency domain multiplexing of two 1.4 MHz LTE into subcarrier of 20 MHz LTE
signal_23dot04MHz_100RB = zeros(num_sc_100RB,1);
signal_23dot04MHz_100RB(sc_sp_p6MHz:sc_ep_p6MHz) = signal2_1dot4MHz_6RB;

% Frequency domain multiplexing of two 1.4 MHz LTE into subcarrier of 20 MHz LTE
figure; 
plot(abs(signal_23dot04MHz_100RB)); 
title('2nd 6 RB into 100 RB for 20 MHz Tx'); 
grid on;

freq_sc_23dot04MHz = single([signal_23dot04MHz_100RB(((num_sc_100RB/2)+1):end); zeros(nfft_23dot04MHz-num_sc_100RB,1); signal_23dot04MHz_100RB(1:(num_sc_100RB/2))]);
ofdm2_symbol_23dot04MHz = ifft(freq_sc_23dot04MHz,nfft_23dot04MHz).*(nfft_23dot04MHz/nfft_1dot4MHz);
ofdm2_symbol_with_cp_23dot04MHz = [ofdm2_symbol_23dot04MHz((end-(len_cp_23dot04MHz-1)):end); ofdm2_symbol_23dot04MHz];

ko = sc_idx_offset_6MHz;
n = (0:prototypeFilter_length-1).';
bandFilter = prototypeFilter.*exp(1i.*2.*pi.*ko.*n./nfft_23dot04MHz);
signal2_filterOut = conv(bandFilter,ofdm2_symbol_with_cp_23dot04MHz);
%wvtool(imag(ofdm1_symbol_23dot04MHz))
%wvtool(imag(bandFilter))
%wvtool(imag(signal2_filterOut))

h2 = figure;
single_precision_fft = fft(ofdm2_symbol_23dot04MHz,nfft_23dot04MHz);
avg_power = 10*log10(sum(abs(single_precision_fft(1:360)))/360);
plot(10*log10(abs(single_precision_fft)))
titleStr = sprintf('2nd SB Single precision - Avg. noise floor %1.2f [dBW]',avg_power);
title(titleStr)
xlabel('IFFT points')
ylabel('10*log10(|Y|)')
grid on
%saveas(h1,'single_precision.png')

figure;
plot(10*log10(abs(fft(bandFilter,nfft_23dot04MHz))))
title('2nd Subband'); 
xlabel('IFFT points')
ylabel('10*log10(|Y|)')
grid on

%% ----------------- Create transmitted signal ---------------------------
%txSig = (ofdm2_symbol_with_cp_23dot04MHz + ofdm1_symbol_with_cp_23dot04MHz)/2;
txSig = (signal1_filterOut + signal2_filterOut)/2;

yRxPadded = [txSig; zeros(2*nfft_23dot04MHz-numel(txSig),1)];

% Perform FFT and downsample by 2
RxSymbols2x = (fft(yRxPadded));
RxSymbols = RxSymbols2x(1:2:end);
plot(10*log10(abs(RxSymbols).^2))

% Plot power spectral density (PSD) per subband
hFig = figure;
axis([-0.5 0.5 -175 11]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
[psd,f] = periodogram(signal1_filterOut, rectwin(length(signal1_filterOut)), nfft_23dot04MHz*2, 1, 'centered');
plot(f,10*log10(psd));
[psd,f] = periodogram(signal2_filterOut, rectwin(length(signal2_filterOut)), nfft_23dot04MHz*2, 1, 'centered');
plot(f,10*log10(psd));
hold off

h4 = figure;
single_precision_fft = fft(yRxPadded,2*nfft_23dot04MHz);
avg_power = 10*log10(sum(abs(single_precision_fft(1:360)))/360);
plot(10*log10(abs(single_precision_fft)))
titleStr = sprintf('Tx signal - Avg. noise floor %1.2f [dBW]',avg_power);
title(titleStr)
xlabel('IFFT points')
ylabel('10*log10(|Y|)')
grid on

%% --------------------- Receive signal -------------------------------
n = (-(len_cp_23dot04MHz+prototypeFilter_length-1):(length(txSig)-(len_cp_23dot04MHz+prototypeFilter_length-1)-1)).';

% Receive the 1st 1.4 MHz LTE signal at an offset of -6 MHz.
ko = sc_idx_offset_6MHz;
rx_n6MHz = txSig.*exp(1i.*2.*pi.*ko.*n./nfft_23dot04MHz);
rx_n6MHz = conv(rx_n6MHz, prototypeFilter);
rx_n6MHz = rx_n6MHz((prototypeFilter_half_length+1):(end-prototypeFilter_half_length));
rx_n6MHz = rx_n6MHz(1:(nfft_23dot04MHz/nfft_1dot4MHz):end);

rx_n6MHz_padded = [rx_n6MHz; zeros(2*nfft_1dot4MHz-numel(rx_n6MHz),1)];

RxSymbols2x = (fft(rx_n6MHz_padded));
RxSymbols = RxSymbols2x(1:2:end);

% Plot power spectral density (PSD) per subband
hFig = figure;
[psd,f] = periodogram(rx_n6MHz_padded, rectwin(length(rx_n6MHz_padded)), nfft_1dot4MHz*2, 1, 'centered');
plot(f,10*log10(psd));
grid on
axis([-0.5 0.5 -175 11]);
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')

