

ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
TESTPROG=$(sort $(patsubst %.c, %$(BINEXT), $(wildcard ./example_*.c)))

build: assemble $(TESTPROG)

assemble:
	cd ../ && $(MAKE)

example_%$(BINEXT): example_%.c
	gcc -g -Og -o $@ $(patsubst %$(BINEXT),%.c, $@) -I../ -lm -lSDL2

clean:
	rm -rf $(TESTPROG)
