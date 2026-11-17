CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++20 -Wall -Wextra -g -Iinclude -Ivendor/glad/include -MMD -MP $(shell sdl2-config --cflags)
CFLAGS   := -Wall -g -Ivendor/glad/include
LDFLAGS  := $(shell sdl2-config --libs) -lGL -ldl
BUILD    := build
TARGET   := $(BUILD)/app

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:src/%.cpp=$(BUILD)/%.o)

# GLAD (C file)
OBJS += $(BUILD)/glad.o

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

# Compile GLAD (C, not C++)
$(BUILD)/glad.o: vendor/glad/src/glad.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

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
