clear all;close all;clc

NFFT = 1536;

PRB20 = 100;

NFFT1dot4 = 128;

PRB1dot4 = 6;

NRE = 12;

channel = 0;

offset_vector = [0 3 0 3];

dc_pos = channel*NFFT1dot4;

for l=0:1:3
    
    fidx = offset_vector(l+1);
    
    init_pos = dc_pos - (((PRB1dot4*NRE)/2) + (NRE/2)) + fidx;

    if(init_pos < 0)
        init_pos = init_pos + NFFT;
    end
    
    for idx=0:1:(2*PRB1dot4)-1
        
        init_pos = mod(init_pos + NRE/2, NFFT);
        
        init_pos = init_pos + l*NFFT;
        
        % Do CRS insertion into Resourge grid here.
        if(l==0)
            fprintf(1,'idx: %d - CRS pos: %d - CRS next pos: %d\n',idx,init_pos,(init_pos+NFFT+offset_vector(l+2)));
        else
            fprintf(1,'idx: %d - CRS pos: %d\n',idx,init_pos);
        end        
        
        if(idx == 5)
            init_pos = init_pos + 1;
        end
        
    end
    
    fprintf(1,'\n\n')
    
end