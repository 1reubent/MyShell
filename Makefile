CC     = gcc
CFLAGS = -g -std=c11 -Wall -Wvla -Werror -fsanitize=address,undefined

all: mysh

mysh: mysh.o
	$(CC) $(CFLAGS) mysh.o -o mysh

mysh.o: mysh.c 
	$(CC) $(CFLAGS) -c mysh.c

clean:
	rm -rf mysh *.o *.a  *.dylib *.dSYM


	
