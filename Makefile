CFLAGS= -std=c99 -Wall -Wextra -Werror -pedantic -g
proj1: proj1.o
	gcc $(CFLAGS) -o proj1 proj1.o
proj1.o: proj1.c
	gcc $(CFLAGS) -c -o proj1.o proj1.c
clean:
	-rm -f proj1 proj1.o

