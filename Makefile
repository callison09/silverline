silverline : main.o
	gcc -Wall -pedantic -o silverline main.o /usr/lib/libncurses.so /usr/lib/libcurl.so /usr/lib/libz.so
main.o : main.c
	gcc -std=c99 -Wall -pedantic -c main.c
clean :
	rm *.o silverline