BIN=pack_zebu_nand

$(BIN): $(BIN).c
	gcc -o $@ $@.c
	strip $@

clean:
	rm -f $(BIN)
