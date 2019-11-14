clear all;close all;clc

nof_vphys = 12;

nof_slots = 2;

data_cnt = 0;
for vphy_id = 0:1:nof_vphys-1
    
    for slot_idx = 1:1:nof_slots
        
        fprintf(1,'vPHY ID: %d - slot #: %d - data: %d\n',vphy_id,slot_idx,data_cnt);
        
        data_cnt = mod(data_cnt+1,256);
        
    end
    
end