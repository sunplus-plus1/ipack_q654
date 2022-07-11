TOPDIR = $(abspath .)

XBOOT = $(TOPDIR)/../../../boot/xboot/bin/xboot.img
UBOOT = $(TOPDIR)/../../../boot/uboot/u-boot.img
DTB = $(TOPDIR)/../../../linux/kernel/dtb
KERNEL = $(TOPDIR)/../../../linux/kernel/arch/arm64/boot/uImage

CC = gcc
CFLAGS = -lm

CSOURCES = bch.c generate_image.c

OBJS = $(CSOURCES:.c=.o)
DEPS = $(OBJS:.o=.d)

.PHONY : all clean cp_bin gen_img run

all:
	make cp_bin
	make gen_img
	make run

run: gen_img
	./gen_img

cp_bin:
	cp $(XBOOT) xboot1
	cp $(UBOOT) uboot1
	cp $(UBOOT) uboot2
	cp $(DTB) dtb
	cp $(KERNEL) kernel

gen_img: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.d: %.c
	@set -e; rm -f $@; $(CC) -MM $< $(INCLUDEFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean :
	-rm -f *.o *.d.* *.d uboot1 uboot2 xboot1 kernel dtb gen_img pnand.img

-include $(DEPS)
