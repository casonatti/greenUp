all: sleep_server clean

sleep_server: main.cpp participantsTable.cpp communication.cpp election.cpp globals.cpp
	g++ -std=c++17 -o sleep_server participantsTable.cpp communication.cpp election.cpp  main.cpp -lpthread

clean:
	rm -rf *.o