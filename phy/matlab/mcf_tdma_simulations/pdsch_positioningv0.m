clear all;close all;clc;

radio_fft_len = 1536;

PRB20 = 100;

vphy_fft_len = 128;

PRB1dot4 = 6;

NRE = 12;

nof_symbols_in_slot = 7;

channel = 0;

%% ---------------------------- PDSCH Mapping -----------------------------
dc_pos = channel*vphy_fft_len;

init_pos = mod(dc_pos - PRB1dot4*NRE/2, radio_fft_len);

for s = 0:1:1 % slot counter
    
    for l = 0:1:6 % ofdm symbol within slot counter
        
        lp = l + s*nof_symbols_in_slot;
        fprintf(1,'OFDM symbol #%d\n',lp);
        
        for n = 0:1:PRB1dot4-1 % resource block counter.
            
            pos = mod(init_pos + n*NRE, radio_fft_len);
            
            if(n >= 3) 
                pos = pos + 1;
            end
       
            pos = pos + lp*radio_fft_len;
            
            fprintf(1,'\tpos: %d\n',pos);
            
        end
    end
end