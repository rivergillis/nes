CXX=g++
RM=rm -f
SDL2CFLAGS=-I/opt/homebrew/include/SDL2 -D_THREAD_SAFE
INC_DIR = ./
CXXFLAGS=-O2 -c --std=c++14 -Wall $(SDL2CFLAGS) -I$(INC_DIR)

# Load dynamic libs here
LDFLAGS=-L/opt/homebrew/lib -lSDL2

nes: main.o image.o sdl_viewer.o sdl_timer.o cpu6502.o mappers/nrom_mapper.o memory_view.o
	$(CXX) $(LDFLAGS) -o nes main.o image.o sdl_viewer.o sdl_timer.o cpu6502.o mappers/nrom_mapper.o memory_view.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp

image.o: image.cpp image.h
	$(CXX) $(CXXFLAGS) image.cpp

# cpu_chip8.o: cpu_chip8.cpp cpu_chip8.h
# 	$(CXX) $(CXXFLAGS) cpu_chip8.cpp

sdl_viewer.o: sdl_viewer.cpp sdl_viewer.h
	$(CXX) $(CXXFLAGS) sdl_viewer.cpp

sdl_timer.o: sdl_timer.cpp sdl_timer.h
	$(CXX) $(CXXFLAGS) sdl_timer.cpp

cpu6502.o: cpu6502.cpp cpu6502.h
	$(CXX) $(CXXFLAGS) cpu6502.cpp

memory_view.o: memory_view.cpp memory_view.h
	$(CXX) $(CXXFLAGS) memory_view.cpp

SUBDIR = mappers
# .PHONY: mappers_dir
# mappers_dir:
# 	$(MAKE) -C $(SUBDIR)
clean:
	$(RM) nes *.o
	$(RM) mappers/*.o


subdirs := $(wildcard */)
sources := $(wildcard $(addsuffix *.cpp,$(subdirs)))
objects := $(patsubst %.cpp,%.o,$(sources))

$(objects) : %.o : %.cpp