CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2

TARGET = game
SRCS = main.cpp game_map.cpp map_generator.cpp terminal.cpp renderer.cpp engine.cpp camera.cpp fov.cpp
OBJS = $(SRCS:.cpp=.o)

TEST_TARGET = test_arch
TEST_SRCS = tests/test_world_architecture.cpp game_map.cpp map_generator.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run test
