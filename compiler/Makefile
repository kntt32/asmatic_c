CC := gcc
CFLAGS := -Wall -Wextra -g

compiler.elf: main.o types.o vec.o str.o parser.o util.o register.o gen.o syntax.o
	$(CC) $(CFLAGS) $^ -o $@

main.o: main.c types.h parser.h gen.h
types.c: types.h
parser.o: parser.c parser.h util.h
vec.o: vec.c vec.h types.h
str.o: str.c str.h vec.h
util.o: util.c util.h types.h
register.o: register.c register.h types.h
gen.o: gen.c gen.h types.h parser.h register.h
syntax.o: syntax.c types.h parser.h syntax.h

types.h:
parser.h: types.h
str.h: types.h vec.h
vec.h: types.h
util.h: types.h
register.h: types.h parser.h
gen.h: register.h types.h vec.h str.h
syntax.h: types.h gen.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: compiler.elf
	./compiler.elf

debug: compiler.elf
	gdb compiler.elf

clean:
	rm -f *.elf *.o
