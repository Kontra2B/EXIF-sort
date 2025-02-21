CC = g++
CFLAGS = -o2
SRC = sort.cpp context.cpp helper.cpp
INC = context.hpp helper.hpp exif.hpp
OBJ = $(SRC:%.cpp=%.o)

.PHONY: all debug clean

all: exif.sort

%.o: %.cpp $(INC)
	$(CC) $(CFLAGS) -c $< -o $@

exif.sort: $(OBJ)
	$(CC) $^ -o $@

debug: CFLAGS = -ggdb3 -o0
debug: exif.sort

clean: 
	rm *.o exif.sort
