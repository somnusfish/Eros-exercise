CC = clang
CFLAGS = -O4 `llvm-config --cflags --ldflags`
LIBS = `llvm-config --libs core` -lncurses

.PHONY: all
all: eros

eros: eros.c
	$(CC) $(CFLAGS) -o $@ $? $(LIBS)

.PHONY: clean
clean:
	rm -fr *.o eros

