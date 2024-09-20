CC = g++ 
SRCS = ./src/*.cpp
INC = ./src/
OPTS = -std=c++17 -Wall -Werror -lpthread -O3
POPTS = -std=c++17 -Wall -Werror -lpthread -O0 -g -fsanitize=address -D__PRINT__

EXEC = bin/prefix_scan

all: clean compile
dbg: clean dcompile

compile:
	$(CC) $(SRCS) $(OPTS) -I$(INC) -o $(EXEC)

dcompile:
	$(CC) $(SRCS) $(POPTS) -I$(INC) -o $(EXEC)

clean:
	rm -f $(EXEC)

