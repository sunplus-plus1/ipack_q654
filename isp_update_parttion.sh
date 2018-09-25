#!/bin/bash
#Example of building Binaries (ISPBOOOT.BIN, ISP_UPDT.BIN):

export PATH="$PATH:../build/tools/isp"

SRC_DIR=${HOME}/qac628/ipack/bin
isp extract4update ${SRC_DIR}/ISPBOOOT.BIN ${SRC_DIR}/ISP_UPDT.BIN $@
echo $@
isp extract4tftpupdate ${SRC_DIR}/ISPBOOOT.BIN ${SRC_DIR}/ISP_TFTP $@
echo $@
