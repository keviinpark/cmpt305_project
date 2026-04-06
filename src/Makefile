CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3
TARGET = cpu_simulator

OBJS = main.o Pipeline.o Simulation.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

main.o: main.cpp Simulation.h
	$(CXX) $(CXXFLAGS) -c main.cpp

Pipeline.o: Pipeline.cpp Pipeline.h Instruction.h
	$(CXX) $(CXXFLAGS) -c Pipeline.cpp

Simulation.o: Simulation.cpp Simulation.h Pipeline.h Instruction.h
	$(CXX) $(CXXFLAGS) -c Simulation.cpp

clean:
	rm -f $(OBJS) $(TARGET)