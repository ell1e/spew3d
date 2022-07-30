

ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif

build:
	cd ../ && $(MAKE)
	gcc -g -o example_cube$(BINEXT) ./example_cube.c -I../ -lm -lSDL2
