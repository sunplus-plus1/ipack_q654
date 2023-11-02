#!/bin/bash
#
# script for generating jffs2 root file-system (rootfs) for SPI-NOR
#

SPI_NOR_SIZE=$2

LINUX=uImage
ROOTFS_IMG=bin/rootfs.img
ROOTFS_DIR=../linux/rootfs/initramfs/disk
JFFS2="mkfs.jffs2"

# Get size of Linux image (in unit of byte).
kernel_sz=`du -sb bin/$LINUX | cut -f1`

# Align size of Linux image to 64 boundary (in unit of 1 kbytes)
kernel_sz_1k=$((((kernel_sz+65535)/65536)*64))

# Calculate offset of rootfs.
rootfs_offset=$((kernel_sz_1k+2048+128))

# Calculate size of rootfs
rootfs_sz=$((SPI_NOR_SIZE*1024*1024-rootfs_offset*1024))

# Check if rootfs directory exists?
if [ ! -d $ROOTFS_DIR ]; then
	echo "\E[1;31mError: $ROOTFS_DIR doesn't exist!\E[0m"
	exit 1
fi

# Remove old rootfs image.
rm -f $ROOTFS_IMG

# Create jffs2 rootfs image.
# page-size = 1024, erase-block-size = 64k
echo -e  "\E[1;33m ========make jffs2 fs========== \E[0m"
echo "$JFFS2 -s 0x1000 -e 0x10000 -d "$ROOTFS_DIR" -o $ROOTFS_IMG"
$JFFS2 -s 0x1000 -e 0x10000 -d "$ROOTFS_DIR" -o $ROOTFS_IMG

# Get real size and percentage of rootfs image.
rootfs_sz2=`du -sk $ROOTFS_IMG | cut -f1`
rootfs_sz3=$((rootfs_sz2*1024))
rootfs_percentage=$(((rootfs_sz3*100)/$rootfs_sz))
echo -e "\E[1;33mSize of $ROOTFS_IMG (jffs2) is $rootfs_sz2 kbytes ($rootfs_percentage%)\E[0m"

# check rootfs image size
if [ $rootfs_sz3 -gt $rootfs_sz ]; then
	echo -e "\E[1;31mError: Size of $ROOTFS_IMG is too big!\E[0m"
	exit 1
fi
