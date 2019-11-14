clear all;close all;clc

NFFT = 1536;

PRB20 = 100;

NFFT1dot4 = 128;

PRB1dot4 = 6;

NRE = 12;

channel = 0;

freq_offset = 3;

dc_pos = channel*NFFT1dot4;

init_pos = dc_pos - ((PRB1dot4*NRE)/2) + freq_offset;

if(init_pos < 0)
   init_pos = init_pos + NFFT;
end

for l=0:1:3
    
    for idx=0:1:(2*PRB1dot4)-1
        
        fprintf(1,'idx: %d - CRS pos: %d\n',idx,init_pos);
        
        % Do CRS insertion into Resourge grid here.
        
        init_pos = mod(init_pos + NRE/2, NFFT);
        
        if(idx == 5)
            init_pos = init_pos + 1;
        end
        
    end
    
end