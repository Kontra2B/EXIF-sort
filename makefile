CC = g++
CFLAGS = -o2
BIN = exif.sort
SRC = sort.cpp context.cpp helper.cpp
INC = context.hpp helper.hpp exif.hpp
OBJ = $(SRC:%.cpp=%.o)

.PHONY: all debug clean

all: exif.sort

%.o: %.cpp $(INC)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

debug: CFLAGS = -ggdb3 -o0
debug: exif.sort

clean: 
	rm *.o exif.sort

install:
	install -m 700 $(BIN) ~/.local/bin/
