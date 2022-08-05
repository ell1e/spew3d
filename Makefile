
ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
HEADERS=$(sort $(filter-out ./spew3d.h ./spew3d_prefixed.h,$(wildcard ./*.h)))
SOURCES=$(sort $(wildcard ./implementation/*.c))
TESTPROG=$(sort $(patsubst %.c, %$(BINEXT), $(wildcard ./examples/example_*.c)))

all:
	cat spew3d_prefixed.h vendor/miniz/include/miniz/miniz.h vendor/stb/stb_image.h $(HEADERS) $(SOURCES) > spew3d.h

test:
	cd examples && $(MAKE) clean && $(MAKE)
	cd examples && valgrind ./example_sprite.bin

update-vendor:
	@if [ -e "vendor/miniz/miniz.h" ]; then cd vendor/miniz && git reset --hard; fi
	git submodule init && git submodule update
	# Make sure miniz is available:
	cd vendor/miniz/ && mkdir -p ./include/miniz/ && rm -f ./include/miniz/miniz.c && rm -f ./include/miniz/miniz.h && rm -rf ./amalgamation/ && rm -rf ./_build && CC=gcc CXX=g++ CFLAGS= bash ./amalgamate.sh && cp ./amalgamation/miniz.c ./include/miniz/miniz.c && cp ./amalgamation/miniz.h ./include/miniz/miniz.h

clean:
	rm -f $(TESTPROG)
	rm -f ./spew3d.h
