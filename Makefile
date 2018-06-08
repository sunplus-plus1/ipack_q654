.PHONY: clean distclean all config

BIN        := bin
SPI_ALL    := spi_all.bin
SPI_HEX    := Q628_run.hex
CFG        := pack.conf
ISP_IMG    := ispbooot.BIN
EMMC_BOOT1 := emmc_boot1.hex
EMMC_USER  := emmc_user0.hex

all: $(SPI_ALL)

config:
	@./configure.sh

$(SPI_ALL):
	make config
	bash ./update_all.sh $(SPI_ALL)
	@echo ""
	@echo "* Gen NOR Hex : $(SPI_HEX)"
	@./tools/gen_hex.sh $(BIN)/$(SPI_ALL) $(BIN)/$(SPI_HEX)

###############################
# Pack for isp boot testing
isp: all
	@echo "Build $(ISP_IMG)"
	@dd if=bin/xboot.img    of=$(BIN)/$(ISP_IMG)
	@dd if=bin/u-boot.img   of=$(BIN)/$(ISP_IMG) conv=notrunc bs=1k seek=64
###############################

# To create isp disk hex for zebu
# 1. make isp --> gen ispbooot.BIN
# 2. Read disk/sample_note.txt to learn how to create disk/sd64m.bin
# 4. make disk_hex --> gen disk/sd_image.hex from disk/sd64m.bin
# 5. upload the hex to zebu : hex_files/MMC/sd_image.hex
DISK_IN=disk/sd64m.bin
DISK_OUT=disk/sd_image.hex
sd_hex:
	@hexdump -v -e '1/1 "%02x\n"' $(DISK_IN) > $(DISK_OUT)
	@ls -l $(DISK_OUT)

###############################
# Pack for emmc boot testing
emmc_hex: all
	@echo ""
	@echo "* Gen eMMC boot1 hex: $(EMMC_BOOT1)"
	@hexdump -v -e '1/1 "%02x\n"' $(BIN)/xboot.img > $(BIN)/$(EMMC_BOOT1)
	@ls -l $(BIN)/$(EMMC_BOOT1)
	@echo ""
	@echo "* Gen eMMC user hex: $(EMMC_USER)"
	@dd if=emmc_gpt/lba0.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 >/dev/null 2>&1
	@dd if=emmc_gpt/lba1.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=1 >/dev/null 2>&1
	@dd if=emmc_gpt/lba2.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=2 >/dev/null 2>&1
	@dd if=emmc_gpt/lba3.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=3 >/dev/null 2>&1
	@dd if=emmc_gpt/lba4.bin of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=4 >/dev/null 2>&1
	@dd if=$(BIN)/u-boot.img of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x22) >/dev/null 2>&1
	@dd if=$(BIN)/dtb.img of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x1422) >/dev/null 2>&1
	@dd if=$(BIN)/uImage of=$(BIN)/emmc_user0.bin conv=notrunc bs=512 seek=$(shell printf %u 0x1822) >/dev/null 2>&1
	@hexdump -v -e '1/1 "%02x\n"' $(BIN)/emmc_user0.bin > $(BIN)/$(EMMC_USER)
	@ls -l $(BIN)/$(EMMC_USER)
###############################

TO_RM   := $(SPI_ALL) $(SPI_HEX) $(ISP_IMG) zmem.hex \
	   bootRom.bin xboot.img u-boot.img ecos.img uImage vmlinux vmlinux.bin \
	   dtb dtb.img \
	   emmc_user0.bin $(EMMC_BOOT1) $(EMMC_USER)
clean:
	rm -f $(CFG)
	cd $(BIN) && rm -f $(TO_RM) $(ISP_IMG) $(SPI_HEX) zmem.hex
	-rm -f disk/sd_image.hex

distclean:
	cd $(BIN) && rm -f *.img *.bin
