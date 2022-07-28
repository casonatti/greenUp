all: sleep_server clean

sleep_server: main.o
	g++ -o sleep_server main.o -lpthread

main.o: main.cpp
	g++ -c main.cpp

clean:
	rm -rf *.o