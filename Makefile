# Portions of this Makefile have come from (wonderful) folks online.
# I'll include a [x] citation label for blocks that have mostly been copy-pasted
# [1] - https://www.throwtheswitch.org/build/make
#		Mostly used this source as-is, except I renamed some defines and use Test_<filename>.c instead of Test<filename>.c naming

# Check what our host is and set appropriate commands [1]
ifeq ($(OS),Windows_NT)
  ifeq ($(shell uname -s),) # not in a bash-like shell
	CLEANUP = del /F /Q
	MKDIR = mkdir
  else # in a bash-like shell, like msys
	CLEANUP = rm -f
	MKDIR = mkdir -p
  endif
	TARGET_EXTENSION=exe
else
	CLEANUP = rm -f
	MKDIR = mkdir -p
	TARGET_EXTENSION=out
endif
# End [1]


# Name some paths [1]
PATH_Unity = unity/src/
PATH_Main = main/
PATH_Lib = lib/
PATH_Test = test/
PATH_Build = build/
PATH_BDepends = build/depends/
PATH_BObjs = build/objs/
PATH_BResults = build/results/

BUILD_PATHS = $(PATH_Build) $(PATH_BDepends) $(PATH_BObjs) $(PATH_BResults)
# End [1]


# Where to look for source [1]
SRC_Test = $(wildcard $(PATH_Test)*.c)
# End [1]

SRC_Lib = $(wildcard $(PATH_Lib)*.c)


# Toolchain [1]
COMPILE=gcc -c
LINK=gcc
DEPEND=gcc -MM -MG -MF
CFLAGS=-I. -I$(PATH_Unity) -I$(PATH_Lib) -DTEST
# End [1]


############################
### Begin rules sections ###
############################

# List out the results [1]
RESULTS = $(patsubst $(PATH_Test)Test_%.c,$(PATH_BResults)Test_%.txt,$(SRC_Test) )
# End [1]


# Summarize results [1]
PASSED = `grep -s PASS $(PATH_BResults)*.txt`
FAIL = `grep -s FAIL $(PATH_BResults)*.txt`
IGNORE = `grep -s IGNORE $(PATH_BResults)*.txt`

test: $(BUILD_PATHS) $(RESULTS)
	@echo "-----------------------\nIGNORES:\n-----------------------"
	@echo "$(IGNORE)"
	@echo "-----------------------\nFAILURES:\n-----------------------"
	@echo "$(FAIL)"
	@echo "-----------------------\nPASSED:\n-----------------------"
	@echo "$(PASSED)"
	@echo "\nDONE"
# End [1]


# Create results [1]
$(PATH_BResults)%.txt: $(PATH_Build)%.$(TARGET_EXTENSION)
	@echo "\nhi"
	-./$< > $@ 2>&1
# End [1]


# Create executables [1]
$(PATH_Build)Test_%.$(TARGET_EXTENSION): $(PATH_BObjs)Test_%.o $(patsubst $(PATH_Lib)%.c,$(PATH_BObjs)%.o,$(SRC_Lib) ) $(PATH_BObjs)unity.o #$(PATH_BDepends)Test%.d
	$(LINK) -o $@ $^
# End [1]


# Create object files [1]
$(PATH_BObjs)%.o:: $(PATH_Test)%.c
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATH_BObjs)%.o:: $(PATH_Lib)%.c
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATH_BObjs)%.o:: $(PATH_Unity)%.c $(PATH_Unity)%.h
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATH_BObjs)%.o:: $(PATH_Main)%.c
	$(COMPILE) $(CFLAGS) $< -o $@
# End [1]


# Dependencies [1]
$(PATH_BDepends)%.d:: $(PATH_Test)%.c
	$(DEPEND) $@ $<
# End [1]


# Create directories [1]
$(PATH_Build):
	$(MKDIR) $(PATH_Build)

$(PATH_BDepends):
	$(MKDIR) $(PATH_BDepends)

$(PATH_BObjs):
	$(MKDIR) $(PATH_BObjs)

$(PATH_BResults):
	$(MKDIR) $(PATH_BResults)
# End [1]




$(PATH_Build)pgrm.$(TARGET_EXTENSION): $(PATH_BObjs)main.o $(patsubst $(PATH_Lib)%.c,$(PATH_BObjs)%.o,$(SRC_Lib) )
	$(LINK) -o $@ $^


.PHONY: build
build: $(PATH_Build)pgrm.$(TARGET_EXTENSION)


.PHONY: run
run: build
	./$(PATH_Build)pgrm.$(TARGET_EXTENSION)



# Clean [1]
clean:
	$(CLEANUP) $(PATH_BObjs)*.o
	$(CLEANUP) $(PATH_Build)*.$(TARGET_EXTENSION)
	$(CLEANUP) $(PATH_BResults)*.txt
# End [1]


# Keep important [1]
.PRECIOUS: $(PATH_Build)Test_%.$(TARGET_EXTENSION)
.PRECIOUS: $(PATH_BDepends)%.d
.PRECIOUS: $(PATH_BObjs)%.o
.PRECIOUS: $(PATH_BResults)%.txt
# End [1]


.PHONY: print
print:
	@echo "Build paths: $(BUILD_PATHS)"
	@echo "Results: $(RESULTS)"
	@echo "PTest: $(PATH_Test)"
	@echo "BR: $(PATH_BResults)"
	@echo "STest: $(SRC_Test)"



# %.asm: %.c
# 	$(CC) -S -fverbose-asm -g -O2 $< -o build/$*.asm

