.PHONY: clean distclean all config

BIN        := bin
SPI_ALL    := spi_all.bin
CFG        := pack.conf

all: $(SPI_ALL)

config:
	@./configure.sh

$(SPI_ALL):
	make config
	bash ./update_all.sh $(SPI_ALL)

TO_RM      := bootRom.bin xboot.img u-boot.img ecos.img uImage
clean:
	rm -f $(CFG)
	cd $(BIN) && rm -f $(SPI_ALL) $(TO_RM)

distclean:
	cd $(BIN) && rm -f *.img *.bin
