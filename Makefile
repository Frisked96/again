CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2

TARGET = game
SRCS = main.cpp game_map.cpp map_generator.cpp terminal.cpp renderer.cpp engine.cpp camera.cpp vision.cpp action.cpp building_prefab.cpp settings.cpp settings_menu.cpp main_menu.cpp
OBJS = $(SRCS:.cpp=.o)

TEST_TARGET = test_arch
TEST_SRCS = tests/test_world_architecture.cpp game_map.cpp map_generator.cpp vision.cpp building_prefab.cpp action.cpp camera.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

TEST_SETTINGS_TARGET = test_settings
TEST_SETTINGS_SRCS = tests/test_settings.cpp settings.cpp
TEST_SETTINGS_OBJS = $(TEST_SETTINGS_SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST_TARGET) $(TEST_SETTINGS_TARGET)
	./$(TEST_TARGET)
	./$(TEST_SETTINGS_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_SETTINGS_TARGET): $(TEST_SETTINGS_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TEST_SETTINGS_OBJS) $(TARGET) $(TEST_TARGET) $(TEST_SETTINGS_TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run test
