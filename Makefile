.PHONY: clean distclean all config

BIN        := bin
SPI_ALL    := spi_all.bin
CFG        := pack.conf
ISP_IMG    := ispbooot.BIN

all: $(SPI_ALL)
	make isp

config:
	@./configure.sh

$(SPI_ALL):
	make config
	bash ./update_all.sh $(SPI_ALL)

###############################
# Pack for isp boot testing
isp:
	@echo "Build $(ISP_IMG)"
	@dd if=bin/xboot.img    of=$(BIN)/$(ISP_IMG)
	@dd if=bin/u-boot.img   of=$(BIN)/$(ISP_IMG) conv=notrunc bs=1k seek=64
###############################

TO_RM      := bootRom.bin xboot.img ecos.img uImage
clean:
	rm -f $(CFG)
	cd $(BIN) && rm -f $(SPI_ALL) $(TO_RM) $(ISP_IMG)

distclean:
	cd $(BIN) && rm -f *.img *.bin
