
CC=gcc

all: animator

#prelib: mythread.o

#lib: mythread.o mymutex.o scheduler.o libmythread.a pruebalib

#%.o: %.c %.h
	#$(CC) -c $^

#mythread: scheduler.o mymutex.o mythread.c
	#$(CC) -o $@ $^ -lrt

scheduler.o: scheduler.c scheduler.h
	$(CC) -c $^

mymutex.o: mymutex.c mymutex.h
	$(CC) -c $^

mythread.o: mythread.c mythread.h
	$(CC) -c $^ -lrt

#libmythread.a: mythread.o mymutex.o scheduler.o
#	ar rcs $@ $^

Client.o: Client.c Client.h
	$(CC) -c $^

Scene.o: Scene.c Scene.h
	$(CC) -c $^

Figure.o: Figure.c Figure.h
	$(CC) -c $^

Parser.o: Parser.c Parser.h
	$(CC) -c $^

Server.o: Server.c Server.h
	$(CC) -c $^

#pruebalib: pruebalib.c
	#$(CC) -o $@ $^ -L. -lmythread -lrt

clean:
	rm *.o *.gch mythread

animator: Server.c mymutex.o scheduler.o Figure.o Parser.o Client.o Scene.o mythread.o
	$(CC) -o $@ $^ -lrt