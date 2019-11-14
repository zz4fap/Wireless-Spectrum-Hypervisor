clear all;close all;clc

% Definitions.
RADIO_FFT_LEN = 1536;

VPHY_FFT_LEN = 128;

VPHY_NOF_PRB = 6;

NOF_RE = 12;

channel = 4;

dc_pos = channel*VPHY_FFT_LEN;

init_pos = dc_pos - (VPHY_NOF_PRB*NOF_RE/2);

%% Initialize vectors.
resource_grid = complex(zeros(1,RADIO_FFT_LEN),zeros(1,RADIO_FFT_LEN));
ifft_buffer = complex(zeros(1,RADIO_FFT_LEN),zeros(1,RADIO_FFT_LEN));
baseband = complex(zeros(1,RADIO_FFT_LEN),zeros(1,RADIO_FFT_LEN));

%% Map symbols into resource grid.
lower_part = complex(ones(1,(VPHY_NOF_PRB*NOF_RE/2)),ones(1,(VPHY_NOF_PRB*NOF_RE/2)));
upper_part = 2*complex(ones(1,(VPHY_NOF_PRB*NOF_RE/2)),ones(1,(VPHY_NOF_PRB*NOF_RE/2)));

adj_init_pos = init_pos+1;
resource_grid(adj_init_pos:adj_init_pos+(VPHY_NOF_PRB*NOF_RE/2)-1) = lower_part;
resource_grid(adj_init_pos+(VPHY_NOF_PRB*NOF_RE/2)+1:adj_init_pos+(VPHY_NOF_PRB*NOF_RE/2)+(VPHY_NOF_PRB*NOF_RE/2)) = upper_part;

figure;
plot(0:1:RADIO_FFT_LEN-1, abs(resource_grid).^2);
title('Resource grid');

%% Swap lower and upper parts.
ifft_buffer(1:(RADIO_FFT_LEN/2)+1) = resource_grid((RADIO_FFT_LEN/2):end);
ifft_buffer((RADIO_FFT_LEN/2)+2:end) = resource_grid(1:(RADIO_FFT_LEN/2)-1);

% Apply IFFT.
ifft_signal = ifft(ifft_buffer,RADIO_FFT_LEN);

% Create frequency shifter.
ko = -(768+dc_pos+1);
n = 0:1:RADIO_FFT_LEN-1;
freq_offset = exp((1i.*2.*pi.*ko.*n)./RADIO_FFT_LEN);

% Translate signal into baseband.
translated_signal = ifft_signal.*freq_offset;

% FFt of translated signal.
translated_signal_fft = fft(translated_signal);

figure;
plot(0:1:RADIO_FFT_LEN-1, abs(translated_signal_fft).^2)
title('Translated signal');

% Swap lower and upper parts.
baseband(1:(RADIO_FFT_LEN/2)-1) = translated_signal_fft((RADIO_FFT_LEN/2)+2:end);
baseband((RADIO_FFT_LEN/2):end) = translated_signal_fft(1:(RADIO_FFT_LEN/2)+1);

figure;
plot(0:1:RADIO_FFT_LEN-1, abs(baseband).^2)
title('Base-band signal');

