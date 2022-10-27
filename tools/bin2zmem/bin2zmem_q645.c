#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#if 0
static int g_debug = 1;
#define dbg(args...) do { if (g_debug) fprintf(stderr, ##args); } while (0)
#else
static int g_debug = 0;
#define dbg(args...)
#endif

static void usage(void)
{
	fprintf(stderr, "bin2zmem (build: %s %s)\n", __DATE__, __TIME__);
	fprintf(stderr, "usage:\n"
			"bin2zmem <input> <output> <input_off> <zmem_off> <1=dram_xtor>\n"
	       );
}

// If using DRAM XTOR :
// 1. cpu view != dram view.
// 2. hex format is 2 bytes

#define ZMEM_FAKE_DRAM_ZW_LEN   16 // zebu + fake dram (q628/q633/642) : each @ has 16 bytes, big-endian (R: byte 0)
//#define ZMEM_FAKE_DRAM_ZW_LEN   8 // zebu + fake dram (q644) : each @ has 8 bytes, big-endian (R: byte 0)
//#define ZMEM_DRAM_XTOR_ZW_LEN    2 // zebu + dram xtor (q628/q633) : each @ has 2 bytes, big-endian (R: byte 0)
#define ZMEM_DRAM_XTOR_ZW_LEN    4 // zebu + dram xtor (q642) : each @ has 4 bytes, big-endian (R: byte 0)

// q642
// zebu_sim_DRAM    : seprate 32-bit data to (High 16b, Low 16b) = (zmem2.hex, zmem.hex)
// zebu_sim_DRAM_x8 : seprate 32-bit data to (8b*4) = high->low (zmem4/zmem3/zmem2/zmem).hex
#define ZMEM_DRAM_XTOR_32B_SEP


//#define ZENDIAN       htonl
#define ZENDIAN       /* x86 is little endian */

#define ZMEM0B_HEX		"./bin/zmem0b.hex"
#define ZMEM1A_HEX		"./bin/zmem1a.hex"
#define ZMEM1B_HEX		"./bin/zmem1b.hex"

/**############### DDR4 controller ###################*/

#define ADDRMAP0    0x00000016  
#define ADDRMAP1    0x00080808 
#define ADDRMAP2    0x00000000 
#define ADDRMAP3    0x00000000 
#define ADDRMAP4    0x00001f1f 
#define ADDRMAP5    0x070f0707 
#define ADDRMAP6    0x08070707
#define ADDRMAP7    0x00000f08
#define ADDRMAP8    0x00003f3f 
#define ADDRMAP9    0x07070707 
#define ADDRMAP10   0x07070707
#define ADDRMAP11   0x001f1f07

unsigned int cs;
/*  convert from python */
static unsigned int cpu2dxtor_view_8x4(unsigned int cpu_view)
{
	unsigned int dram_view;
	unsigned int cs0; 
    unsigned int b2, b1, b0 ;   
    unsigned int r17, r16, r15, r14, r13, r12, r11, r10, r9, r8, r7, r6, r5, r4, r3, r2, r1, r0 ;
    unsigned int c11, c10, c9, c8, c7, c6, c5, c4, c3, c2, c1, c0;
    unsigned int  bank, row, column;

	int map_dch_bit0             = (ADDRMAP0 >> 16)&0x1F;	//ADDRMAP0[16 : 20+1]
	int map_cs_bit1              = (ADDRMAP0 >> 8)&0x1F;	//ADDRMAP0 [8  : 12+1] 
	int map_cs_bit0              = (ADDRMAP0 >> 0)&0x1F;	//ADDRMAP0 [0  : 4+1] 
	int map_bank_b2              = (ADDRMAP1 >> 16)&0x3F;	//ADDRMAP1 [16 : 21+1] 
	int map_bank_b1              = (ADDRMAP1 >> 8)&0x3F;	//ADDRMAP1 [8  : 13+1] 
	int map_bank_b0              = (ADDRMAP1 >> 0)&0x3F;	//ADDRMAP1 [0  : 5+1] 
	int map_col_b5               = (ADDRMAP2 >> 24)&0xF;	//ADDRMAP2 [24 : 27+1] 
	int map_col_b4               = (ADDRMAP2 >> 16)&0xF;	//ADDRMAP2 [16 : 19+1] 
	int map_col_b3               = (ADDRMAP2 >> 8)&0x1F;	//ADDRMAP2 [8  : 12+1] 
	int map_col_b2               = (ADDRMAP2 >> 0)&0xF;		//ADDRMAP2 [0  : 3+1] 
	int map_col_b9               = (ADDRMAP3 >> 24)&0x1F;	//ADDRMAP3 [24 : 28+1] 
	int map_col_b8               = (ADDRMAP3 >> 16)&0x1F;	//ADDRMAP3 [16 : 20+1] 
	int map_col_b7               = (ADDRMAP3 >> 8)&0x1F;	//ADDRMAP3 [8  : 12+1] 
	int map_col_b6               = (ADDRMAP3 >> 0)&0x1F;	//ADDRMAP3 [0  : 4+1] 
	int map_col_addr_shift       = (ADDRMAP4 >> 31)&0x1;	//ADDRMAP4 [31 : ] 
	int map_col_b11              = (ADDRMAP4 >> 8)&0x1F;	//ADDRMAP4 [8  : 12+1] 
	int map_col_b10              = (ADDRMAP4 >> 0)&0x1F;	//ADDRMAP4 [0  : 4+1] 
	int map_row_b11              = (ADDRMAP5 >> 24)&0xF;	//ADDRMAP5 [24 : 27+1] 
	int map_row_b2_10            = (ADDRMAP5 >> 16)&0xF;	//ADDRMAP5 [16 : 19+1] 
	int map_row_b1               = (ADDRMAP5 >> 8)&0xF;		//ADDRMAP5 [8  : 11+1] 
	int map_row_b0               = (ADDRMAP5 >> 0)&0xF;		//ADDRMAP5 [0  : 3+1] 
	int map_lpddr34_3gb_6gb_12gb = (ADDRMAP6 >> 29)&0x7;	//ADDRMAP6 [29 : 31+1] 
	int map_row_b15              = (ADDRMAP6 >> 24)&0xF;	//ADDRMAP6 [24 : 27+1] 
	int map_row_b14              = (ADDRMAP6 >> 16)&0xF;	//ADDRMAP6 [16 : 19+1] 
	int map_row_b13              = (ADDRMAP6 >> 8)&0xF;		//ADDRMAP6 [8  : 11+1] 
	int map_row_b12              = (ADDRMAP6 >> 0)&0xF;		//ADDRMAP6 [0  : 3+1] 
	int map_row_b17              = (ADDRMAP7 >> 8)&0xF;		//ADDRMAP7 [8  : 11+1] 
	int map_row_b16              = (ADDRMAP7 >> 0)&0xF;		//ADDRMAP7 [0  : 3+1] 
	int map_bg_b1                = (ADDRMAP8 >> 8)&0x3F;	//ADDRMAP8 [8  : 13+1] 
	int map_bg_b0                = (ADDRMAP8 >> 0)&0x3F;	//ADDRMAP8 [0  : 5+1] 
	int map_row_b5               = (ADDRMAP9 >> 24)&0xF;	//ADDRMAP9 [24 : 27+1] 
	int map_row_b4               = (ADDRMAP9 >> 16)&0xF;	//ADDRMAP9 [16 : 19+1] 
	int map_row_b3               = (ADDRMAP9 >> 8)&0xF;		//ADDRMAP9 [8  : 11+1] 
	int map_row_b2               = (ADDRMAP9 >> 0)&0xF;		//ADDRMAP9 [0  : 3+1] 
	int map_row_b9               = (ADDRMAP10 >> 24)&0xF;	//ADDRMAP10[24 : 27+1] 
	int map_row_b8               = (ADDRMAP10 >> 16)&0xF;	//ADDRMAP10[16 : 19+1] 
	int map_row_b7               = (ADDRMAP10 >> 8)&0xF;	//ADDRMAP10[8  : 11+1] 
	int map_row_b6               = (ADDRMAP10 >> 0)&0xF;	//ADDRMAP10[0  : 3+1]
	int map_cid_b1               = (ADDRMAP11 >> 16)&0x1F;	//ADDRMAP11[16 : 20+1]  
	int map_cid_b0               = (ADDRMAP11 >> 8)&0x1F;	//ADDRMAP11[8  : 12+1]  
	int map_row_b10              = (ADDRMAP11 >> 0)&0xF;	//ADDRMAP11[0  : 3+1] 

	unsigned long long HIF_ADDR = (cpu_view >> 6) << 4;

	cs0   = (HIF_ADDR >> (6+map_cs_bit0))&0x01       ;// HIF_ADDR[6 +int(map_cs_bit0,2)]
     
    b2    = (HIF_ADDR >> (4+map_bank_b2))&0x01       ;// HIF_ADDR[4 +int(map_bank_b2,2)] 
    b1    = (HIF_ADDR >> (3+map_bank_b1))&0x01       ;//  HIF_ADDR[3 +int(map_bank_b1,2)] 
    b0    = (HIF_ADDR >> (2+map_bank_b0))&0x01       ;//  HIF_ADDR[2 +int(map_bank_b0,2)] 
    
    if (map_row_b17 == 0x0f)
        r17   =  0; 
    else
        r17   = (HIF_ADDR >> (23+map_row_b17))&0x01       ;// HIF_ADDR[23+int(map_row_b17,2)] 
    r16   =  (HIF_ADDR >> (22+map_row_b16))&0x01       ;//HIF_ADDR[22+int(map_row_b16,2)] 
    r15   =  (HIF_ADDR >> (21+map_row_b15))&0x01       ;//HIF_ADDR[21+int(map_row_b15,2)] 

    r14   =  (HIF_ADDR >> (20+map_row_b14))&0x01       ;//HIF_ADDR[20+int(map_row_b14,2)] 
    r13   =  (HIF_ADDR >> (19+map_row_b13))&0x01       ;//HIF_ADDR[19+int(map_row_b13,2)] 
    r12   =  (HIF_ADDR >> (18+map_row_b12))&0x01       ;//HIF_ADDR[18+int(map_row_b12,2)] 
    r11   =  (HIF_ADDR >> (17+map_row_b11))&0x01       ;//HIF_ADDR[17+int(map_row_b11,2)] 

    if (map_row_b2_10 == 0x0f)
    {
          r10   =  (HIF_ADDR >> (16+map_row_b10))&0x01       ;//HIF_ADDR[16+int(map_row_b10,2)] 
          r9    =  (HIF_ADDR >> (15+map_row_b9))&0x01       ;//HIF_ADDR[15+int(map_row_b9 ,2)] 
          r8    =  (HIF_ADDR >> (14+map_row_b8))&0x01       ;//HIF_ADDR[14+int(map_row_b8 ,2)] 
          r7    =  (HIF_ADDR >> (13+map_row_b7))&0x01       ;//HIF_ADDR[13+int(map_row_b7 ,2)] 
          r6    =  (HIF_ADDR >> (12+map_row_b6))&0x01       ;//HIF_ADDR[12+int(map_row_b6 ,2)] 
          r5    =  (HIF_ADDR >> (11+map_row_b5))&0x01       ;// HIF_ADDR[11+int(map_row_b5 ,2)] 
          r4    =  (HIF_ADDR >> (10+map_row_b4))&0x01       ;//HIF_ADDR[10+int(map_row_b4 ,2)] 
          r3    =  (HIF_ADDR >> (9+map_row_b3))&0x01       ;//HIF_ADDR[9 +int(map_row_b3 ,2)] 
          r2    =  (HIF_ADDR >> (8+map_row_b2))&0x01       ;//HIF_ADDR[8 +int(map_row_b2 ,2)]
    } 
    else
    {
          r10   =(HIF_ADDR >> (16+map_row_b2_10))&0x01       ;//  HIF_ADDR[16+int(map_row_b2_10,2)] 
          r9    =(HIF_ADDR >> (15+map_row_b2_10))&0x01       ;//  HIF_ADDR[15+int(map_row_b2_10,2)] 
          r8    =(HIF_ADDR >> (14+map_row_b2_10))&0x01       ;//  HIF_ADDR[14+int(map_row_b2_10,2)] 
          r7    =(HIF_ADDR >> (13+map_row_b2_10))&0x01       ;//  HIF_ADDR[13+int(map_row_b2_10,2)] 
          r6    =(HIF_ADDR >> (12+map_row_b2_10))&0x01       ;//  HIF_ADDR[12+int(map_row_b2_10,2)] 
          r5    =(HIF_ADDR >> (11+map_row_b2_10))&0x01       ;//  HIF_ADDR[11+int(map_row_b2_10,2)] 
          r4    =(HIF_ADDR >> (10+map_row_b2_10))&0x01       ;//  HIF_ADDR[10+int(map_row_b2_10,2)] 
          r3    =(HIF_ADDR >> (9+map_row_b2_10))&0x01        ;//  HIF_ADDR[9 +int(map_row_b2_10,2)] 
          r2    =(HIF_ADDR >> (8+map_row_b2_10))&0x01        ;//  HIF_ADDR[8 +int(map_row_b2_10,2)] 
    }	
    r1    =  (HIF_ADDR >> (7+map_row_b1))&0x01        ;//HIF_ADDR[7 +int(map_row_b1 ,2)] 
    r0    =  (HIF_ADDR >> (6+map_row_b0))&0x01        ;//HIF_ADDR[6 +int(map_row_b0 ,2)] 
    
    if (map_col_b11 == 0x1f)  //'11111111111111111111111111111111'[0:len(map_col_b11)]:
        c11   =  0 ;
    else
        c11   =   (HIF_ADDR >> (11+map_col_b11))&0x01        ; //HIF_ADDR[11+int(map_col_b11,2)] 
    if (map_col_b10 == 0x1f) //'11111111111111111111111111111111'[0:len(map_col_b10)]:
        c10   =  0 ;
    else
    	c10   =  (HIF_ADDR >> (10+map_col_b10))&0x01        ;//HIF_ADDR[10+int(map_col_b10,2)] 
    c9    =  (HIF_ADDR >> (9+map_col_b9))&0x01        ;//HIF_ADDR[9 +int(map_col_b9 ,2)] 
    c8    =  (HIF_ADDR >> (8+map_col_b8))&0x01        ;//HIF_ADDR[8 +int(map_col_b8 ,2)] 
    c7    =  (HIF_ADDR >> (7+map_col_b7))&0x01        ;//HIF_ADDR[7 +int(map_col_b7 ,2)] 
    c6    =  (HIF_ADDR >> (6+map_col_b6))&0x01        ;//HIF_ADDR[6 +int(map_col_b6 ,2)] 
    c5    =  (HIF_ADDR >> (5+map_col_b5))&0x01        ;//HIF_ADDR[5 +int(map_col_b5 ,2)] 
    c4    =  (HIF_ADDR >> (4+map_col_b4))&0x01        ;//HIF_ADDR[4 +int(map_col_b4 ,2)] 
    c3    =  (HIF_ADDR >> (3+map_col_b3))&0x01        ;//HIF_ADDR[3 +int(map_col_b3 ,2)] 
    c2    =  (HIF_ADDR >> (2+map_col_b2))&0x01        ;//HIF_ADDR[2 +int(map_col_b2 ,2)] 
    c1    = 0;
    c0    = 0;
    cs    = cs0;
    bank  = (b2<<2)+(b1<<1)+b0;
    row   = (r14<<14)+(r13<<13)+(r12<<12)+(r11<<11)+(r10<<10)+(r9<<9)+(r8<<8)+(r7<<7)+(r6<<6)+(r5<<5)+(r4<<4)+(r3<<3)+(r2<<2)+(r1<<1)+r0;
	column= (c9<<9)+(c8<<8)+(c7<<7)+(c6<<6)+(c5<<5)+(c4<<4)+(c3<<3)+(c2<<2)+(c1<<1)+c0;
	int bank_addr_width = 3;
    int row_addr_width  = 15;
    int column_addr_width = 10;
    dram_view = (bank << (row_addr_width + column_addr_width)) | row << (column_addr_width) | column;
	return dram_view;
}

// argv
// [1]   = input bin filename       (read only)
// [2]   = output zmem hex filename (append)
// [3]   = input offset
// [4]   = zmem offset
// [5]   = dram view: 0=fake_dram, 1=DRAM_XTOR
// [6]   = number of zmem files
int main(int argc, char **argv)
{
	char *input, *output, *z_out[4] = { 0 };
	struct stat st;
	FILE *fp_in = NULL, *fp_out[4] = { 0 }; // = NULL;
#ifdef ZMEM_DRAM_XTOR_32B_SEP
	int sep_files = 4; // zmem.hex, zmem2.hex
#else
	int sep_files = 1; // zmem.hex
#endif
	int got;
	unsigned char buf[512];
	unsigned int bufsz = sizeof(buf);;
	int in_skip;
	unsigned int zmem_off;
	unsigned int zmem_off_zw, zw_off; // offset in zword (size = zw_size)
	int res, i;
	unsigned int val, val2, val3, val4;
	int dxtor = 0;
	int zw_step = ZMEM_FAKE_DRAM_ZW_LEN;

	if (argc < 5) {
		usage();
		return -1;
	}

	input = argv[1];
	z_out[0] = output = argv[2];
	in_skip = strtol(argv[3], NULL, 0);
	zmem_off = strtol(argv[4], NULL, 0);

	if (argc >= 6 && strtol(argv[5], NULL, 0) == 1) {
		dbg("Gen for dram xtor\n");
		dxtor = 1;
		zw_step = ZMEM_DRAM_XTOR_ZW_LEN;
	}

	if (argc >= 7) {
		// only 1, 2, 4 are possible
		sep_files = strtol(argv[6], NULL, 0);
		if (sep_files > 4 || sep_files == 3)
			return -1;
	}

	dbg("zw_step=%d bytes\n", zw_step);

	if (zmem_off % zw_step) {
		fprintf(stderr, "Error: arg zmem_off is not zw_step-byte aligned!\n");
		return -1;
	}

	zmem_off_zw = zmem_off / zw_step;

	printf("input=%s output=%s in_skip=0x%x zmem_off=0x%x (qw=0x%x) sep=%d dxtor=%d \n",
		input, output, in_skip, zmem_off, zmem_off_zw, sep_files,dxtor);

	// get file size
	memset(&st, 0, sizeof(st));
	stat(input, &st);
	if (st.st_size == 0) {
		fprintf(stderr, "input %s is empty file\n", input);
		return -1;
	}

	dbg("fopen %s\n", input);
	fp_in = fopen(input, "rb");
	if (NULL == fp_in) {
		fprintf(stderr, "cannot open %s\n", input);
		goto err_out;
	}

	dbg("fseek %s to 0x%x\n", input, in_skip);
	res = fseek(fp_in, in_skip, SEEK_SET);
	if (res < 0) {
		fprintf(stderr, "input: seek offset=0x%x failed, res=%d\n", in_skip, res);
		goto err_out;
	}

	dbg("fopen %s\n", output);
	fp_out[0] = fopen(output, "a+");
	if (NULL == fp_out[0]) {
		fprintf(stderr, "cannot open %s\n", output);
		goto err_out;
	}
#ifdef ZMEM_DRAM_XTOR_32B_SEP
	if (dxtor) {
		res = strlen(output) + 1; // 8+1  (0 ended)
		for (i = 1; i < sep_files; i++) {
			z_out[i] = malloc(res + 1); // zmem.hex -> zmem2.hex
			if(i == 1)
			{
				strcpy(z_out[i], ZMEM0B_HEX);
			}
			else if(i == 2)
			{
				strcpy(z_out[i], ZMEM1A_HEX);
			}
			else if(i == 3)
			{
				strcpy(z_out[i], ZMEM1B_HEX);
			}
			dbg("fopen %s\n", z_out[i]);
			fp_out[i] = fopen(z_out[i], "a+");
			if (NULL == fp_out[i]) {
				fprintf(stderr, "cannot open %s\n", z_out[i]);
				goto err_out;
			}
		}
	}
#endif

	// Loop {
	//    1. Read a piece to buf
	//    2. Write out the piece as zmem format
	// }
	do {
		got = fread(buf, 1, bufsz, fp_in);

		//dbg("fread got=%d\n", (int)got);

		if (got <= 0) {
			dbg("Input EOF (got=%d)\n", got);
			break;
		} else if (got < bufsz) {
			int pad = zw_step - (got % zw_step);
			if (pad != zw_step) {
				dbg("pad %d\n", pad);
				// pad with 0 until next zw_step-byte aligned #define ADDress
				memset(&buf[got], 0, pad);
				got += pad;
			}
		}

		for (i = 0; got > 0; i++, got -= zw_step, zmem_off_zw++) {
			// Output format:
			//   @<zword off>	<zword value>

			if (dxtor && sep_files == 2) {
				//zw_off = cpu2dxtor_view(zmem_off_zw * zw_step);
			} else if (dxtor && sep_files == 4) {
				zw_off = cpu2dxtor_view_8x4(zmem_off_zw * zw_step);
			} else {
				zw_off = zmem_off_zw;
			}
			if (zw_step == 16) {
				val  = ZENDIAN(*(int *)&buf[i * zw_step]);
				val2 = ZENDIAN(*(int *)&buf[(i * zw_step) + 0x4]);
				val3 = ZENDIAN(*(int *)&buf[(i * zw_step) + 0x8]);
				val4 = ZENDIAN(*(int *)&buf[(i * zw_step) + 0xc]);
				fprintf(fp_out[0], "@%x %08x%08x%08x%08x\n", (int)zw_off, val4, val3, val2, val);
			} else if (zw_step == 8) {
				val  = ZENDIAN(*(int *)&buf[i * zw_step]);
				val2 = ZENDIAN(*(int *)&buf[(i * zw_step) + 0x4]);
				fprintf(fp_out[0], "@%x %08x%08x\n", (int)zw_off, val2, val);
			} else if (zw_step == 4) {
				val  = ZENDIAN(*(unsigned int *)&buf[i * zw_step]);
#ifdef ZMEM_DRAM_XTOR_32B_SEP
				if (sep_files == 2) {
					fprintf(fp_out[0], "@%x %04x\n", (int)zw_off, val & 0xffff);
					fprintf(fp_out[1], "@%x %04x\n", (int)zw_off, (val >> 16) & 0xffff);
				} else if (sep_files == 4) {
					int new_zw_off = zw_off+(((zmem_off_zw * zw_step)>>2)&0xf);
					if(cs == 0)
					{	
						fprintf(fp_out[0], "@%x %04x\n", (int)new_zw_off, (val      ) & 0xffff);
						fprintf(fp_out[1], "@%x %04x\n", (int)new_zw_off, (val >>  16) & 0xffff);
					}
					else
					{
						fprintf(fp_out[2], "@%x %04x\n", (int)new_zw_off, (val >> 0) & 0xffff);
						fprintf(fp_out[3], "@%x %04x\n", (int)new_zw_off, (val >> 16) & 0xffff);
					}

				}
#else
				fprintf(fp_out[0], "@%x %08x\n", (int)zw_off, val);
#endif
			} else if (zw_step == 2) {
				val  = ZENDIAN(*(unsigned short *)&buf[i * zw_step]);
				fprintf(fp_out[0], "@%x %04x\n", (int)zw_off, val);
			}
		}
	} while (1);

	dbg("end of program\n");
err_out:
	if (fp_in)
		fclose(fp_in);
	if (fp_out[0])
		fclose(fp_out[0]);
#ifdef ZMEM_DRAM_XTOR_32B_SEP
	for (i = 1; i < 4; i++) {
		if (fp_out[i])
			fclose(fp_out[i]);
		if (z_out[i])
			free(z_out[i]);
	}
#endif

	return 0;
}
