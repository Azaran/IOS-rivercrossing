
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -lrt -pthread
OBJ=main.c 


main: 
	     $(CC) -o rivercrossing $(OBJ) $(CFLAGS)

#test:
#	     $(CC) -o test test_fork.c $(CFLAGS)
