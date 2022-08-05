
ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
HEADERS=$(filter-out ./spew3d.h,$(wildcard ./*.h))
SOURCES=$(wildcard ./*.c)
TESTPROG=$(patsubst %.c, %.$(BINEXT), $(wildcard ./examples/example_*.c))

all:
	cat $(HEADERS) $(SOURCES) > spew3d.h
test:
	cd examples && $(MAKE)
	cd examples && valgrind ./example_cube.bin
