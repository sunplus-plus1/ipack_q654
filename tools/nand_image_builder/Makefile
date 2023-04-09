TOPDIR = $(abspath .)

XBOOT = $(TOPDIR)/../../../boot/xboot/bin/xboot.img
UBOOT = $(TOPDIR)/../../../boot/uboot/u-boot.img
FIP = $(TOPDIR)/../../../boot/trusted-firmware-a/build/fip.img
DTB = $(TOPDIR)/../../../linux/kernel/dtb
KERNEL = $(TOPDIR)/../../../linux/kernel/arch/arm64/boot/uImage
ROOTFS = $(TOPDIR)/../../../linux/rootfs/rootfs.img

PNAND_IMG = pnand.img
PNAND_ERR = pnand_err.img
PNAND_HEX = pnand.hex

CC = gcc
CFLAGS = -lm

CSOURCES = bch.c generate_image.c
TEST = random_error_bit.c

OBJS = $(CSOURCES:.c=.o)
DEPS = $(OBJS:.o=.d)

.PHONY : all clean cp_bin gen_img run err_bit

all:
	@make cp_bin
	@make gen_img
	@make run

run: gen_img
	@tr '\000' '\377' < /dev/zero | dd iflag=fullblock of=pnand.img bs=1M count=264
	@./gen_img
	@split -b 256M $(PNAND_IMG)

cp_bin:
	@cp $(XBOOT) xboot1
	@cp $(UBOOT) uboot1
	@cp $(UBOOT) uboot2
	@cp $(DTB) dtb
	@cp $(FIP) fip
	@cp $(KERNEL) kernel
	@cp $(ROOTFS) rootfs

err_bit: random_error_bit.o
	@cp $(PNAND_IMG) $(PNAND_ERR)
	@$(CC) $(CFLAGS) -o $@ $^
	@./err_bit
	@if [ ! -f $(PNAND_ERR) ]; then \
		echo "No input : $(PNAND_ERR)" ; \
		exit 1 ; \
	fi
	@echo "* Gen zebu PNAND hex $(PNAND_HEX) ..."
	@hexdump -v -e '1/1 "%02x\n"' $(PNAND_ERR) > $(PNAND_HEX)
	@ls -l $(PNAND_HEX)

gen_img: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.d: %.c
	set -e; rm -f $@; $(CC) -MM $< $(INCLUDEFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean :
	-rm -f *.o *.d.* *.d uboot1 uboot2 xboot1 kernel dtb gen_img $(PNAND_IMG)

-include $(DEPS)
