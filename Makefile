.PHONY: clean distclean all config

BIN        := bin
SPI_ALL    := spi_all.bin
CFG        := pack.conf
ISP_IMG    := ISPBOOOT.BIN
EMMC_BOOT1 := emmc_boot1.hex
EMMC_USER  := emmc_user0.hex

ZEBU_RUN ?= 0
BOOT_KERNEL_FROM_TFTP ?= 0
TFTP_SERVER_PATH ?= 0
CHIP ?= Q628
ARCH ?= arm
ifeq ($(CHIP),Q645)
ARCH=arm64
endif
ifeq ($(CHIP),SP7350)
ARCH=arm64
endif


all: $(SPI_ALL)

config:
	@./configure.sh

$(SPI_ALL):
	make config
	DXTOR=0 fakeroot ./update_all.sh $(SPI_ALL) $(ZEBU_RUN) $(BOOT_KERNEL_FROM_TFTP) $(CHIP) $(ARCH) $(NOR_JFFS2)
	@if [ "$(BOOT_KERNEL_FROM_TFTP)" = '1' ]; then \
		if [ "$(CHIP)" = "I143" ]; then  \
			./copy2tftp_riscv.sh $(TFTP_SERVER_PATH);\
		else \
			./copy2tftp.sh $(TFTP_SERVER_PATH);\
		fi;\
	fi

###############################
# Pack for isp boot testing
isp: all
	@echo "Build $(ISP_IMG)"
	@dd if=bin/xboot.img of=$(BIN)/$(ISP_IMG)
	@if [ "$(CHIP)" = "Q645" -o "$(CHIP)" = "SP7350" ]; then \
		dd if=bin/u-boot.img of=$(BIN)/$(ISP_IMG) conv=notrunc bs=1k seek=192 ; \
	else \
		dd if=bin/u-boot.img of=$(BIN)/$(ISP_IMG) conv=notrunc bs=1k seek=64 ; \
	fi;

###############################
# Pack for SPI-NOR boot testing
nor_hex:
	@if [ ! -f $(BIN)/$(SPI_ALL) ]; then echo "No input : $(BIN)/$(SPI_ALL)" ; exit 1 ; fi
	@if [ "$(CHIP)" = "SP7350" ]; then \
		echo "* Gen NOR Hex : Q654_run.hex" ; \
		./tools/gen_hex.sh $(BIN)/$(SPI_ALL) $(BIN)/Q654_run.hex ; \
	else \
		echo "* Gen NOR Hex : $(CHIP)_run.hex" ; \
		./tools/gen_hex.sh $(BIN)/$(SPI_ALL) $(BIN)/$(CHIP)_run.hex ; \
	fi;

###############################
# Create SD card hex file for Zebu
# 1. Run 'make config' to configure compilation environment for booting from SD card.
# 2. Run 'make' to build all, including image file of SD card: 'out/boot2linux_SDcard/ISP_SD_BOOOT.img'.
# 3. Copy image file 'out/boot2linux_SDcard/ISP_SD_BOOOT.img' to 'ipack/disk/sd64m.bin'.
# 4. Run 'cd ipack' to go to folder 'ipack'.
# 5. Run 'make sd_hex' to convert binary file 'disk/sd64m.bin' to hex file 'disk/sd_image.hex'.
# 6. Transfer hex file 'sd_image.hex' to zebu folder 'hex_files/MMC/'.
DISK_IN=disk/sd64m.bin
DISK_OUT=disk/sd_image.hex
sd_hex:
	@if [ ! -f $(DISK_IN) ];then echo "No input : $(DISK_IN)" ; exit 1 ; fi;
	@hexdump -v -e '1/1 "%02x\n"' $(DISK_IN) > $(DISK_OUT)
	@ls -l $(DISK_OUT)

###############################
# Pack for SPI-NAND boot testing
NAND_RAW=nand/nand32mb.raw
NAND_ZEBU_BIN=nand/nand32mb.raw.zebu
NAND_ZEBU_HEX=nand/nand.hex
# tool to convert 2K-page-nand raw bin (include oob) to zebu 4kb-aligned page nand
CONV_ZEBU_NAND=./tools/pack_zebu_nand/pack_zebu_nand
nand_hex:
	@if [ ! -f $(NAND_RAW) ];then echo "No input : $(NAND_RAW)" ; exit 1 ; fi;
	@rm -f $(NAND_ZEBU_BIN)
	$(CONV_ZEBU_NAND) $(NAND_RAW) $(NAND_ZEBU_BIN)
	@echo "* Gen zebu NAND hex $(NAND_ZEBU_HEX) ..."
	@hexdump -v -e '1/1 "%02x\n"' $(NAND_ZEBU_BIN) > $(NAND_ZEBU_HEX)
	@rm -f $(NAND_ZEBU_BIN)
	@ls -l $(NAND_ZEBU_HEX)

###############################
# Pack for Parallel-NAND boot testing
PNAND_TOOL=tools/nand_image_builder
PNAND_IMAGE=$(PNAND_TOOL)/pnand.img
PNAND_HEX=nand/pnand.hex
pnand_hex:
	@make -C $(PNAND_TOOL) all
	@if [ ! -f $(PNAND_IMAGE) ]; then \
		echo "No input : $(PNAND_IMAGE)" ; \
		exit 1 ; \
	fi
	@echo "* Gen zebu PNAND hex $(PNAND_HEX) ..."
	@hexdump -v -e '1/1 "%02x\n"' $(PNAND_IMAGE) > $(PNAND_HEX)
	@ls -l $(PNAND_HEX)
# The 4KB page device design itself determines the hex file format
pnand_hex_4k:
	@make -C $(PNAND_TOOL) all
	@if [ ! -f $(PNAND_IMAGE) ]; then \
		echo "No input : $(PNAND_IMAGE)" ; \
		exit 1 ; \
	fi
	@echo "* Gen zebu PNAND hex $(PNAND_HEX) ..."
	@hexdump -v -e '1/4 "%08x\n"' $(PNAND_IMAGE) > $(PNAND_HEX)
	@ls -l $(PNAND_HEX)

###############################
# Pack for emmc boot testing
emmc_hex:
	@echo ""
	@echo "* Gen eMMC boot1 hex: $(EMMC_BOOT1)"
	@hexdump -v -e '1/1 "%02x\n"' ../out/xboot.img > $(BIN)/$(EMMC_BOOT1)
	@ls -l $(BIN)/$(EMMC_BOOT1)
	@echo ""
	@echo "* Gen eMMC user hex: $(EMMC_USER)"
	@dd if=emmc_gpt/lba0.bin of=$(BIN)/emmc_user0.bin              bs=512 >/dev/null 2>&1
	@dd if=emmc_gpt/lba1.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=1 >/dev/null 2>&1
	@dd if=emmc_gpt/lba2.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=2 >/dev/null 2>&1
	@dd if=emmc_gpt/lba3.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=3 >/dev/null 2>&1
	@dd if=emmc_gpt/lba4.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=4 >/dev/null 2>&1
	@dd if=../out/u-boot.img of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x22) >/dev/null 2>&1
	@dd if=../out/u-boot.img of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x822) >/dev/null 2>&1
	@if [ "$(CHIP)" = "I143" ]; then \
		dd if=../out/freertos.img of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x1822) >/dev/null 2>&1 ; \
	fi;
	@dd if=../out/dtb of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x2022) >/dev/null 2>&1
	@dd if=../out/uImage of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x2222) >/dev/null 2>&1
	@dd if=../out/rootfs.img of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x12222) >/dev/null 2>&1
	@hexdump -v -e '1/1 "%02x\n"' $(BIN)/emmc_user0.bin > $(BIN)/$(EMMC_USER)
	@ls -l $(BIN)/$(EMMC_USER)

zebu_hex_dxtor:
	@./configure.sh x # x for zebu
	DXTOR=1 bash ./update_all.sh $(SPI_ALL) 1 0 $(CHIP) $(ARCH) 0
	@echo ""

zebu_hex_fakedram:
	@./configure.sh x # x for zebu
	DXTOR=0 bash ./update_all.sh $(SPI_ALL) 1 0 $(CHIP) $(ARCH) 0
	@echo ""

dxtor:
	@make DXTOR=1
TO_RM   := $(SPI_ALL) $(ISP_IMG) *.hex \
	   bootRom.bin xboot.img u-boot.img ecos.img ecos.img.orig uImage vmlinux vmlinux.bin \
	   dtb dtb.img \
	   emmc_user0.bin

clean:
	make -C tools/bin2zmem $@
	rm -f $(CFG)
	cd $(BIN) && rm -f $(TO_RM)
	-rm -f $(DISK_OUT) $(NAND_ZEBU_BIN) $(NAND_ZEBU_HEX)

distclean: clean
	cd $(BIN) && rm -f *.img *.bin
