#! /usr/bin/python

import sys,os,re

reg_data_width = 32


input_hex_file = ''
input_axi_addr = ''
len_size = ''

for input_con in sys.argv:
    if re.search('.hex',input_con):
        input_hex_file = input_con
    #if re.search('addr=',input_con):
    #    input_axi_addr = input_con[5:]
    if re.search('len=0x',input_con):
        len_size = int(input_con[6:],16)
    elif re.search('len=',input_con):
        len_size = input_con[4:]
    input_axi_addr = sys.argv[2]

#-------------------------------------------------------------------
# modify model/lpddr4_model/base_test.sv


#--------------------------------------------------------------------
# UMCTL2 MAPPING REGISTER
#ADDRMAP0   = '00000018'  
#ADDRMAP1   = '00080808' 
#ADDRMAP2   = '00000000' 
#ADDRMAP3   = '00000000' 
#ADDRMAP4   = '00001f1f' 
#ADDRMAP5   = '070f0707' 
#ADDRMAP6   = '07070707' 
#ADDRMAP7   = '00000f07' 
#ADDRMAP8   = '00003f3f' 
#ADDRMAP9   = '07070707' 
#ADDRMAP10  = '07070707' 
#ADDRMAP11  = '00000007' 

ADDRMAP0   = '00000008'  
ADDRMAP1   = '00000204' 
ADDRMAP2   = '02040000' 
ADDRMAP3   = '00050406' 
ADDRMAP4   = '00001f1f' 
ADDRMAP5   = '0a080309' 
ADDRMAP6   = '09080608'
ADDRMAP7   = '00000f07'
ADDRMAP8   = '00000000' 
ADDRMAP9   = '09090b0b' 
ADDRMAP10  = '0b0a0602' 
ADDRMAP11  = '0000000a' 
#--------------------------------------------------------------------
# change 0x to bin
UMCTL2_ADDRMAP0  = bin(int(ADDRMAP0 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP1  = bin(int(ADDRMAP1 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP2  = bin(int(ADDRMAP2 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP3  = bin(int(ADDRMAP3 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP4  = bin(int(ADDRMAP4 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP5  = bin(int(ADDRMAP5 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP6  = bin(int(ADDRMAP6 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP7  = bin(int(ADDRMAP7 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP8  = bin(int(ADDRMAP8 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP9  = bin(int(ADDRMAP9 ,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP10 = bin(int(ADDRMAP10,16))[2:].zfill(int(reg_data_width))
UMCTL2_ADDRMAP11 = bin(int(ADDRMAP11,16))[2:].zfill(int(reg_data_width))

# inverted order for python
UMCTL2_ADDRMAP0  = UMCTL2_ADDRMAP0[::-1] 
UMCTL2_ADDRMAP1  = UMCTL2_ADDRMAP1[::-1] 
UMCTL2_ADDRMAP2  = UMCTL2_ADDRMAP2[::-1] 
UMCTL2_ADDRMAP3  = UMCTL2_ADDRMAP3[::-1] 
UMCTL2_ADDRMAP4  = UMCTL2_ADDRMAP4[::-1] 
UMCTL2_ADDRMAP5  = UMCTL2_ADDRMAP5[::-1] 
UMCTL2_ADDRMAP6  = UMCTL2_ADDRMAP6[::-1] 
UMCTL2_ADDRMAP7  = UMCTL2_ADDRMAP7[::-1] 
UMCTL2_ADDRMAP8  = UMCTL2_ADDRMAP8[::-1] 
UMCTL2_ADDRMAP9  = UMCTL2_ADDRMAP9[::-1] 
UMCTL2_ADDRMAP10 = UMCTL2_ADDRMAP10[::-1]
UMCTL2_ADDRMAP11 = UMCTL2_ADDRMAP11[::-1]

#--------------------------------------------------------------------
#AXI to SDRAM address mapping
# Note: +1 for select in python
map_dch_bit0             = UMCTL2_ADDRMAP0 [16 : 20+1]
map_cs_bit1              = UMCTL2_ADDRMAP0 [8  : 12+1] 
map_cs_bit0              = UMCTL2_ADDRMAP0 [0  : 4+1] 
map_bank_b2              = UMCTL2_ADDRMAP1 [16 : 21+1] 
map_bank_b1              = UMCTL2_ADDRMAP1 [8  : 13+1] 
map_bank_b0              = UMCTL2_ADDRMAP1 [0  : 5+1] 
map_col_b5               = UMCTL2_ADDRMAP2 [24 : 27+1] 
map_col_b4               = UMCTL2_ADDRMAP2 [16 : 19+1] 
map_col_b3               = UMCTL2_ADDRMAP2 [8  : 12+1] 
map_col_b2               = UMCTL2_ADDRMAP2 [0  : 3+1] 
map_col_b9               = UMCTL2_ADDRMAP3 [24 : 28+1] 
map_col_b8               = UMCTL2_ADDRMAP3 [16 : 20+1] 
map_col_b7               = UMCTL2_ADDRMAP3 [8  : 12+1] 
map_col_b6               = UMCTL2_ADDRMAP3 [0  : 4+1] 
map_col_addr_shift       = UMCTL2_ADDRMAP4 [31 : ] 
map_col_b11              = UMCTL2_ADDRMAP4 [8  : 12+1] 
map_col_b10              = UMCTL2_ADDRMAP4 [0  : 4+1] 
map_row_b11              = UMCTL2_ADDRMAP5 [24 : 27+1] 
map_row_b2_10            = UMCTL2_ADDRMAP5 [16 : 19+1] 
map_row_b1               = UMCTL2_ADDRMAP5 [8  : 11+1] 
map_row_b0               = UMCTL2_ADDRMAP5 [0  : 3+1] 
map_lpddr34_3gb_6gb_12gb = UMCTL2_ADDRMAP6 [29 : 31+1] 
map_row_b15              = UMCTL2_ADDRMAP6 [24 : 27+1] 
map_row_b14              = UMCTL2_ADDRMAP6 [16 : 19+1] 
map_row_b13              = UMCTL2_ADDRMAP6 [8  : 11+1] 
map_row_b12              = UMCTL2_ADDRMAP6 [0  : 3+1] 
map_row_b17              = UMCTL2_ADDRMAP7 [8  : 11+1] 
map_row_b16              = UMCTL2_ADDRMAP7 [0  : 3+1] 
map_bg_b1                = UMCTL2_ADDRMAP8 [8  : 13+1] 
map_bg_b0                = UMCTL2_ADDRMAP8 [0  : 5+1] 
map_row_b5               = UMCTL2_ADDRMAP9 [24 : 27+1] 
map_row_b4               = UMCTL2_ADDRMAP9 [16 : 19+1] 
map_row_b3               = UMCTL2_ADDRMAP9 [8  : 11+1] 
map_row_b2               = UMCTL2_ADDRMAP9 [0  : 3+1] 
map_row_b9               = UMCTL2_ADDRMAP10[24 : 27+1] 
map_row_b8               = UMCTL2_ADDRMAP10[16 : 19+1] 
map_row_b7               = UMCTL2_ADDRMAP10[8  : 11+1] 
map_row_b6               = UMCTL2_ADDRMAP10[0  : 3+1] 
map_cid_b1               = UMCTL2_ADDRMAP11[16 : 20+1]  
map_cid_b0               = UMCTL2_ADDRMAP11[8  : 12+1]  
map_row_b10              = UMCTL2_ADDRMAP11[0  : 3+1]  


map_dch_bit0             =  map_dch_bit0[::-1] 
map_cs_bit1              =  map_cs_bit1[::-1] 
map_cs_bit0              =  map_cs_bit0[::-1] 
map_bank_b2              =  map_bank_b2[::-1] 
map_bank_b1              =  map_bank_b1[::-1] 
map_bank_b0              =  map_bank_b0[::-1] 
map_col_b5               =  map_col_b5[::-1] 
map_col_b4               =  map_col_b4[::-1] 
map_col_b3               =  map_col_b3[::-1] 
map_col_b2               =  map_col_b2[::-1] 
map_col_b9               =  map_col_b9[::-1] 
map_col_b8               =  map_col_b8[::-1] 
map_col_b7               =  map_col_b7[::-1] 
map_col_b6               =  map_col_b6[::-1] 
map_col_addr_shift       =  map_col_addr_shift[::-1] 
map_col_b11              =  map_col_b11[::-1] 
map_col_b10              =  map_col_b10[::-1] 
map_row_b11              =  map_row_b11[::-1] 
map_row_b2_10            =  map_row_b2_10[::-1] 
map_row_b1               =  map_row_b1[::-1] 
map_row_b0               =  map_row_b0[::-1] 
map_lpddr34_3gb_6gb_12gb =  map_lpddr34_3gb_6gb_12gb[::-1] 
map_row_b15              =  map_row_b15[::-1] 
map_row_b14              =  map_row_b14[::-1] 
map_row_b13              =  map_row_b13[::-1] 
map_row_b12              =  map_row_b12[::-1] 
map_row_b17              =  map_row_b17[::-1] 
map_row_b16              =  map_row_b16[::-1] 
map_bg_b1                =  map_bg_b1[::-1] 
map_bg_b0                =  map_bg_b0[::-1] 
map_row_b5               =  map_row_b5[::-1] 
map_row_b4               =  map_row_b4[::-1] 
map_row_b3               =  map_row_b3[::-1] 
map_row_b2               =  map_row_b2[::-1] 
map_row_b9               =  map_row_b9[::-1] 
map_row_b8               =  map_row_b8[::-1] 
map_row_b7               =  map_row_b7[::-1] 
map_row_b6               =  map_row_b6[::-1] 
map_cid_b1               =  map_cid_b1[::-1] 
map_cid_b0               =  map_cid_b0[::-1] 
map_row_b10              =  map_row_b10[::-1] 



#--------------------------------------------------------------------
# AXI to HIF mapping (base addr + register config value)
def AXItoHIF(AXI_ADDR):
    global cs0   
    global b2, b1, b0    
    global r17, r16, r15, r14, r13, r12, r11, r10, r9, r8, r7, r6, r5, r4, r3, r2, r1, r0 
    global c11, c10, c9, c8, c7, c6, c5, c4, c3, c2, c1, c0
    global cs, bank, row, column

    HIF_ADDR = (bin(int(AXI_ADDR ,16))[2:].zfill(int(33))[:-6] + '0000').zfill(int(37))
    HIF_ADDR = HIF_ADDR[::-1]
    
    cs0   =  HIF_ADDR[6 +int(map_cs_bit0,2)]
    
    b2    =  HIF_ADDR[4 +int(map_bank_b2,2)] 
    b1    =  HIF_ADDR[3 +int(map_bank_b1,2)] 
    b0    =  HIF_ADDR[2 +int(map_bank_b0,2)] 
    
    
    if map_row_b17 == '11111111111111111111111111111111'[0:4]:
        r17   =  '0' 
    else:
        r17   =  HIF_ADDR[23+int(map_row_b17,2)] 
    r16   =  HIF_ADDR[22+int(map_row_b16,2)] 
    r15   =  HIF_ADDR[21+int(map_row_b15,2)] 
    r14   =  HIF_ADDR[20+int(map_row_b14,2)] 
    r13   =  HIF_ADDR[19+int(map_row_b13,2)] 
    r12   =  HIF_ADDR[18+int(map_row_b12,2)] 
    r11   =  HIF_ADDR[17+int(map_row_b11,2)] 
    if map_row_b2_10 == '1111':
        r10   =  HIF_ADDR[16+int(map_row_b10,2)] 
        r9    =  HIF_ADDR[15+int(map_row_b9 ,2)] 
        r8    =  HIF_ADDR[14+int(map_row_b8 ,2)] 
        r7    =  HIF_ADDR[13+int(map_row_b7 ,2)] 
        r6    =  HIF_ADDR[12+int(map_row_b6 ,2)] 
        r5    =  HIF_ADDR[11+int(map_row_b5 ,2)] 
        r4    =  HIF_ADDR[10+int(map_row_b4 ,2)] 
        r3    =  HIF_ADDR[9 +int(map_row_b3 ,2)] 
        r2    =  HIF_ADDR[8 +int(map_row_b2 ,2)] 
    else:
        r10   =  HIF_ADDR[16+int(map_row_b2_10,2)] 
        r9    =  HIF_ADDR[15+int(map_row_b2_10,2)] 
        r8    =  HIF_ADDR[14+int(map_row_b2_10,2)] 
        r7    =  HIF_ADDR[13+int(map_row_b2_10,2)] 
        r6    =  HIF_ADDR[12+int(map_row_b2_10,2)] 
        r5    =  HIF_ADDR[11+int(map_row_b2_10,2)] 
        r4    =  HIF_ADDR[10+int(map_row_b2_10,2)] 
        r3    =  HIF_ADDR[9 +int(map_row_b2_10,2)] 
        r2    =  HIF_ADDR[8 +int(map_row_b2_10,2)] 
    r1    =  HIF_ADDR[7 +int(map_row_b1 ,2)] 
    r0    =  HIF_ADDR[6 +int(map_row_b0 ,2)] 
    
    if map_col_b11 == '11111111111111111111111111111111'[0:len(map_col_b11)]:
        c11   =  '0' 
    else:
        c11   =  HIF_ADDR[11+int(map_col_b11,2)] 
    if map_col_b10 == '11111111111111111111111111111111'[0:len(map_col_b10)]:
        c10   =  '0' 
    else:
        c10   =  HIF_ADDR[10+int(map_col_b10,2)] 
    c9    =  HIF_ADDR[9 +int(map_col_b9 ,2)] 
    c8    =  HIF_ADDR[8 +int(map_col_b8 ,2)] 
    c7    =  HIF_ADDR[7 +int(map_col_b7 ,2)] 
    c6    =  HIF_ADDR[6 +int(map_col_b6 ,2)] 
    c5    =  HIF_ADDR[5 +int(map_col_b5 ,2)] 
    c4    =  HIF_ADDR[4 +int(map_col_b4 ,2)] 
    c3    =  HIF_ADDR[3 +int(map_col_b3 ,2)] 
    c2    =  HIF_ADDR[2 +int(map_col_b2 ,2)] 
    c1    = '0'
    c0    = '0'

    cs    = cs0
    bank  = b2+b1+b0
    row   = r14+r13+r12+r11+r10+r9+r8+r7+r6+r5+r4+r3+r2+r1+r0
    column= c9+c8+c7+c6+c5+c4+c3+c2+c1+c0
    #print 'AXI_ADDR = '+AXI_ADDR
    #print 'cs = '+ cs
    #print 'bank = ' +bank
    #print 'row = ' +row
    #print 'column = '+column

#======================================================================================
def VIP_LOGIC_ADDR(cs,bank,row,column):
    global logical_addr
    bank_addr_width = 3
    row_addr_width  = 15
    column_addr_width = 10
    logical_addr = ((int(bank,2) << (row_addr_width + column_addr_width)) | (int(row,2) << (column_addr_width)) | int(column,2))
    #print 'logical_addr = ' + str(hex(logical_addr) )

#======================================================================================
if input_hex_file:
    file_read_axi_doc = open(input_hex_file,'r')
    file_write_rank0_channel_A = open('./bin/zmem0a.hex','a')
    file_write_rank0_channel_B = open('./bin/zmem0b.hex','a')
    file_write_rank1_channel_A = open('./bin/zmem1a.hex','a')
    file_write_rank1_channel_B = open('./bin/zmem1b.hex','a')

    read_lines = file_read_axi_doc.readlines()

#------------------------------------------------------------------------------
# for all data file with write axi addr
def WRITE_AXI_ADDR(axi_addr):
    with_axi_addr =axi_addr
    #print 'axi addr = ' +with_axi_addr
    if with_axi_addr:
        AXItoHIF(with_axi_addr)
        VIP_LOGIC_ADDR(cs,bank,row,column)
        if cs == '1':
            #print type(axi_addr)
            #print bin((int(axi_addr,16)>>2))[-4:]
            A_1 = hex(logical_addr + int(bin((int(axi_addr,16)>>2))[-4:],2))
            #print A_1
            file_write_rank1_channel_A.write('@'+str(A_1[2:])+' ')
            file_write_rank1_channel_B.write('@'+str(A_1[2:])+' ')
        else:
            A_2 = hex(logical_addr + int(bin((int(axi_addr,16)>>2))[-4:],2))
            file_write_rank0_channel_A.write('@'+str(A_2[2:])+' ')
            file_write_rank0_channel_B.write('@'+str(A_2[2:])+' ')
#------------------------------------------------------------------------------

if len_size:
    size =  int(len_size)
else:
    size = float('inf')
length = 0
if input_hex_file:
    for line in read_lines:
        if length < size:
            WRITE_AXI_ADDR(input_axi_addr)
            if line:
                if cs == '1':
                    file_write_rank1_channel_B.write(line[0:2])
                    file_write_rank1_channel_B.write(line[2:4]+'\n')
                    file_write_rank1_channel_A.write(line[4:6])
                    file_write_rank1_channel_A.write(line[6:8]+'\n')
                else:
                    file_write_rank0_channel_B.write(line[0:2])
                    file_write_rank0_channel_B.write(line[2:4]+'\n')
                    file_write_rank0_channel_A.write(line[4:6])
                    file_write_rank0_channel_A.write(line[6:8]+'\n')
        input_axi_addr = str(hex(int(input_axi_addr,16) + 4)[2:])

if input_hex_file:
    file_write_rank0_channel_A.close()
    file_write_rank0_channel_B.close()
    file_write_rank1_channel_A.close()
    file_write_rank1_channel_B.close()


