CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2

TARGET = game
SRCS = main.cpp game_map.cpp map_generator.cpp terminal.cpp renderer.cpp engine.cpp camera.cpp vision.cpp action.cpp building_prefab.cpp settings.cpp settings_menu.cpp main_menu.cpp
OBJS = $(SRCS:.cpp=.o)

# Modular Test Targets
TEST_TILE_TARGET = test_tile
TEST_TILE_OBJS = tests/test_tile.o

TEST_VEGETATION_TARGET = test_vegetation
TEST_VEGETATION_OBJS = tests/test_vegetation.o game_map.o vision.o

TEST_WEATHER_TARGET = test_weather
TEST_WEATHER_OBJS = tests/test_weather.o game_map.o map_generator.o vision.o building_prefab.o

TEST_MAP_TARGET = test_map
TEST_MAP_OBJS = tests/test_map.o game_map.o map_generator.o vision.o building_prefab.o action.o camera.o

TEST_VISION_TARGET = test_vision
TEST_VISION_OBJS = tests/test_vision.o vision.o game_map.o

TEST_BUILDING_TARGET = test_building_prefab
TEST_BUILDING_OBJS = tests/test_building_prefab.o building_prefab.o game_map.o vision.o action.o camera.o

TEST_SETTINGS_TARGET = test_settings
TEST_SETTINGS_OBJS = tests/test_settings.o settings.o

ARCH_TEST_TARGETS = $(TEST_TILE_TARGET) $(TEST_VEGETATION_TARGET) $(TEST_WEATHER_TARGET) $(TEST_MAP_TARGET) $(TEST_VISION_TARGET) $(TEST_BUILDING_TARGET)
ALL_TEST_TARGETS = $(ARCH_TEST_TARGETS) $(TEST_SETTINGS_TARGET)
ALL_TEST_OBJS = tests/test_tile.o tests/test_vegetation.o tests/test_weather.o tests/test_map.o tests/test_vision.o tests/test_building_prefab.o tests/test_settings.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(ALL_TEST_TARGETS)
	./$(TEST_TILE_TARGET)
	./$(TEST_VEGETATION_TARGET)
	./$(TEST_WEATHER_TARGET)
	./$(TEST_MAP_TARGET)
	./$(TEST_VISION_TARGET)
	./$(TEST_BUILDING_TARGET)
	./$(TEST_SETTINGS_TARGET)

test_arch: $(ARCH_TEST_TARGETS)
	./$(TEST_TILE_TARGET)
	./$(TEST_VEGETATION_TARGET)
	./$(TEST_WEATHER_TARGET)
	./$(TEST_MAP_TARGET)
	./$(TEST_VISION_TARGET)
	./$(TEST_BUILDING_TARGET)

$(TEST_TILE_TARGET): $(TEST_TILE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_VEGETATION_TARGET): $(TEST_VEGETATION_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_WEATHER_TARGET): $(TEST_WEATHER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_MAP_TARGET): $(TEST_MAP_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_VISION_TARGET): $(TEST_VISION_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_BUILDING_TARGET): $(TEST_BUILDING_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_SETTINGS_TARGET): $(TEST_SETTINGS_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(ALL_TEST_OBJS) $(TARGET) $(ALL_TEST_TARGETS) test_arch

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run test test_arch
