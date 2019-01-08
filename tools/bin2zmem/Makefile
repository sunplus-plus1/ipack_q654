BIN=bin2zmem

$(BIN): $(BIN).c
	gcc -m32 -o $@ $@.c
	strip $@

clean:
	@rm -f *.o $(BIN)
