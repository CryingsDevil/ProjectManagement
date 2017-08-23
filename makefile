demo: demo.o
	gcc -std=gnu99 -pthread -o demo demo.o

demo.o: demo.c
	gcc -std=gnu99 -pthread -c demo.c

clean:
	rm demo demo.o



