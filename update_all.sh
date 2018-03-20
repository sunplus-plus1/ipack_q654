#./update_me.sh <source_img>

export PATH="../build/tools/armv5-eabi--glibc--stable/bin/:$PATH"


IMG_OUT=$1

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
#DRAMINIT=draminit.img
UBOOT=u-boot.img
ECOS=ecos.img
LINUX=uImage
#VMLINUX=          # mark to use uImage
VMLINUX=vmlinux    # create customized uImage from vmlinux
DTB=dtb

# Use uncompressed version first
if [ -f ../ecos/bin/$ECOS.orig ];then
	ECOS=$ECOS.orig
fi

echo "* Update from source images..."
if [ "$pf_type" = "s" ];then
	./update_me.sh ../boot/iboot/bin/$BOOTROM && warn_up_ok $BOOTROM
	./update_me.sh ../boot/xboot/bin/$XBOOT   && warn_up_ok $XBOOT
	#./update_me.sh ../boot/dram_init/bin/$DRAMINIT
elif [ "$pf_type" = "x" ];then
	./update_me.sh ../boot/xboot/bin/$XBOOT   && warn_up_ok $XBOOT
fi
#./update_me.sh ../boot/uboot/$UBOOT  && warn_up_ok $UBOOT
#./update_me.sh ../ecos/bin/$ECOS  && warn_up_ok $ECOS

if [ "$VMLINUX" = "" ];then
	./update_me.sh ../linux/kernel/arch/arm/boot/$LINUX  || warn_no_up $LINUX
else
	./update_me.sh ../linux/k4/$VMLINUX && warn_up_ok $VMLINUX
	echo "*******************************"
	echo "* Create $LINUX from $VMLINUX"
	echo "*******************************"
        armv5-glibc-linux-objcopy -O binary -S bin/$VMLINUX bin/$VMLINUX.bin
	./add_uhdr.sh linux-`date +%Y%m%d-%H%M%S` bin/$VMLINUX.bin bin/$LINUX 0x208000 0x208000
fi

if [ "$DTB" != "" ];then
	echo "*******************************"
	echo "* Create dtb.img from $DTB"
	echo "*******************************"
	./update_me.sh ../linux/k4/$DTB && warn_up_ok $DTB
	./add_uhdr.sh dtb-`date +%Y%m%d-%H%M%S` bin/$DTB bin/dtb.img
fi

echo "* Check image..."

# without iboot: use romcode iboot
#exit_no_file bin/$BOOTROM
exit_no_file bin/$XBOOT

echo "* Gen $IMG_OUT ..."
dd if=bin/$BOOTROM     of=bin/$IMG_OUT
dd if=bin/$XBOOT       of=bin/$IMG_OUT conv=notrunc bs=1k seek=64
#dd if=bin/$DRAMINIT    of=bin/$IMG_OUT conv=notrunc bs=1k seek=128
dd if=bin/dtb.img       of=bin/$IMG_OUT conv=notrunc bs=1k seek=128
dd if=bin/$UBOOT       of=bin/$IMG_OUT conv=notrunc bs=1k seek=256
#dd if=bin/$ECOS        of=bin/$IMG_OUT conv=notrunc bs=1M seek=1
dd if=bin/$LINUX       of=bin/$IMG_OUT conv=notrunc bs=1M seek=6

ls -lh bin/$IMG_OUT
