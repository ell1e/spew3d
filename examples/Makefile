

ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif

build:
	cd ../ && $(MAKE)
	gcc -g -Og -o example_cube$(BINEXT) ./example_cube.c -I../ -lm -lSDL2
	gcc -g -Og -o example_sprite$(BINEXT) ./example_sprite.c -I../ -lm -lSDL2
