clear all;close all;clc

radio_fft_len = 1536;

PRB20 = 100;

vphy_fft_len = 128;

PRB1dot4 = 6;

NRE = 12;

channel = 0;

pdcch_k_vector = [4 5 6 7 61 62 64 65 24 25 26 27 48 49 50 51 12 13 14 15 67 68 70 71 32 33 34 35 40 41 42 43 0 1 2 3];

pdcch_l_vector = [1 1 1 1 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1];

dc_pos = channel*vphy_fft_len;

init_pos = dc_pos - ( ((PRB1dot4*NRE)/2) );

if(init_pos < 0)
    init_pos = init_pos + radio_fft_len;
end

% PDCCH groups
for group_idx=0:1:8
    
    fprintf(1,'------- REG: %d --------\n',group_idx);
    
    for reg_idx=0:1:3
        
        pos = group_idx*4 + reg_idx;
        
        idx = mod(init_pos + pdcch_k_vector(pos+1), radio_fft_len);
        
        if(pdcch_k_vector(pos+1) >= ((PRB1dot4*NRE)/2))
            idx = idx + 1;
        end
        
        idx = idx + pdcch_l_vector(pos+1)*radio_fft_len;
        
        % Do PDCCH insertion into Resourge grid here.
        fprintf(1,'idx: %d - PDCCH pos: %d\n',reg_idx,idx);
        
    end
    
    fprintf(1,'-----------------------\n\n')
    
end