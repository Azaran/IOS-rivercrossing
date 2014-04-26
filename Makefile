CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic


rivercrossing: 
	     $(CC) -o rivercrossing hellomake.o hellofunc.o $(CFLAGS)
