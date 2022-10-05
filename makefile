all: clean sleep_server

sleep_server:
	g++ -std=c++17 -o sleep_server participantsTable.cpp election.cpp communication.cpp main.cpp -lpthread

clean:
	rm -rf *.o