#!/bin/bash
#Example of building Binaries (ISPBOOOT.BIN, ISP_UPDT.BIN):

export PATH="$PATH:../build/tools/isp"

SRC_DIR=${HOME}/qac628/ipack/bin
cp ${SRC_DIR}/xboot.img xboot0
cp ${SRC_DIR}/xboot.img xboot1
cp ${SRC_DIR}/u-boot.img uboot0
cp ${SRC_DIR}/u-boot.img uboot1
cp ${SRC_DIR}/u-boot.img uboot2
cp ${SRC_DIR}/dtb.img dtb

isp pack_image ISPBOOOT.BIN \
	xboot0 uboot0 \
	xboot1 0x100000 \
	uboot1 0x100000 \
	uboot2 0x100000 \
	env 0x80000 \
	env_redund 0x80000 \
	dtb 0x10000 \
	${SRC_DIR}/uImage 0xa00000 \

rm -rf xboot0
rm -rf xboot1
rm -rf uboot0
rm -rf uboot1
rm -rf uboot2
rm -rf dtb
mv ISPBOOOT.BIN ${SRC_DIR}
