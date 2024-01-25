CC=g++
CFLAGS=-I.
CPPFLAGS += -I$(CPPUTEST_HOME)/include -I../src
CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
CFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorMallocMacros.h
LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt -mmacosx-version-min=14.0 

OBJ = AllTests test

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

pgrm.bin: AllTests.o test.o 
	$(CC) $(LD_LIBRARIES) -o $@ $^ ../build/castle.o

.PHONY: build clean

build: pgrm.bin

clean:
	rm -f $(OBJ).bin $(OBJ).o
