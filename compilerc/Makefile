CFLAGS=-Wall -Wextra -Werror -Og -g
OBJ=../build/main.o ../build/compiler.o ../build/tokenizer.o ../build/map.o

all: ../build/compiler

../build/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

../build/compiler: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)