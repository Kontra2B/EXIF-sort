CC = g++
CFLAGS = -o0
SRC = sort.cpp context.cpp helper.cpp
INC = context.hpp helper.hpp exif.hpp
OBJ = $(SRC:%.cpp=%.o)

.PHONY: all debug release clean

all: exif.sort

*.o: *.cpp
	$(CC) $(CFLAGS) -c

exif.sort: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

debug: CFLAGS += -ggdb3
debug: exif.sort

clean: 
	rm *.o exif.sort
