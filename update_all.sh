#!/bin/bash

#./update_me.sh <source_img>
source ../.config

export PATH="../crossgcc/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/:$PATH"

IMG_OUT=$1
ZEBU_RUN=$2
BOOT_KERNEL_FROM_TFTP=$3
CHIP=$4
ARCH=$5
NOR_JFFS2=$6

if [ -n "$7" ]; then
	FLASH_SIZE=$7
else
	FLASH_SIZE=16	# default size = 16 MiB
fi

if [ "IMG_OUT" = "" ];then
	echo "Error: no output file name"
	exit 1
fi

if [ -f colors.env ];then
	. colors.env
fi

if [ -f pack.conf ];then
	. pack.conf
fi

# $1: filename
warn_no_up()
{
	echo -e "${YELLOW}WARN: $1 isn't updated${NC}"
}

# $1: filename
exit_no_file()
{
	[ ! -f $1 ] && echo -e "${RED}WARN: $1 is not found${NC} (make config?)" && exit 1
}

# $1: filename
warn_up_ok()
{
	echo -e "${CYAN} $1 is updated ${NC} from your source"
}

####################
BOOTROM=bootRom.bin
XBOOT=xboot.img
UBOOT=u-boot.img
FIP=fip.img
NONOS=rom.img
ROOTFS=rootfs.img
LINUX=uImage

kernel_max_size=$(($FLASH_SIZE*0x100000-0x220000))

if [ "$ZEBU_RUN" = "1" ];then
	VMLINUX=vmlinux   # Use uncompressed uImage (qkboot + uncompressed vmlinux)
else
	VMLINUX=          # Use compressed uImage
fi
DTB=dtb
FREEROTS=freertos
OPENSBI_KERNEL=OpenSBI_Kernel.img
KPATH=linux/kernel/

echo "* Update from source images..."
if [ "$pf_type" = "s" ];then
	./update_me.sh ../boot/iboot/iboot_q654/bin/$BOOTROM && warn_up_ok $BOOTROM
	./update_me.sh ../boot/xboot/bin/$XBOOT   && warn_up_ok $XBOOT
elif [ "$pf_type" = "x" ];then
	./update_me.sh ../boot/xboot/bin/$XBOOT   && warn_up_ok $XBOOT
fi
./update_me.sh ../boot/uboot/$UBOOT  && warn_up_ok $UBOOT

if [ "$VMLINUX" = "" ];then
	./update_me.sh ../$KPATH/arch/$ARCH/boot/$LINUX  || warn_no_up $LINUX
else
	./update_me.sh ../$KPATH/$VMLINUX && warn_up_ok $VMLINUX
	echo "*******************************"
	echo "* Create $LINUX from $VMLINUX"
	echo "*******************************"
	aarch64-none-linux-gnu-objcopy -O binary -S bin/$VMLINUX bin/$VMLINUX.bin
	./add_uhdr.sh linux-`date +%Y%m%d-%H%M%S` bin/$VMLINUX.bin bin/$LINUX $ARCH 0x800000 0x800000 kernel
	if [ "$SECURE" = "1" ]; then
		make -C ../boot/uboot/board/sunplus/common/secure_sp7350 sign IMG=$(realpath bin/$LINUX) || exit 1
	fi
fi

if [ "$DTB" != "" ];then
	echo "*******************************"
	echo "* Create dtb.img from $DTB"
	echo "*******************************"
	./update_me.sh ../$KPATH/$DTB && warn_up_ok $DTB
	if [ "$VMLINUX" = "" ];then
		# If we use uImage, not needed to add sp header.
		cp bin/$DTB bin/dtb.img
	else
		./add_uhdr.sh dtb-`date +%Y%m%d-%H%M%S` bin/$DTB bin/dtb.img $ARCH
	fi
fi

./update_me.sh ../boot/trusted-firmware-a/build/$FIP && warn_up_ok $FIP

echo "* Check image..."
# without iboot: use romcode iboot
#exit_no_file bin/$BOOTROM
exit_no_file bin/$XBOOT

if [ "$ZEBU_RUN" = "0" ]; then
	echo ""
	echo "* Gen NOR image: $IMG_OUT ..."
	if [ -f bin/$BOOTROM ]; then
		dd if=bin/$BOOTROM of=bin/$IMG_OUT
	else
		rm -f bin/$IMG_OUT
	fi

	dd if=bin/$XBOOT  of=bin/$IMG_OUT conv=notrunc bs=1k seek=96
	dd if=bin/dtb.img of=bin/$IMG_OUT conv=notrunc bs=1k seek=288
	dd if=bin/$UBOOT  of=bin/$IMG_OUT conv=notrunc bs=1k seek=416
	dd if=bin/$FIP    of=bin/$IMG_OUT conv=notrunc bs=1k seek=1184

	if [ "$BOOT_KERNEL_FROM_TFTP" != "1" ]; then

		dd if=bin/$LINUX of=bin/$IMG_OUT conv=notrunc bs=1k seek=2048

		if [ "$NOR_JFFS2" == "1" ]; then
			# Generate jffs2 rootfs for SPI-NOR
			bash ./gen_nor_jffs2.sh $CHIP $FLASH_SIZE
			if [ $? -ne 0 ]; then
				exit 1;
			fi

			# Get size of Linux image (in unit of byte).
			kernel_sz=`du -sb bin/$LINUX | cut -f1`

			# Align size of Linux image to 64 boundary (in unit of 1 kbytes)
			kernel_sz_1k=$((((kernel_sz+65535)/65536)*64))

			# Calculate offset of rootfs.
			rootfs_offset=$((kernel_sz_1k+2048))

			dd if=bin/$ROOTFS of=bin/$IMG_OUT conv=notrunc bs=1k seek=$rootfs_offset

			ls -l bin/$IMG_OUT
		else
			# Check linux image size
			kernel_sz=`du -sb bin/$LINUX | cut -f1`
			if [[ $kernel_sz -gt $kernel_max_size ]]; then
				echo -e "${YELLOW}Warning: $LINUX size ($kernel_sz) is too big. Need bigger SPI_NOR flash (> ${FLASH_SIZE}MiB)!${NC}"
			fi
		fi
	fi
else
	B2ZMEM=./tools/bin2zmem/bin2zmem
	ZMEM_HEX=./bin/zmem.hex
	make -C ./tools/bin2zmem

	# Set DXTOR=1 to gen DRAM XTOR hex. Otherwise, gen for fake dram hex.
	DXTOR=${DXTOR-1}
	echo ""
	if [ "$DXTOR" = "1" ];then
		echo -e "* Gen ZMEM : $ZMEM_HEX ... (${YELLOW}DRAM XTOR${NC})"
	else
		echo -e "* Gen ZMEM : $ZMEM_HEX ... (${CYAN}FAKE DRAM${NC})"
	fi
	rm -f $ZMEM_HEX
	#        in               out           in_skip   DRAM_off
	# Gen Q645_run.hex or Q654_run.hex for xboot.img
	if [ -f bin/$BOOTROM ]; then
		dd if=bin/$BOOTROM     of=bin/$IMG_OUT
	else
		rm -f bin/$IMG_OUT
	fi
	dd if=bin/$XBOOT of=bin/$IMG_OUT conv=notrunc bs=1k seek=96
	if [ "$CHIP" == "Q645" ]; then
		./tools/gen_hex.sh bin/$IMG_OUT bin/Q645_run.hex
	else
		./tools/gen_hex.sh bin/$IMG_OUT bin/Q654_run.hex
	fi

	# Gen zmem*.hex
	rm -f ./bin/zmem*.hex
	ZMEM_HEX=./bin/zmem0a.hex
	#B2ZMEM=./tools/bin2zmem/bin2zmem_ddr4.sh
	B2ZMEM=./tools/bin2zmem/bin2zmem_q645
	M4=../firmware/arduino_core_sunplus/bin/firmware.bin

	DXTOR=1
	$B2ZMEM  bin/$FIP         $ZMEM_HEX     0x0       0x1000000             $DXTOR # 16MB (fip load address)
	$B2ZMEM  bin/$UBOOT       $ZMEM_HEX     0x0       0x0500000             $DXTOR # 5MB  (uboot before relocation)
	$B2ZMEM  bin/dtb.img      $ZMEM_HEX     0x0       $((0x1f80000 - 0x40)) $DXTOR # 31MB + 512KB - 64
	$B2ZMEM  bin/$LINUX       $ZMEM_HEX     0x0       $((0x2000000 - 0x40)) $DXTOR # 32MB - 64
	$B2ZMEM  bin/$UBOOT       $ZMEM_HEX     0x0       0xff00000             $DXTOR # 255MB (uboot after relocation)
	$B2ZMEM  $M4              $ZMEM_HEX     0x0       0x77000000            $DXTOR
	zmem_kernel_max_size=$((0xdf00000))

	ls -lh $ZMEM_HEX

	# check linux image size
	kernel_sz=`du -sb bin/$LINUX | cut -f1`
	if [[ $kernel_sz -gt ${zmem_kernel_max_size} ]]; then
		echo -e "${RED}Error: $LINUX size ($kernel_sz) is too big in ZMEM arrangement!${NC}"
		exit 1
	fi
	echo -e "# check linux image size ok"
fi
