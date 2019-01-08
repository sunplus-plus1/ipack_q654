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

#define ZMEM_FAKE_DRAM_ZW_LEN   16 // zebu + fake dram (q628) : each @ has 16 bytes, big-endian (R: byte 0)
#define ZMEM_DRAM_XTOR_ZW_LEN    2 // zebu + dram xtor (q628) : each @ has 2 bytes, big-endian (R: byte 0)

//#define ZENDIAN       htonl
#define ZENDIAN       /* x86 is little endian */

/*
 * @cpu_view : cpu view address in byte
 *
 * Return dram xtor view in zword (ZMEM_DRAM_XTOR_ZW_LEN)
 */
static unsigned int cpu2dxtor_view(unsigned int cpu_view)
{
	unsigned int dram_view, tmp;

	/* dram view   = (      BA,       ROW,   COL)
	 * Offset      @       25         10       0
	 * = address_cpu ( [13:11],   [28:14], [10:1]) 	// Q628 is 4Gb
	 */
	dram_view =
		(((cpu_view >> 11) & 0x7) << 25)    | /* cpu[13:11]   */
		(((cpu_view >> 14) & 0x7fff) << 10) | /* cpu[28:14] */
		((cpu_view  >>  1) & 0x3ff);          /* cpu[10:1]  */

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
int main(int argc, char **argv)
{
	char *input, *output;
	struct stat st;
	FILE *fp_in = NULL, *fp_out = NULL;
	int got;
	unsigned char buf[512];
	unsigned int bufsz = sizeof(buf);;
	int in_skip;
	unsigned int zmem_off;
	unsigned int zmem_off_zw, zw_off; // offset in zword (size = zw_size)
	int res;
	int i;
	unsigned int val, val2, val3, val4;
	int dxtor = 0;
	int zw_step = ZMEM_FAKE_DRAM_ZW_LEN;

	if (argc < 5) {
		usage();
		return -1;
	}

	input = argv[1];
	output = argv[2];
	in_skip = strtol(argv[3], NULL, 0);
	zmem_off = strtol(argv[4], NULL, 0);

	if (argc >= 6 && strtol(argv[5], NULL, 0) == 1) {
		dbg("Gen for dram xtor\n");
		dxtor = 1;
		zw_step = ZMEM_DRAM_XTOR_ZW_LEN;
	}

	dbg("zw_step=%d bytes\n", zw_step);

	if (zmem_off % zw_step) {
		fprintf(stderr, "Error: arg zmem_off is not zw_step-byte aligned!\n");
		return -1;
	}

	zmem_off_zw = zmem_off / zw_step;

	fprintf(stderr, "input=%s output=%s in_skip=0x%x zmem_off=0x%x (qw=0x%x)\n",
		input, output, in_skip, zmem_off, zmem_off_zw);

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
	fp_out = fopen(output, "a+");
	if (NULL == fp_in) {
		fprintf(stderr, "cannot open %s\n", output);
		goto err_out;
	}

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

			zw_off = dxtor ? cpu2dxtor_view(zmem_off_zw * zw_step) : zmem_off_zw;
			if (zw_step == 16) {
				val  = ZENDIAN(*(int *)&buf[i * zw_step]);
				val2 = ZENDIAN(*(int *)&buf[(i * zw_step) + 0x4]);
				val3 = ZENDIAN(*(int *)&buf[(i * zw_step) + 0x8]);
				val4 = ZENDIAN(*(int *)&buf[(i * zw_step) + 0xc]);
				fprintf(fp_out, "@%x %08x%08x%08x%08x\n", (int)zw_off, val4, val3, val2, val);
			} else if (zw_step == 2) {
				val  = ZENDIAN(*(unsigned short *)&buf[i * zw_step]);
				fprintf(fp_out, "@%x %04x\n", (int)zw_off, val);
			}
		}
	} while (1);

	dbg("end of program\n");
err_out:
	if (fp_in)
		fclose(fp_in);
	if (fp_out)
		fclose(fp_out);
	return 0;
}
