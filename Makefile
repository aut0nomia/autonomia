CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -g -Iinclude -MMD -MP $(shell sdl2-config --cflags)
LDFLAGS  := $(shell sdl2-config --libs)
BUILD    := build
TARGET   := $(BUILD)/app

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:src/%.cpp=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

TEST_TARGET := $(BUILD)/test_runner
TEST_SRCS   := $(wildcard tests/*.cpp)
TEST_OBJS   := $(TEST_SRCS:tests/%.cpp=$(BUILD)/tests/%.o)
TEST_DEPS   := $(TEST_OBJS:.o=.d)

.PHONY: all clean compdb run test

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/%.o: src/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(TEST_OBJS) -o $@

$(BUILD)/tests/%.o: tests/%.cpp | $(BUILD)/tests
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/tests:
	mkdir -p $(BUILD)/tests

# Intercept a clean build to generate compile_commands.json
compdb: clean
	bear -- $(MAKE) all test

clean:
	rm -rf $(BUILD)

-include $(DEPS) $(TEST_DEPS)
