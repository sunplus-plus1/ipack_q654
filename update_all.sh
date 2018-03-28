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
elif [ "$pf_type" = "x" ];then
	./update_me.sh ../boot/xboot/bin/$XBOOT   && warn_up_ok $XBOOT
fi
./update_me.sh ../boot/uboot/$UBOOT  && warn_up_ok $UBOOT
#./update_me.sh ../ecos/bin/$ECOS  && warn_up_ok $ECOS

if [ "$VMLINUX" = "" ];then
	./update_me.sh ../linux/kernel/arch/arm/boot/$LINUX  || warn_no_up $LINUX
else
	./update_me.sh ../linux/k4/$VMLINUX && warn_up_ok $VMLINUX
	echo "*******************************"
	echo "* Create $LINUX from $VMLINUX"
	echo "*******************************"
        armv5-glibc-linux-objcopy -O binary -S bin/$VMLINUX bin/$VMLINUX.bin
	./add_uhdr.sh linux-`date +%Y%m%d-%H%M%S` bin/$VMLINUX.bin bin/$LINUX 0x308000 0x308000
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

echo ""
echo "* Gen NOR image: $IMG_OUT ..."
dd if=bin/$BOOTROM     of=bin/$IMG_OUT
dd if=bin/$XBOOT       of=bin/$IMG_OUT conv=notrunc bs=1k seek=64
dd if=bin/dtb.img       of=bin/$IMG_OUT conv=notrunc bs=1k seek=128
dd if=bin/$UBOOT       of=bin/$IMG_OUT conv=notrunc bs=1k seek=256
#dd if=bin/$ECOS        of=bin/$IMG_OUT conv=notrunc bs=1M seek=1
dd if=bin/$LINUX       of=bin/$IMG_OUT conv=notrunc bs=1M seek=6

ls -lh bin/$IMG_OUT

B2ZMEM=./tools/bin2zmem/bin2zmem
ZMEM_HEX=./bin/zmem.hex
echo ""
echo "* Gen ZMEM : $ZMEM_HEX ..."
rm -f $ZMEM_HEX
#        in               out           in_skip     DRAM_off
$B2ZMEM  bin/$XBOOT       $ZMEM_HEX     0x0       0x0001000             # 4KB
#$B2ZMEM  bin/$ECOS        $ZMEM_HEX     0x0       0x0010000             # 64KB
$B2ZMEM  bin/$UBOOT       $ZMEM_HEX     0x0       0x0200000             # 2MB  (uboot before relocation)
$B2ZMEM  bin/dtb.img      $ZMEM_HEX     0x0       0x0300000             # 3MB
$B2ZMEM  bin/$LINUX       $ZMEM_HEX     0x0       $((0x0308000 - 0x40)) # 3MB + 32KB - 64
$B2ZMEM  bin/$UBOOT       $ZMEM_HEX     0x0       0x1F00000             # 31MB (uboot after relocation)
ls -lh $ZMEM_HEX
