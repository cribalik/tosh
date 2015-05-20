
CC=gcc
FLAGS=-pedantic -Wall -ansi -O4
TARGETS=src/*.c

all:
	$(CC) $(FLAGS) $(TARGETS) -o tosh
sigdet:
	$(CC) $(FLAGS) $(TARGETS) -DSIGDET=1 -o tosh
nosigdet:
	$(CC) $(FLAGS) $(TARGETS) -DSIGDET=0 -o tosh
