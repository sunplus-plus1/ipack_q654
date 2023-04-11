#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "include/bch.h"

/* Select page size to test */
//#define TEST_PAGE_4K
//#define TEST_PAGE_8K

/* Avoid configuration conflicts */
#ifdef TEST_PAGE_8K
#undef TEST_PAGE_4K
#endif

#ifdef TEST_PAGE_4K
#undef TEST_PAGE_8K
#endif

/* PNAND actiming */
#define CONFIG_PNAND

/* Enable the BBM(Bad Block Management) function */
#undef CONFIG_BI

#define DEBUG_BUILD_IMG
#ifdef DEBUG_BUILD_IMG
#define xt_debug(format, ...)	printf("[%s:%d] "format"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define xt_debug(format, ...)
#endif

#define BCH_PRIMITIVE_POLY	0x4443 //0x600D

#define BCH_GF_ORDER_M		14

#define BHDR_BEG_SIGNATURE	0x52444842  // BHDR
#define BHDR_END_SIGNATURE	0x444e4548  // HEND

#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))
#define ALIGN_TO_16B(X)		((X + ((1 << 4) - 1)) & (0xFFFFFFFF - ((1 << 4) - 1)))

#define SIZE_FILE_NAME		32
#define NUM_OF_PARTITION	9

#define IDX_PARTITION_XBOOT1	(0)
#define IDX_PARTITION_UBOOT1	(1)
#define IDX_PARTITION_UBOOT2	(2)

typedef unsigned char u8;
typedef unsigned int  u32;
typedef unsigned long u64;

struct partition_info_s {
	u8 file_name[SIZE_FILE_NAME];  // file name of source (xboot1,uboot1,uboot2...)
	u64 file_size;                 // file size of source align to 1k
	u32 partition_start_addr;      // partition start address in NAND image file
	u64 partition_size;            // partition occupied size in NAND image file(include oob size)
	u32 flags;
} __attribute__((packed));

struct image_info {
	u32 page_size;
	u32 oob_size;
	int eraseblock_size;
	int block_cnt;
	const u8 *dest;
	u8  BI_bytes;
	/* ecc info */
	u32 ecc_strength;
	u32 ecc_step_size;
	u32 usable_page_size;
};

#ifdef TEST_PAGE_8K
struct image_info q654_image_info = {\
	.page_size = 8192,\
	.oob_size = 640,\
	.eraseblock_size = 0x100000,\
	.block_cnt = 4096\
};
#elif defined TEST_PAGE_4K
struct image_info q654_image_info = {\
	.page_size = 4096,\
	.oob_size = 128,\
	.eraseblock_size = 0x40000,\
	.block_cnt = 4096\
};
#else
struct image_info q654_image_info = {\
	.page_size = 2048,\
	.oob_size = 64,\
	.eraseblock_size = 0x20000,\
	.block_cnt = 2048\
};
#endif

struct partition_info_s partition_info[NUM_OF_PARTITION];

struct BootProfileHeader
{
	// 0
	uint32_t    Signature;     // BHDR_BEG_SIGNATURE
	uint32_t    Length;        // 256
	uint32_t    Version;       // 0
	uint32_t    reserved12;

	// 16
	uint8_t     BchType;       // BCH method: 0=1K60, 0xff=BCH OFF
	uint8_t     addrCycle;     // NAND addressing cycle
	uint16_t    ReduntSize;    // depricated, keep it 0
	uint32_t    BlockNum;      // Total NAND block number
	uint32_t    BadBlockNum;   // not used now
	uint32_t    PagePerBlock;  // NAND Pages per Block

	// 32
	uint32_t    PageSize;       // NAND Page size
	uint32_t    NandIDMode;     // not used now
	uint32_t    NandFeatureMode;// not used now
	uint32_t    PlaneSelectMode;// special odd blocks read mode (bit 0: special sw flow en. bit 1 read mode en. bit 2~bit 5 plane select bit addr)

	// 48
	uint32_t    xboot_copies;   // Number of Xboot copies. Copies are consecutive.
	uint32_t    xboot_pg_off;   // page offset (usu. page offset to block 1) to Xboot
	uint32_t    xboot_pg_cnt;   // page count of one copy of Xboot
	uint32_t    reserved60;

	// 64
	//struct OptBootEntry16 opt_entry[10]; // optional boot entries at 80, ..., 224
	uint32_t    AC_Timing0;
	uint32_t    AC_Timing1;
	uint32_t    AC_Timing2;
	uint32_t    AC_Timing3;
	uint32_t    reserved96[40];

	// 240
	uint32_t    reserved240;
	uint32_t    reserved244;
	uint32_t    EndSignature;   // BHDR_END_SIGNATURE
	uint32_t    CheckSum;       // TCP checksum (little endian)

	// 256
};

static unsigned short tcpsum(const unsigned char *buf, unsigned size)
{
	unsigned sum = 0;
	int i;

	/* Accumulate checksum */
	for (i = 0; i < size - 1; i += 2) {
		unsigned short word16 = *(unsigned short *) &buf[i];
		sum += word16;
	}

	/* Handle odd-sized case */
	if (size & 1) {
		unsigned short word16 = (unsigned char) buf[i];
		sum += word16;
	}

	/* Fold to get the ones-complement result */
	while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

	/* Invert to get the negative in ones-complement arithmetic */
	return ~sum;
}

int compose_header(struct BootProfileHeader *hdr)
{
	memset((void *)hdr, 0, sizeof(*hdr));

	// 0
	hdr->Signature     = BHDR_BEG_SIGNATURE;
	hdr->Length        = sizeof(*hdr);
	hdr->Version       = 0;
	//reserved12;

	// 16
	hdr->BchType       = 0xff; // BCH OFF
	//hdr->addrCycle     = sp_get_addr_cycle(nand);

	hdr->ReduntSize    = q654_image_info.oob_size;
	hdr->BlockNum      = q654_image_info.block_cnt;
	hdr->BadBlockNum   = 0; //TODO
	hdr->PagePerBlock  = q654_image_info.eraseblock_size / q654_image_info.page_size;

	// 32
	hdr->PageSize      = q654_image_info.page_size;

	#ifdef CONFIG_SP_SPINAND
	//hdr->PlaneSelectMode = sinfo->plane_sel_mode;
	//printf("PlaneSelectMode=%x\n", hdr->PlaneSelectMode);
	#endif

	// 48
	hdr->xboot_copies  = 1;
	hdr->xboot_pg_off  = q654_image_info.eraseblock_size / \
			     q654_image_info.page_size; // assume xboot is at block 1
	hdr->xboot_pg_cnt  = partition_info[IDX_PARTITION_XBOOT1].file_size / \
			     q654_image_info.usable_page_size;
	//hdr->xboot_pg_cnt = 2;
	//reserved60

	//64
	//hdr->uboot_env_off = get_mtd_env_off(); // u64 hint for "Scanning for env"
	//reserved72
	//reserved76

	//80
	//struct OptBootEntry16 opt_entry[10];
#ifdef CONFIG_PNAND
	hdr->AC_Timing0 = 0x0f1f0f1f;
	hdr->AC_Timing1 = 0x00007f7f;//bit 16 tRLAT
	hdr->AC_Timing2 = 0x7f7f7f7f;
	hdr->AC_Timing3 = 0xff1f001f;
#endif
	//reserved96[40]

	// 240
	//reserved240
	//reserved244
	hdr->EndSignature  = BHDR_END_SIGNATURE;
	hdr->CheckSum      = tcpsum((u8 *)hdr, 252);


}

static void swap_bits(uint8_t *buf, int len)
{
	int i, j;

	for (j = 0; j < len; j++) {
		uint8_t byte = buf[j];

		buf[j] = 0;
		for (i = 0; i < 8; i++) {
			if (byte & (1 << i))
				buf[j] |= (1 << (7 - i));
		}
	}
}

static int write_page(const struct image_info *info, uint8_t *buffer,
		      FILE *src, FILE *dst, struct bch_control *bch)
{
	int steps = info->usable_page_size / info->ecc_step_size;
	int eccbytes = DIV_ROUND_UP(info->ecc_strength * 14, 8);
	off_t pos = ftell(dst);
	size_t pad, cnt;
	int i;
	int ecc_offs=0, data_offs=0;//Real offs in one page of image.
	uint8_t *ecc;
	int one_copy_size;//Data length of one step copy to image, include BI_bytes.

	/* Toggle and ONFI need data size of (sector + ecc parity) is even, Legacy flash not use */
	//if (eccbytes % 2)
		//eccbytes++;

	/* page pad 0xff */
	memset(buffer, 0xff, info->page_size + info->oob_size);
	fwrite(buffer, info->page_size + info->oob_size, 1, dst);

	/* src file end but partation not end, write 0xff page to dst */
	if(feof(src)) {
		return 0;
	} else {
		fseek(dst, pos, SEEK_SET);
	}

	for (i = 0; i < steps; i++) {

		memset(buffer, 0xff, info->ecc_step_size + eccbytes);
		ecc = buffer + info->BI_bytes + info->ecc_step_size;//not impact the pos of ecc in image

		cnt = fread(buffer, 1, info->ecc_step_size, src);// read 1k data from source
		if (!cnt && !feof(src)) {
			fprintf(stderr,
				"Failed to read data from the source\n");
			return -1;
		}

		pad = info->ecc_step_size - cnt;// data less than 1k pad 0xff
		if (pad)
			memset(buffer + cnt, 0xff, pad);

		/* gernerate the ecc data */
		memset(ecc, 0, eccbytes);
		swap_bits(buffer, info->ecc_step_size);
		encode_bch(bch, buffer, info->ecc_step_size, ecc);
		swap_bits(buffer, info->ecc_step_size);
		swap_bits(ecc, eccbytes);

		one_copy_size = info->ecc_step_size;

#ifdef CONFIG_BI //Consider the BBM, for FTNANDC BI function
		if ((info->usable_page_size == info->page_size) && (i == (steps - 1))) {
		/* the part of the last sector data(include parity data) in data region is sec_1st,
		 * the part of the last sector data(include parity data) in spare region is sec_2nd.
		 */
			int sec_2nd = i * eccbytes;
			int sec_1st = info->ecc_step_size - i * eccbytes;
			u8 * temp_for_BI = malloc(info->oob_size); // must be larger than sec_2nd size

			memset(temp_for_BI, 0xff, info->oob_size);
			memcpy(temp_for_BI + info->BI_bytes, buffer + sec_1st, sec_2nd);
	/* FIX: here I don't think about the case that (eccbytes *(step-1) <BI_bytes) */
			memcpy(buffer + sec_1st, temp_for_BI, info->BI_bytes + sec_2nd);

			free(temp_for_BI);

			one_copy_size = info->ecc_step_size + info->BI_bytes;
		}
#endif

		data_offs = i * (info->ecc_step_size + eccbytes);
		ecc_offs = data_offs + one_copy_size;

		/* write the last sector of the page into image */
		fseek(dst, pos + data_offs, SEEK_SET);
		fwrite(buffer, one_copy_size, 1, dst);
		fseek(dst, pos + ecc_offs, SEEK_SET);
		fwrite(ecc, eccbytes, 1, dst);
	}

	/* Make dst pointer point to the next page. */
	fseek(dst, pos + info->page_size + info->oob_size, SEEK_SET);

	return 0;
}

/* every four pages(2k) contains one header
	 * total of 16 backups
	 * -------------------
	 * |Header page (#0) |-|
	 * |reserved page    | |
	 * |reserved page    | |-8192 bytes(not include spare size)
	 * |reserved page    | | = 4 * 2k page = 2 * 4k page = 1 * 8k page
	 * |Header page (#1) |-|
	 * |reserved page    |  The default Page size is 2KB for reading header in iboot.
	 * |reserved page    |  And every four pages(2kB) read one header.So ensure the
	 * |reserved page    |  correct size between the headers when pack header.
	 * |	...          |
	 * |Header page (#7) |
	 * |reserved page    |
	 * |reserved page    |
	 * |reserved page    |
	 * -------------------
	 */
static int write_header(const struct image_info *info, uint8_t *buffer, FILE *dst,
			struct BootProfileHeader *hdr, struct bch_control *bch)
{
	uint8_t *ecc;
	int eccbytes = DIV_ROUND_UP(info->ecc_strength * 14, 8);

	ecc = buffer + info->ecc_step_size;//parity data start with 1KB in page offset

	for (int i = 0; i < info->eraseblock_size/8192; i++) {
		for (int j = 0; j < 8192/info->page_size; j++) {

			memset(buffer, 0xff, info->page_size + info->oob_size);

			if (j == 0)
			{
				memcpy(buffer, hdr, sizeof(*hdr));
				memset(ecc, 0, eccbytes);//encode_bch() specifies

				swap_bits(buffer, info->ecc_step_size);
				encode_bch(bch, buffer, info->ecc_step_size, ecc);
				swap_bits(buffer, info->ecc_step_size);
				swap_bits(ecc, eccbytes);
			}

			fwrite(buffer, info->page_size + info->oob_size, 1, dst);
		}
	}
}

int build_image(void)
{
	char dst_name[32] = "pnand.img";
	u32 target_offset = 0; //target means test file in nand zebu
	u32 source_offset = 0; //source means compile file(xboot uboot and so on)

	FILE *dst, *src;
	u8 *buffer; //temporarily store page data
	u8 *ecc;//temporarily store one ecc data
	off_t page;
	off_t header_pos;
	struct BootProfileHeader hdr;
	struct bch_control *bch;
	u32 page_cnt, use_cnt, copies;
	int steps, eccbytes;

	/* Create the buffer */
	buffer = malloc(q654_image_info.page_size + q654_image_info.oob_size);

	/* Create the image with cmd 'dd' in Makefile, only open the file here */
	dst = fopen(dst_name, "r+b");
	//dst = fopen(dst_name, "wb");
	if (dst == NULL) {
		printf("Error: Can't open %s\n", dst_name);
		exit(-1);
	}

	/* Write each part */
	for (int i = -1; i < NUM_OF_PARTITION; i++) {//

		/*==========================CONFIG BCH======================*/
		/* config bch 1k60bit once when write header.
		 *	1k60bit is used for header, xboot, uboot.
		 * config bch 512/2bit once when write kernel.
		 *	512/2bit is used for kernel/rootfs
		 */
	 	if ((i == -1) || (i == 6)) {
			if (i == -1) {
				q654_image_info.ecc_strength = 60;
				q654_image_info.ecc_step_size = 1024;
			}
			else if (i == 6) {
				q654_image_info.ecc_strength = 2;
				q654_image_info.ecc_step_size = 512;

			}
			bch = init_bch(BCH_GF_ORDER_M, q654_image_info.ecc_strength, BCH_PRIMITIVE_POLY);

			eccbytes = DIV_ROUND_UP(q654_image_info.ecc_strength * 14, 8);
			steps = (q654_image_info.page_size + q654_image_info.oob_size) \
				/ (q654_image_info.ecc_step_size + eccbytes);

			q654_image_info.usable_page_size = steps * q654_image_info.ecc_step_size;
	 	}
		/*==========================CONFIG BCH======================*/

		/* init and write header */
		if (i == -1) {
			//xt_debug("init header\n");
			compose_header(&hdr);
			write_header(&q654_image_info, buffer, dst, &hdr, bch);
			continue;
		}

		/* Set the write postion of each part */
		fseek(dst, partition_info[i].partition_start_addr, SEEK_SET);

		/* skip env,env_redunt,nonos, pad 0xff */
		if ((i > 3) && (i < 6)) {
			while (ftell(dst) < partition_info[i].partition_start_addr +
			       partition_info[i].partition_size) {
				memset(buffer, 0xff, q654_image_info.page_size + q654_image_info.oob_size);
				fwrite(buffer, q654_image_info.page_size + q654_image_info.oob_size, 1, dst);
			}
			continue;
		}

		src = fopen(partition_info[i].file_name, "r");
		if (src == NULL) {
			printf("Error: Can't open %s\n", partition_info[i].file_name);
			exit(-1);
		}
#if 0
		/* One block store as many copies as possible. Currently, this function
		 * is valid when the size of xboot.img is smaller than usable_block_size(64KB).
		 * after Q628, xboot.img compose of xboot and dram data(>64KB). So copies not use?
		 */
		page_cnt = q654_image_info.eraseblock_size / q654_image_info.page_size;
		use_cnt = partition_info[i].file_size / q654_image_info.usable_page_size;
		copies = page_cnt / use_cnt;
		if(copies == 0) // uboot roofs kernel
			copies = 1;
#endif
		copies = 1;
		while(copies--) {
			fseek(src, 0, SEEK_SET);

			if (copies > 1) {
				while (!feof(src))
					write_page(&q654_image_info, buffer, src, dst, bch);
			} else {
				while (ftell(dst) < partition_info[i].partition_start_addr +
				       partition_info[i].partition_size)
					write_page(&q654_image_info, buffer, src, dst, bch);
			}
		}

		fclose(src);
	}
#if 0
	printf("----------------------------\n");
	printf("TEST read spare, 2048+64 bytes pad 0xff\n");
	printf("----------------------------\n");
	fseek(dst, partition_info[0].partition_start_addr, SEEK_SET);

	memset(buffer, 0xff, q654_image_info.page_size + q654_image_info.oob_size);
	fwrite(buffer, q654_image_info.page_size + q654_image_info.oob_size, 1, dst);
#endif
	free(buffer);
	fclose(dst);
	return 1;
}

void set_partition_info(void)
{
	//u8 ecc[105];
	struct stat file_stat;
	int i;
	u64 u64_temp;
	u32 next_offs; //Cal the next partition_start_addr
	u32 block_cnt; //The actual block cnt used
	u32 page_per_blk; //page cnt of one block
	u32 file_blk_sz; //The block size in file
#if 1
	u8 *file_name[] = {"xboot1", "uboot1", "uboot2","fip", "env", "env_redund",\
		 "dtb", "kernel", "rootfs"};
#else
	u8 *file_name[] = {"nand_1k.bin"};
#endif
	page_per_blk = q654_image_info.eraseblock_size / q654_image_info.page_size;
	file_blk_sz = (q654_image_info.page_size + q654_image_info.oob_size) * page_per_blk;
	next_offs = file_blk_sz;//after header

	for (i = 0; i < NUM_OF_PARTITION; i++) {
		/* set file_name for each part */
		strcpy(partition_info[i].file_name, file_name[i]);

		xt_debug("[%s]\n", partition_info[i].file_name);

		/* set start_addr in nand file for each part */
		partition_info[i].partition_start_addr = next_offs;

		xt_debug("Start address:\t0x%x\n", partition_info[i].partition_start_addr);

		/* skip env, env_redund, reserve, the file not exist temporary */
		if ((i < 4) || (i > 5)) {
			/* get the src file stat */
			if (stat(partition_info[i].file_name, &file_stat) != 0) {
				printf("File not found: %s\n", partition_info[i].file_name);
				exit (-1);
			}
			/* set file_size and align it to usable_page_size(1k) */
			u64_temp = file_stat.st_size;
			u64_temp = (u64_temp + 0x3ff) & 0xfffffffffffffc00UL;
			truncate(partition_info[i].file_name, u64_temp);

			partition_info[i].file_size = u64_temp;

			xt_debug("File size:\t0x%lx, extend it to 0x%lx\n", file_stat.st_size, partition_info[i].file_size);

#ifdef AUTO_LAYOUT_ONE_BY_ONE//only used for xboot/uboot. do not implement kernel rootfs.
			/* cal the block_cnt that the partition used */
			if (i < 4) {
				printf("%d %d \n",q654_image_info.usable_page_size , q654_image_info.page_size);
				u64_temp = u64_temp / q654_image_info.usable_page_size * q654_image_info.page_size; // u64_temp*2
			}
			block_cnt = DIV_ROUND_UP(u64_temp, q654_image_info.eraseblock_size);
		} else {
			block_cnt = 0;
			if (i == 5)
				block_cnt = 16;//512 + 512 + 1024 = 2048 = 16 blocks,temp set
		}

#else //fixed partition_size
		}
		/* Partition	 Start_Address	   Size(KB)
		 *
		 * nand_header	  0x0		   128//0x20000
		 * xboot1	  0x20000	   384//0x60000
		 * uboot1	  0x80000	  1536//0x180000
		 * uboot2	  0x200000	  2048//0x200000
		 * reserve/nonos  0x400000	  1024//0x200000
		 * env		  0x600000	   512//0x80000
		 * env_redund	  0x680000	   512//0x80000
		 * dtb		  0x700000	   256//0x40000
		 * kernel	  0x840000	 25600//0x1900000
		 * rootfs	  0x1f40000	 33536//0x1f40000          230144//0xe0c0000
		 */
		u32 part_size[9] = {0x60000, 0x160000, 0x220000,0x200000, 0x80000, 0x80000,\
				 0x40000, 0x1900000, 0x1f40000};

		block_cnt = DIV_ROUND_UP(part_size[i], q654_image_info.eraseblock_size);
#endif
		xt_debug("block_cnt:\t%d\n", block_cnt);

		/* set partition_size */
		partition_info[i].partition_size = block_cnt * file_blk_sz;
		xt_debug("Part size:\t0x%lx\n", partition_info[i].partition_size);

		/* record the next partition addr */
		next_offs += partition_info[i].partition_size;
	}
}

int main()
{
#ifdef CONFIG_BI
	q654_image_info.BI_bytes	= 2;
#else
	q654_image_info.BI_bytes	= 0;
#endif

	set_partition_info();

	build_image();
}
