CFLAGS=-Wall -Werror
OBJ=../build/main.o ../build/compiler.o ../build/tokenizer.o

all: ../build/compiler

../build/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

../build/compiler: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)