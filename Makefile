CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
OBJ=main.c


main: 
	     $(CC) -o rivercrossing $(OBJ) $(CFLAGS)
