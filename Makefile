
ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
HEADERS=$(sort $(filter-out ./spew3d.h ./implementation/spew3d_prefix_all.h ./implementation/spew3d_prefix_miniz_c.h ./implementation/spew3d_postfix_miniz_c.h,$(wildcard ./*.h) $(wildcard ./implementation/*.h)))
SOURCES=$(sort $(wildcard ./implementation/*.c))
TESTPROG=$(sort $(patsubst %.c, %$(BINEXT), $(wildcard ./examples/example_*.c)))

all:
	cat implementation/spew3d_prefix_all.h vendor/miniz/include/miniz/miniz.h implementation/spew3d_prefix_miniz_c.h vendor/miniz/include/miniz/miniz.c implementation/spew3d_postfix_miniz_c.h vendor/stb/stb_image.h $(HEADERS) $(SOURCES) > spew3d.h

test:
	cd examples && $(MAKE) clean && $(MAKE)
	cd examples && valgrind ./example_sprite.bin

update-vendor:
	@if [ -e "vendor/miniz/miniz.h" ]; then cd vendor/miniz && git reset --hard; fi
	git submodule init && git submodule update
	# Make sure miniz is available:
	cd vendor/miniz/ && mkdir -p ./include/miniz/ && rm -f ./include/miniz/miniz.c && rm -f ./include/miniz/miniz.h && rm -rf ./amalgamation/ && rm -rf ./_build && CC=gcc CXX=g++ CFLAGS= bash ./amalgamate.sh && cp ./amalgamation/miniz.c ./include/miniz/miniz.c && cp ./amalgamation/miniz.h ./include/miniz/miniz.h
	cd vendor/miniz && sed -i 's/\#include \"miniz\.h\"//g' ./include/miniz/miniz.c

clean:
	rm -f $(TESTPROG)
	rm -f ./spew3d.h
