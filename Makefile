
ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
UNITTEST_SOURCES_NOSDL=$(sort $(wildcard ./src/test_*_nosdl.c))
UNITTEST_SOURCES=$(sort $(wildcard ./src/test_*.c))
UNITTEST_SOURCES_WITHSDL=$(sort $(filter-out $(UNITTEST_SOURCES), $(UNITTEST_SOURCES_NOSDL)))
UNITTEST_BASENAMES=$(sort $(patsubst %.c, %, $(UNITTEST_SOURCES)))
UNITTEST_BASENAMES_NOSDL=$(sort $(patsubst %.c, %, $(UNITTEST_SOURCES_NOSDL)))
UNITTEST_BASENAMES_WITHSDL=$(sort $(patsubst %.c, %, $(UNITTEST_SOURCES_WITHSDL)))
HEADERS=./include/spew3d_math2d.h ./include/spew3d_math3d.h $(sort $(filter-out ./include/spew3d.h ./src/testmain.h ./src/spew3d_prefix_drlibsstbvorbis.h ./src/spew3d_postfix_drlibsstbvorbis.h ./src/spew3d_prefix_all.h ./src/spew3d_prefix_miniz_c.h ./src/spew3d_postfix_miniz_c.h,$(wildcard ./include/*.h) $(wildcard ./src/*.h)))
SOURCES=$(sort $(filter-out $(UNITTEST_SOURCES), $(wildcard ./src/*.c)))
TESTPROG=$(sort $(patsubst %.c, %$(BINEXT), $(wildcard ./examples/example_*.c ./src/test_*.c)))

all: amalgamate build-tests

amalgamate: update-vendor-if-needed
	echo "#ifdef SPEW3D_IMPLEMENTATION" > .spew3d_ifdef
	echo "" >> .spew3d_ifdef
	echo "#endif  // SPEW3D_IMPLEMENTATION" > .spew3d_ifndef
	echo "" >> .spew3d_ifndef
	cat src/spew3d_prefix_all.h .spew3d_ifdef vendor/siphash.c .spew3d_ifndef vendor/miniz/include/miniz/miniz.h src/spew3d_prefix_miniz_c.h vendor/miniz/include/miniz/miniz.c src/spew3d_postfix_miniz_c.h src/spew3d_prefix_drlibsstbvorbis.h vendor/dr_libs/dr_flac.h vendor/dr_libs/dr_mp3.h vendor/dr_libs/dr_wav.h src/spew3d_postfix_drlibsstbvorbis.h vendor/stb/stb_image.h $(HEADERS) $(SOURCES) > include/spew3d.h
	rm -f .spew3d_ifdef
	rm -f .spew3d_ifndef

reset-deps:
	git submodule foreach --recursive git reset --hard && git submodule foreach --recursive git clean -xfd && git submodule update --init

build-tests:
	cd examples && $(MAKE) clean && $(MAKE) CC="$(CC)"

test: amalgamate build-tests unittests
	cd examples && ./example_sprite.bin

update-vendor-if-needed:
	@if [ ! -e "vendor/miniz/include/miniz/miniz.h" ]; then $(MAKE) update-vendor; fi

update-vendor:
	@if [ ! -e "vendor/miniz/miniz.h" ]; then cd vendor/miniz && git submodule update --init; fi
	# Make sure miniz is available:
	cd vendor/miniz/ && mkdir -p ./include/miniz/ && rm -f ./include/miniz/miniz.c && rm -f ./include/miniz/miniz.h && rm -rf ./amalgamation/ && rm -rf ./_build && CC=gcc CXX=g++ CFLAGS= bash ./amalgamate.sh && cp ./amalgamation/miniz.c ./include/miniz/miniz.c && cp ./amalgamation/miniz.h ./include/miniz/miniz.h
	cd vendor/miniz && sed -i 's/\#include \"miniz\.h\"//g' ./include/miniz/miniz.c

unittests:
	echo "TESTS: $(UNITTEST_SOURCES) | $(UNITTEST_BASENAMES)"
	for x in $(UNITTEST_BASENAMES_WITHSDL); do $(CC) -g -O0 $(CFLAGS) -Iinclude/ $(CXXFLAGS) -pthread -o ./$$x$(BINEXT) ./$$x.c -lSDL2 -lcheck -lrt -lm $(LDFLAGS) || { exit 1; }; done
	for x in $(UNITTEST_BASENAMES_NOSDL); do $(CC) -g -O0 $(CFLAGS) -Iinclude/ $(CXXFLAGS) -pthread -o ./$$x$(BINEXT) ./$$x.c -lcheck -lrt -lm $(LDFLAGS) || { exit 1; }; done
	for x in $(UNITTEST_BASENAMES); do echo ">>> TEST RUN: $$x"; CK_FORK=no valgrind --track-origins=yes --leak-check=full ./$$x$(BINEXT) || { exit 1; }; done

clean:
	rm -f $(TESTPROG)
	rm -f ./include/spew3d.h
