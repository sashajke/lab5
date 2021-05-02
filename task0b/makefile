all : looper

looper : looper.c
	gcc -g -m32 -c -o looper.o looper.c
	gcc -g -m32 looper.o -o looper
	
.PHONY : clean
clean :
	rm looper
