CXX=g++
RM=rm -f
INC_DIR = ../
CXXFLAGS=-O2 -c --std=c++14 -Wall -I$(INC_DIR)

nrom_mapper.o: nrom_mapper.cpp nrom_mapper.h
	$(CXX) $(CXXFLAGS) nrom_mapper.cpp

clean:
	$(RM) *.o
