CC := gcc
CFLAGS := -Wall -Wextra -g

compiler.elf: main.o vec.o str.o parser.o util.o register.o gen.o
	$(CC) $(CFLAGS) $^ -o $@

main.o: main.c types.h parser.h
parser.o: parser.c parser.h
vec.o: vec.c vec.h types.h
str.o: str.c str.h
util.o: util.c util.h types.h
register.o: register.c register.h types.h
gen.o: gen.c gen.h

parser.h: types.h str.h
str.h: types.h vec.h
vec.h: types.h
util.h: types.h
register.h: types.h
gen.h: register.h types.h vec.h str.h


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: compiler.elf
	./compiler.elf

debug: compiler.elf
	gdb compiler.elf

clean:
	rm -f *.elf *.o
