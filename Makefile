CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2

TARGET = game
SRCS = main.cpp game_map.cpp terminal.cpp renderer.cpp engine.cpp camera.cpp fov.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
