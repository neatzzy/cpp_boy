TARGET  = emulator

CXX     = g++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -MMD -MP

SDL2_CFLAGS = $(shell pkg-config --cflags sdl2)
SDL2_LIBS   = $(shell pkg-config --libs sdl2)

SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build

CORE_SRCS = $(SRC_DIR)/cpu.cpp \
            $(SRC_DIR)/memory.cpp \
            $(SRC_DIR)/cartridge.cpp \
            $(SRC_DIR)/timer.cpp \
            $(SRC_DIR)/ppu.cpp

SRCS = $(SRC_DIR)/main.cpp $(CORE_SRCS)

OBJS      = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
CORE_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CORE_SRCS))

TEST_RUNNER = $(BUILD_DIR)/run_rom
STAT_TEST   = $(BUILD_DIR)/stat_test
PPU_TEST    = $(BUILD_DIR)/ppu_test
TEST_ROMS   = 01 02 03 04 05 06 07 08 09 10 11 instr_timing halt_bug

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(SDL2_LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) $(SDL2_CFLAGS) -c $< -o $@

$(TEST_RUNNER): tests/run_rom.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/run_rom.cpp $(CORE_OBJS) -o $@

$(STAT_TEST): tests/stat_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/stat_test.cpp $(CORE_OBJS) -o $@

$(PPU_TEST): tests/ppu_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/ppu_test.cpp $(CORE_OBJS) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TEST_RUNNER) $(STAT_TEST) $(PPU_TEST)
	@sh tests/fetch_roms.sh
	@fail=0; \
	if $(STAT_TEST); then echo "PASS stat_test"; else echo "FAIL stat_test"; fail=1; fi; \
	if $(PPU_TEST); then echo "PASS ppu_test"; else echo "FAIL ppu_test"; fail=1; fi; \
	for rom in $(TEST_ROMS); do \
		if $(TEST_RUNNER) tests/roms/$$rom.gb > $(BUILD_DIR)/$$rom.log; then \
			echo "PASS $$rom"; \
		else \
			echo "FAIL $$rom (see $(BUILD_DIR)/$$rom.log)"; fail=1; \
		fi; \
	done; exit $$fail

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(OBJS:.o=.d)

.PHONY: clean test
