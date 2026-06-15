TARGET  = emulator

CXX     = g++
CXXFLAGS = -std=c++23 -Wall

SDL2_CFLAGS = $(shell pkg-config --cflags sdl2)
SDL2_LIBS   = $(shell pkg-config --libs sdl2)

SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build

SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/cpu.cpp \
       $(SRC_DIR)/memory.cpp \
       $(SRC_DIR)/cartridge.cpp \
       $(SRC_DIR)/timer.cpp \
       $(SRC_DIR)/ppu.cpp

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(SDL2_LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) $(SDL2_CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: clean
