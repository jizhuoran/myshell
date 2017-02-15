

myshell: myshell.c util execute parser sig viewtree
	gcc myshell.c util.o execute.o parser.o sig.o viewtree.o -o myshell -std=gnu99

execute: execute.c
	gcc -c execute.c -std=gnu99

parser: parser.c
	gcc -c parser.c -std=gnu99

util: util.c
	gcc -c util.c -std=gnu99

sig: sig.c
	gcc -c sig.c -std=gnu99

viewtree: viewtree.c
	gcc -c viewtree.c -std=gnu99

clear:
	rm *.o

.PHONY:
	clear