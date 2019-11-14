clear all;close all;clc

% Definitions.
NFFT = 1536;

NFFT1dot4 = 128;

% Initialize vectors.
resource_grid = complex(zeros(1,NFFT),zeros(1,NFFT));

ifft_buffer = complex(zeros(1,NFFT),zeros(1,NFFT));

baseband = complex(zeros(1,NFFT),zeros(1,NFFT));

% Map upper part.
upper_part = complex(ones(1,NFFT1dot4/2),ones(1,NFFT1dot4/2));
resource_grid(1,NFFT-(NFFT1dot4/2)+1:end) = upper_part;

% Map lower part.
lower_part = 2*complex(ones(1,NFFT1dot4/2),ones(1,NFFT1dot4/2));
resource_grid(1,1:(NFFT1dot4/2)) = lower_part;

resource_grid(1,1) = 0;

figure;
plot(abs(resource_grid).^2);
title('Resource grid');

% Swap lower and upper parts.
ifft_buffer(1:NFFT/2) = resource_grid(NFFT/2+1:end);
ifft_buffer(NFFT/2+1:end) = resource_grid(1:NFFT/2);

% Apply IFFT.
ifft_signal = ifft(ifft_buffer,NFFT);

% Create frequency shifter.
ko = -NFFT/2;
n = 0:1:NFFT-1;
freq_offset = exp((1i.*2.*pi.*ko.*n)./NFFT);

% Translate signal into baseband.
translated_signal = ifft_signal.*freq_offset;

% FFt of translated signal.
translated_signal_fft = fft(translated_signal);

figure;
plot(abs(translated_signal_fft).^2)
title('Translated signal');

% Swap lower and upper parts.
baseband(1:NFFT/2) = translated_signal_fft(NFFT/2+1:end);
baseband(NFFT/2+1:end) = translated_signal_fft(1:NFFT/2);

figure;
plot(abs(baseband).^2)
title('Base-band signal');

