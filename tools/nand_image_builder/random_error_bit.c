#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#define ERROR_BASE_ADDRESS	0
#define ERROR_BASE_SIZE		1024 //bytes
#define ERROR_BIT_CNT		80

#define DEBUG		1

#if DEBUG > 0
	#define TAG "[random_error] "
	#define debug_verbose(fmt, ...) printf(TAG fmt, ##__VA_ARGS__)
#else
	#define debug_verbose(fmt, ...) do {} while (0)
#endif

int main()
{
	FILE *fd;
	int i, j;
	char data_byte;
	unsigned char *buf, *buf_cmp;
	unsigned int err_bit[100]; // store the error bit
	unsigned int err_bit_cnt = ERROR_BIT_CNT;
	unsigned int err_addr_start = ERROR_BASE_ADDRESS;
	unsigned int err_size = ERROR_BASE_SIZE;

	debug_verbose("error area start from= %d\n", err_addr_start);
	debug_verbose("error area size= %d\n", err_size);
	debug_verbose("error bit total cnt= %d\n", err_bit_cnt);

	buf = malloc(ERROR_BASE_SIZE);
	memset(buf, 0, ERROR_BASE_SIZE);
	buf_cmp = malloc(ERROR_BASE_SIZE);
	memset(buf_cmp, 0, ERROR_BASE_SIZE);

	fd = fopen("pnand_err.img", "r+");
	if (fd == NULL) {
		printf("Failed to open pnand.img\n");
		return -1;
	}

	/* get the data from image */
	fseek(fd, ERROR_BASE_ADDRESS, SEEK_SET);//page 0
	fread(buf, 1, ERROR_BASE_SIZE, fd);
	memcpy(buf_cmp, buf, ERROR_BASE_SIZE);

	/* init random */
	srand((unsigned)time(NULL));

	/***************** main code of error bit *****************/

	/* get the error bit num randomly */
	for (i = 0; i < err_bit_cnt; i++) {
		err_bit[i] = rand() % (ERROR_BASE_SIZE * 8);

		/* avoid modifying a bit repeatedly */
		for (j = 0; j < i; j++) {
			if (err_bit[i] == err_bit[j]) {
				i--;
				break;
			}
		}
	}

	/* sort the error bit num from smallest to largest */
	for (i = 0; i < err_bit_cnt; i++) {
		int flag = 1;//speed up sort

		for (j = 0; j < err_bit_cnt - 1 - i; j++) {
			/* swap */
			if (err_bit[j] > err_bit[j + 1]) {
				err_bit[j] = err_bit[j] + err_bit[j + 1];
				err_bit[j + 1] = err_bit[j] - err_bit[j + 1];
				err_bit[j] = err_bit[j] - err_bit[j + 1];
				flag = 0;
			}
		}

		if (flag == 1)
			break;

	}

	/* inverse the bit */
	for (i = 0; i < err_bit_cnt; i++)
		*(buf + err_bit[i] / 8) ^= (1 << (err_bit[i] % 8));

	fseek(fd, ERROR_BASE_ADDRESS, SEEK_SET);
	fwrite(buf, ERROR_BASE_SIZE, 1, fd);

	/* compare */
	for (i = 0; i < ERROR_BASE_SIZE; i++) {
		if (*(buf + i) != *(buf_cmp + i)) {
			debug_verbose("bytes[%d]\t: 0x%02x--->0x%02x\n", i, *(buf_cmp + i), *(buf + i));
		}
	}

	free(buf);
	free(buf_cmp);
	fclose(fd);

	return 0;
}
