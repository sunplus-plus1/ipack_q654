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

/*
 * @cpu_view : cpu view address in byte
 *
 * Return dram xtor view in zword (ZMEM_DRAM_XTOR_ZW_LEN)
 */
static unsigned int cpu2dxtor_view(unsigned int cpu_view)
{
	unsigned int dram_view, ba, other_addr;

	// Q642 zebu_sim_DRAM (16bits x2 ) , total 1 Gbytes
	/*
	 * dram view   = (        BA,     ROW,                  COL)
	 * = address_cpu ( [12,11,6],  [29:15],  [14:13, 10:7, 5:2])
	 * Offset      @ (        25,       10,       8,    4,   0 )
	 * bits                    3        15                   10  = 28 bits ,  in 4-byte (2 bits) units
	 *
	 * RD information :
	 *  BA2 = BUS_ADR[12], BA1 = BUS_ADR[11], BA0 = BUS_ADR[6]
	 *  other_addr = { BUS_ADR[29:13], BUS_ADR[10:7], BUS_ADR[5:0] }; // [26:0]
	 *  total_col_adr = other_addr >> 2                               // [24:0]
	 *  Col_Addr = total_col_adr[9:0]
	 *  Row_Addr = total_col_adr[24:10]                               // 15 bits
	 *  DRAM_ADR = {{BA2,BA1,BA0}, Row_Addr[14:0], Col_Addr[9:0]}     // 28 bits
	 *
	 * Eg: cpu address 0x2000_D144
	 * ->
	 * dram view =
	 * address_cpu ( ____ [12,11,6],                [29:15], [14:13, 10:7,  5:2])
	 *               0000     10 1     1 0000 0000 0000 01       10  0010  0001
	 * = 0b00_0621
	 */
	ba = (((cpu_view >> 11) & 3)    << 26) |
		(((cpu_view >> 6) & 1)	<< 25);

	other_addr =(cpu_view & 0x3f) |               // [5:0]
		(((cpu_view >> 7) & 0xf) << 6)|       // [10:7]
		(((cpu_view >> 13) & 0x7ffff) << 10); // [31:13]

	dram_view = ba | (other_addr >> 2);

	if (g_debug) {
		static int init = 0;
		if (init++ < 50) {
			dbg("cpu view=0x%08x\n", cpu_view);
			dbg("zw_off  =0x%08x\n", dram_view);
		}
	}

	return dram_view;
}

static unsigned int cpu2dxtor_view_8x4(unsigned int cpu_view)
{
	unsigned int dram_view;

	// Q642 zebu_sim_DRAM_x8 ( 8bits x4 ) , total 2 Gbytes
	/*
	 * dram view   = (        BA,     ROW,                  COL)
	 * = address_cpu ( [12:11,6],  [30:15],  [14:13, 10:7, 5:2])
	 * Offset      @ (        26,       10,       8,    4,   0 )
	 * bits                    3        16                   10       // 29 bits ,  in 4-byte units
	 *
	 * Eg: cpu address 0ff0_0040
	 * dram view =
	 * address_cpu ( ___ [12:11,6] ,               [30:15], [14:13, 10:7,  5:2])
	 *               000    0 0 1    00 0111 1111 1000 00       00  0000  0000
	 * = 047f_8000
	 *
	 * Eg: cpu address 0x4000_d144
	 * ->
	 * dram view =
	 * address_cpu ( ___ [12:11,6] ,               [30:15], [14:13, 10:7,  5:2])
	 *               000    1 0 1    10 0000 0000 0000 01       10  0010  0001
	 * = 1600_0621
	 */
	dram_view =
		(((cpu_view >> 11) &       3) << 27) |
		(((cpu_view >>  6) &       1) << 26) |
		(((cpu_view >> 13) & 0x3ffff) <<  8) |
		(((cpu_view >>  7) &     0xf) <<  4) |
		(((cpu_view >>  2) &     0xf) <<  0);

	if (g_debug) {
		static int init = 0;
		if (init++ < 50) {
			dbg("cpu view=0x%08x\n", cpu_view);
			dbg("zw_off  =0x%08x\n", dram_view);
		}
	}

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
	int sep_files = 2; // zmem.hex, zmem2.hex
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

	printf("input=%s output=%s in_skip=0x%x zmem_off=0x%x (qw=0x%x) sep=%d\n",
		input, output, in_skip, zmem_off, zmem_off_zw, sep_files);

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
			strcpy(z_out[i], output);
			strcpy(z_out[i] + res - 5, "x.hex");
			z_out[i][res - 5] = '1' + i; // 2 3 4

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
				// pad with 0 until next zw_step-byte aligned address
				memset(&buf[got], 0, pad);
				got += pad;
			}
		}

		for (i = 0; got > 0; i++, got -= zw_step, zmem_off_zw++) {
			// Output format:
			//   @<zword off>	<zword value>

			if (dxtor && sep_files == 2) {
				zw_off = cpu2dxtor_view(zmem_off_zw * zw_step);
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
					fprintf(fp_out[0], "@%x %02x\n", (int)zw_off, (val      ) & 0xff);
					fprintf(fp_out[1], "@%x %02x\n", (int)zw_off, (val >>  8) & 0xff);
					fprintf(fp_out[2], "@%x %02x\n", (int)zw_off, (val >> 16) & 0xff);
					fprintf(fp_out[3], "@%x %02x\n", (int)zw_off, (val >> 24) & 0xff);
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
