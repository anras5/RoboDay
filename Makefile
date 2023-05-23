# SOURCES=$(wildcard *.c)
# HEADERS=$(SOURCES:.c=.h)
SOURCES=$(wildcard *.cpp)
HEADERS=$(wildcard *.hpp)
# FLAGS=-DDEBUG -g
FLAGS=

all: main

main: $(SOURCES) #
	mpic++ $(SOURCES) $(HEADERS) $(FLAGS) -o main -pthread -Wall

clear: clean

clean:
	rm main

run: main
	mpirun -np 2 ./main
