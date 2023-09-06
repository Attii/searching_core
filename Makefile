CC=g++
CFLAFGS=-c -Wall 

all: main

main: main.o document.o read_input_functions.o search_server.o string_processing.o request_queue.o remove_duplicates.o process_queries.o
	$(CC) main.o document.o read_input_functions.o search_server.o string_processing.o request_queue.o remove_duplicates.o process_queries.o -o main

main.o: main.cpp
	$(CC) $(CFLAFGS) main.cpp

document.o: document.cpp
	$(CC) $(CFLAFGS) document.cpp

read_input_functions.o: read_input_functions.cpp
	$(CC) $(CFLAFGS) read_input_functions.cpp

search_server.o: search_server.cpp 
	$(CC) $(CFLAFGS) search_server.cpp

string_processing.o: string_processing.cpp
	$(CC) $(CFLAFGS) string_processing.cpp

request_queue.o: request_queue.cpp
	$(CC) $(CFLAFGS) request_queue.cpp

remove_duplicates.o: remove_duplicates.cpp
	$(CC) $(CFLAFGS) remove_duplicates.cpp

process_queries.o: process_queries.cpp
	$(CC) $(CFLAFGS) process_queries.cpp
	
clean:
	rm -rf *.o main