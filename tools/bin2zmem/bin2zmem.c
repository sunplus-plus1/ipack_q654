#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

//static int g_debug = 0;
static int g_debug = 1;

//#define dbg(args...) do { if (g_debug) fprintf(stderr, ##args); } while (0)
#define dbg(args...)

static void usage(void)
{
	fprintf(stderr, "bin2zmem (build: %s %s)\n", __DATE__, __TIME__);
	fprintf(stderr, "usage:\n"
			"bin2zmem <input> <output> <input_sz> <zmem_off>\n"
	       );
}

//#define ZMEM_IDX_STEP  8   // old zebu (137) : each @ has 8 bytes, little-endian (L: byte 0)
#define ZMEM_IDX_STEP  16  // new zebu (q628) : each @ has 16 bytes, big-endian (R: byte 0)

//#define ZENDIAN       htonl
#define ZENDIAN       /* x86 is little endian */

// argv
// [1]   = input bin filename       (read only)
// [2]   = output zmem hex filename (append)
// [3]   = input offset
// [4]   = zmem offset
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
	unsigned int zmem_off_zw; // offset in zword (size = ZMEM_IDX_STEP)
	int res;
	int i;
	unsigned int val, val2, val3, val4;

	if (argc < 5) {
		usage();
		return -1;
	}

	input = argv[1];
	output = argv[2];
	in_skip = strtol(argv[3], NULL, 0);
	zmem_off = strtol(argv[4], NULL, 0);

	if (zmem_off % ZMEM_IDX_STEP) {
		fprintf(stderr, "Error: arg zmem_off is not ZMEM_IDX_STEP-byte aligned!\n");
		return -1;
	}

	zmem_off_zw = zmem_off / ZMEM_IDX_STEP;

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
			int pad = ZMEM_IDX_STEP - (got % ZMEM_IDX_STEP);
			if (pad != ZMEM_IDX_STEP) {
				dbg("pad %d\n", pad);
				// pad with 0 until next ZMEM_IDX_STEP-byte aligned address
				memset(&buf[got], 0, pad);
				got += pad;
			}
		}

		for (i = 0; got > 0; i++, got -= ZMEM_IDX_STEP, zmem_off_zw++) {
			// Output format:
			//   @<zword off>	<zword value>

			val  = ZENDIAN(*(int *)&buf[i * ZMEM_IDX_STEP]);
			val2 = ZENDIAN(*(int *)&buf[(i * ZMEM_IDX_STEP) + 0x4]);
			val3 = ZENDIAN(*(int *)&buf[(i * ZMEM_IDX_STEP) + 0x8]);
			val4 = ZENDIAN(*(int *)&buf[(i * ZMEM_IDX_STEP) + 0xc]);
			fprintf(fp_out, "@%x %08x%08x%08x%08x\n", (int)zmem_off_zw, val4, val3, val2, val);
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
