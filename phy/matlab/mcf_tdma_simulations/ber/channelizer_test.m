clear all;close all;clc

numOfvPHYsIn20MHz = 12;

len_cp_20MHz = 108;

NFFT = 1536;

channelizer = dsp.Channelizer(numOfvPHYsIn20MHz, 'StopbandAttenuation', 4*80, 'NumTapsPerBand', 10*12);
freqz(channelizer,'Fs', 23.04e6)

spectrumAnalyzer =  dsp.SpectrumAnalyzer('ShowLegend', true, 'NumInputPorts', 1, 'ChannelNames',{'Output'}, 'Title', 'Output of QMF');


% Signal.
signal = [1, zeros(1, NFFT-1 + len_cp_20MHz)].';

% Channelizer.
channl_out = channelizer(signal);


rx_ofdm_symbol_with_cp_23dot04MHz_aux = reshape(channl_out, 1, 12*137);
spectrumAnalyzer(rx_ofdm_symbol_with_cp_23dot04MHz_aux.');