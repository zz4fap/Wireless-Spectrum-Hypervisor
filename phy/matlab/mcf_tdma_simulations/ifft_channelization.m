clear all;close all;clc

channel = 0;

NFFT = 1536;

NFFT1dot4 = 128;

expected_center_frequencies = [0 128 256 384 512 640 768 896 1024 1152 1280 1408];

error = 0;
for idx=0:1:NFFT1dot4-1
    
    ifft_idx1 = mod(channel*NFFT1dot4 + NFFT - (NFFT1dot4/2) + idx, NFFT);
    
    ifft_idx2 = mod(channel*NFFT1dot4 - (NFFT1dot4/2) + idx, NFFT);
    
    ifft_idx3 = channel*NFFT1dot4 - (NFFT1dot4/2) + idx;
    if(ifft_idx3 < 0)
        ifft_idx3 = ifft_idx3 + NFFT;
    end
    
    error = error + (ifft_idx1-ifft_idx3);
    
    fprintf(1,'channel idx: %d - ifft_idx3: %d\n',ifft_idx1,ifft_idx3);
    
end

fprintf(1,'error: %f\n',error);
fprintf(1,'channel beginning: %d - channel end: %d\n', mod(channel*NFFT1dot4 - (NFFT1dot4/2), NFFT  ), mod(channel*NFFT1dot4 + (NFFT1dot4/2) - 1, NFFT  ));



