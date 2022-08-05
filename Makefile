
ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
HEADERS=$(filter-out ./spew3d.h ./spew3d_prefixed.h,$(wildcard ./*.h))
SOURCES=$(wildcard ./implementation/*.c)
TESTPROG=$(patsubst %.c, %.$(BINEXT), $(wildcard ./examples/example_*.c))

all:
	cat spew3d_prefixed.h vendor/miniz/miniz.h vendor/stb/stb_image.h $(HEADERS) $(SOURCES) > spew3d.h
test:
	cd examples && $(MAKE)
	cd examples && valgrind ./example_cube.bin
update-vendor:
	@if [ -e "vendor/miniz/miniz.h" ]; cd vendor/miniz && git reset --hard; fi
	git submodule init && git submodule update

