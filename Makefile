
ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
HEADERS=$(filter-out ./spew3d.h ./spew3d_prefixed.h,$(wildcard ./*.h))
SOURCES=$(wildcard ./*.c)
TESTPROG=$(patsubst %.c, %.$(BINEXT), $(wildcard ./examples/example_*.c))

all:
	cat spew3d_prefixed.h vendor/stb/stb_image.h $(HEADERS) $(SOURCES) > spew3d.h
test:
	cd examples && $(MAKE)
	cd examples && valgrind ./example_cube.bin
