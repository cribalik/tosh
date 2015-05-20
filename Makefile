
CC=gcc
FLAGS=-pedantic -Wall -ansi -O4
TARGETS=src/*.c
OUTPUT=tosh

all:
	$(CC) $(FLAGS) $(TARGETS) -o $(OUTPUT)
sigdet:
	$(CC) $(FLAGS) $(TARGETS) -DSIGDET=1 -o $(OUTPUT)
nosigdet:
	$(CC) $(FLAGS) $(TARGETS) -DSIGDET=0 -o $(OUTPUT)
clean:
	rm $(OUTPUT)
