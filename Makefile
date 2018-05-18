all:
	g++ -std=c++11 -o server server.cpp -lpthread
	g++ -std=c++11 -o client client.cpp -lpthread
clean:
	rm server client
