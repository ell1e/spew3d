

ifneq (,$(findstring mingw,$(CC)))
BINEXT:=.exe
else
BINEXT:=.bin
endif
TESTPROG=$(sort $(patsubst %.c, %$(BINEXT), $(wildcard ./example_*.c)))

build: amalgamate $(TESTPROG)

amalgamate:
	cd ../ && $(MAKE) amalgamate

example_%$(BINEXT): example_%.c
	$(CC) -pthread -g -Og -o $@ $(patsubst %$(BINEXT),%.c, $@) -I../include -lm -lSDL2

clean:
	rm -rf $(TESTPROG)
