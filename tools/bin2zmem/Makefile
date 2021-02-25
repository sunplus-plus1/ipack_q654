BIN=bin2zmem
BIN2=bin2zmem_q645
all:$(BIN) $(BIN2)

$(BIN): $(BIN).c
	gcc -o $@ $@.c
	strip $@

$(BIN2): $(BIN2).c
	gcc -o $@ $@.c
	strip $@

clean:
	@rm -f *.o $(BIN) $(BIN2)
