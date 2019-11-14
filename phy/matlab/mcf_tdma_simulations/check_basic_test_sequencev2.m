clear all;close all;clc

nof_vphys = 4;

nof_slots = 4;

MAX_VALUE = 256;

specific_vphy = 0;

data_cnt = 0;

numIter = 16;
for iter=1:1:numIter
    
    for vphy_id = 0:1:nof_vphys-1
        
        for slot_idx = 1:1:nof_slots
            
            if(vphy_id == specific_vphy)
                fprintf(1,'%d,\t',data_cnt);
            end
            
            data_cnt = mod(data_cnt+1,MAX_VALUE);
            
        end
        
    end
    
    fprintf(1,'\n');
    
end