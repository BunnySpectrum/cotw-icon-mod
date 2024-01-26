CC=gcc
CFLAGS=-I.
OBJ = main.o castle.o io.o win.o

VPATH = src

%.asm: %.c
	$(CC) -S -fverbose-asm -g -O2 $< -o build/$*.asm

build/%.o: %.c
	$(CC) -c $< -o $@

pgrm.bin: $(addprefix build/,$(OBJ))
	$(CC) -o build/$@ $^

clean:
	rm -f build/pgrm.bin build/*.o build/*.asm
