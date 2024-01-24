CC=gcc
CFLAGS=-I.
OBJ = main
VPATH = src

%.asm: %.c
	$(CC) -S -fverbose-asm -g -O2 $< -o build/$*.asm

%.o: %.c
	$(CC) -c $< -o build/$@

build: $(OBJ).o $(OBJ).c $(OBJ).asm
	$(CC) -o build/pgrm_$(OBJ) build/$(OBJ).o

clean:
	rm -f build/pgrm_main build/$(OBJ).o build/$(OBJ).asm