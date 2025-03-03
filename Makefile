http-server: http-server.o 
	gcc http-server.o -o http-server
http-server.o: http-server.c
	gcc -c -g -Wall http-server.c


.PHONY:clean
clean:
	rm -f *.o http-server
