CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2 -Iinclude

BUILD_DIR = build
SRC_DIR = src
TEST_DIR = tests

TARGET = game
SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/game_map.cpp \
       $(SRC_DIR)/map_generator.cpp \
       $(SRC_DIR)/terminal.cpp \
       $(SRC_DIR)/renderer.cpp \
       $(SRC_DIR)/engine.cpp \
       $(SRC_DIR)/camera.cpp \
       $(SRC_DIR)/vision.cpp \
       $(SRC_DIR)/action.cpp \
       $(SRC_DIR)/building_prefab.cpp \
       $(SRC_DIR)/settings.cpp \
       $(SRC_DIR)/settings_menu.cpp \
       $(SRC_DIR)/main_menu.cpp

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Modular Test Targets
TEST_TILE_TARGET = test_tile
TEST_TILE_OBJS = $(BUILD_DIR)/tests/test_tile.o

TEST_VEGETATION_TARGET = test_vegetation
TEST_VEGETATION_OBJS = $(BUILD_DIR)/tests/test_vegetation.o $(BUILD_DIR)/game_map.o $(BUILD_DIR)/vision.o

TEST_WEATHER_TARGET = test_weather
TEST_WEATHER_OBJS = $(BUILD_DIR)/tests/test_weather.o $(BUILD_DIR)/game_map.o $(BUILD_DIR)/map_generator.o $(BUILD_DIR)/vision.o $(BUILD_DIR)/building_prefab.o

TEST_MAP_TARGET = test_map
TEST_MAP_OBJS = $(BUILD_DIR)/tests/test_map.o $(BUILD_DIR)/game_map.o $(BUILD_DIR)/map_generator.o $(BUILD_DIR)/vision.o $(BUILD_DIR)/building_prefab.o $(BUILD_DIR)/action.o $(BUILD_DIR)/camera.o

TEST_VISION_TARGET = test_vision
TEST_VISION_OBJS = $(BUILD_DIR)/tests/test_vision.o $(BUILD_DIR)/vision.o $(BUILD_DIR)/game_map.o

TEST_BUILDING_TARGET = test_building_prefab
TEST_BUILDING_OBJS = $(BUILD_DIR)/tests/test_building_prefab.o $(BUILD_DIR)/building_prefab.o $(BUILD_DIR)/game_map.o $(BUILD_DIR)/vision.o $(BUILD_DIR)/action.o $(BUILD_DIR)/camera.o

TEST_SETTINGS_TARGET = test_settings
TEST_SETTINGS_OBJS = $(BUILD_DIR)/tests/test_settings.o $(BUILD_DIR)/settings.o

ARCH_TEST_TARGETS = $(TEST_TILE_TARGET) $(TEST_VEGETATION_TARGET) $(TEST_WEATHER_TARGET) $(TEST_MAP_TARGET) $(TEST_VISION_TARGET) $(TEST_BUILDING_TARGET)
ALL_TEST_TARGETS = $(ARCH_TEST_TARGETS) $(TEST_SETTINGS_TARGET)

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

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)/tests
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(ALL_TEST_TARGETS) test_arch

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run test test_arch
