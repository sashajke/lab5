all : myShell LineParser linkAll

myShell : myShell.c
	gcc -g -m32 -c -o myShell.o myShell.c

LineParser : LineParser.c
	gcc -g -m32 -c -o LineParser.o LineParser.c

linkAll : myShell.o LineParser.o
	gcc -g -m32 myShell.o LineParser.o -o myShell
	rm myShell.o LineParser.o

	
.PHONY : clean
clean :
	rm myShell
