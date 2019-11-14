clear all;close all;clc

lower_idx_start = 0; %564;
lower_idx_end = 599;

upper_idx_start = 600;
upper_idx_end = 635;

%lower_idx_end - lower_idx_start + 1
%upper_idx_end - upper_idx_start + 1
%upper_idx_end - lower_idx_start + 1

for symb_idx=0:1:13
    
    %fprintf(1,'idx: %d\n',(lower_idx_start + 1200*symb_idx))
    
    cnt = 0;
    for sub_idx=0:1:72
    
        cnt = cnt + 1;
        
        if(sub_idx==36)
            value = 0;
        else
            value = 1;
        end
        
        fprintf(1,'cnt: %d - idx: %d - symbol: %d\n',cnt,(lower_idx_start + 1200*symb_idx + sub_idx),value)
    
    end
    
    fprintf(1,'\n\n\n');
end


