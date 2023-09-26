
ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
UNITTEST_SOURCES_NOSDL=$(sort $(wildcard ./implementation/test_*_nosdl.c))
UNITTEST_SOURCES=$(sort $(wildcard ./implementation/test_*.c))
UNITTEST_SOURCES_WITHSDL=$(sort $(filter-out $(UNITTEST_SOURCES), $(UNITTEST_SOURCES_NOSDL)))
UNITTEST_BASENAMES=$(sort $(patsubst %.c, %, $(UNITTEST_SOURCES)))
UNITTEST_BASENAMES_NOSDL=$(sort $(patsubst %.c, %, $(UNITTEST_SOURCES_NOSDL)))
UNITTEST_BASENAMES_WITHSDL=$(sort $(patsubst %.c, %, $(UNITTEST_SOURCES_WITHSDL)))
HEADERS=$(sort $(filter-out ./include/spew3d.h ./implementation/testmain.h ./implementation/spew3d_prefix_drlibsstbvorbis.h ./implementation/spew3d_postfix_drlibsstbvorbis.h ./implementation/spew3d_prefix_all.h ./implementation/spew3d_prefix_miniz_c.h ./implementation/spew3d_postfix_miniz_c.h,$(wildcard ./include/*.h) $(wildcard ./implementation/*.h)))
SOURCES=$(sort $(filter-out $(UNITTEST_SOURCES), $(wildcard ./implementation/*.c)))
TESTPROG=$(sort $(patsubst %.c, %$(BINEXT), $(wildcard ./examples/example_*.c ./implementation/test_*.c)))

all: amalgamate build-tests

amalgamate: update-vendor-if-needed
	echo "#ifdef SPEW3D_IMPLEMENTATION" > .spew3d_ifdef
	echo "" >> .spew3d_ifdef
	echo "#endif  // SPEW3D_IMPLEMENTATION" > .spew3d_ifndef
	echo "" >> .spew3d_ifndef
	cat implementation/spew3d_prefix_all.h vendor/r128/r128.h include/spew3d_int128.h .spew3d_ifdef vendor/siphash.c .spew3d_ifndef vendor/miniz/include/miniz/miniz.h implementation/spew3d_prefix_miniz_c.h vendor/miniz/include/miniz/miniz.c implementation/spew3d_postfix_miniz_c.h implementation/spew3d_prefix_drlibsstbvorbis.h vendor/stb/stb_vorbis.c vendor/dr_libs/dr_flac.h vendor/dr_libs/dr_mp3.h vendor/dr_libs/dr_wav.h implementation/spew3d_postfix_drlibsstbvorbis.h vendor/stb/stb_image.h $(HEADERS) $(SOURCES) > include/spew3d.h
	rm -f .spew3d_ifdef
	rm -f .spew3d_ifndef

build-tests:
	cd examples && $(MAKE) clean && $(MAKE) CC="$(CC)"

test: amalgamate build-tests unittests
	cd examples && ./example_sprite.bin

update-vendor-if-needed:
	@if [ ! -e "vendor/miniz/include/miniz/miniz.h" ]; then $(MAKE) update-vendor; fi

update-vendor:
	@if [ -e "vendor/miniz/miniz.h" ]; then cd vendor/miniz && git reset --hard; fi
	git submodule init && git submodule update
	# Make sure miniz is available:
	cd vendor/miniz/ && mkdir -p ./include/miniz/ && rm -f ./include/miniz/miniz.c && rm -f ./include/miniz/miniz.h && rm -rf ./amalgamation/ && rm -rf ./_build && CC=gcc CXX=g++ CFLAGS= bash ./amalgamate.sh && cp ./amalgamation/miniz.c ./include/miniz/miniz.c && cp ./amalgamation/miniz.h ./include/miniz/miniz.h
	cd vendor/miniz && sed -i 's/\#include \"miniz\.h\"//g' ./include/miniz/miniz.c

unittests:
	echo "TESTS: $(UNITTEST_SOURCES) | $(UNITTEST_BASENAMES)"
	for x in $(UNITTEST_BASENAMES_WITHSDL); do $(CC) -g -Og $(CFLAGS) -Iinclude/ $(CXXFLAGS) -pthread -o ./$$x$(BINEXT) ./$$x.c -lSDL2 -lcheck -lrt -lm $(LDFLAGS) || { exit 1; }; done
	for x in $(UNITTEST_BASENAMES_NOSDL); do $(CC) -g -Og $(CFLAGS) -Iinclude/ $(CXXFLAGS) -pthread -o ./$$x$(BINEXT) ./$$x.c -lcheck -lrt -lm $(LDFLAGS) || { exit 1; }; done
	for x in $(UNITTEST_BASENAMES); do echo ">>> TEST RUN: $$x"; CK_FORK=no valgrind --track-origins=yes --leak-check=full ./$$x$(BINEXT) || { exit 1; }; done

clean:
	rm -f $(TESTPROG)
	rm -f ./include/spew3d.h
