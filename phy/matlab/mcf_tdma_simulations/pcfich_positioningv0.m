clear all;close all;clc

radio_fft_len = 1536;

PRB20 = 100;

vphy_fft_len = 128;

PRB1dot4 = 6;

NRE = 12;

channel = 11;

pcfich_pos_vector = [1 2 4 5 19 20 22 23 37 38 40 41 55 56 58 59];

dc_pos = channel*vphy_fft_len;

init_pos = dc_pos - ( ((PRB1dot4*NRE)/2) );

if(init_pos < 0)
    init_pos = init_pos + radio_fft_len;
end

% PCFICH groups
for group_idx=0:1:3
    
    for reg_idx=0:1:3
        
        pos = group_idx*4 + reg_idx;
        
        idx = mod(init_pos + pcfich_pos_vector(pos+1), radio_fft_len);
        
        if(pcfich_pos_vector(pos+1) >= ((PRB1dot4*NRE)/2))
            idx = idx + 1;
        end
        
        % Do PCFICH insertion into Resourge grid here.
        fprintf(1,'idx: %d - PCFICH pos: %d\n',reg_idx,idx);
        
    end
    
    fprintf(1,'-----------------------\n\n')
    
end