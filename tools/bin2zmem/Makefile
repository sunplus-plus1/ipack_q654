BIN=bin2zmem

$(BIN): $(BIN).c
	gcc -o $@ $@.c
	strip $@

clean:
	@rm -f *.o $(BIN)
