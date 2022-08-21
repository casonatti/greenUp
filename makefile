all: sleep_server clean

sleep_server: main.o
	g++ -std=c++2a -o sleep_server main.o -lpthread

main.o: main.cpp
	g++ -std=c++2a -c main.cpp

clean:
	rm -rf *.o