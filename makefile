demo: demo.o
	gcc -std=gnu99 -pthread -o demo demo.o

demo: demo.c
	gcc -std=gnu99 -phtread -c demo.c

clean:
	rm demo demo.o



