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
            $(SRC_DIR)/input.cpp \
            $(SRC_DIR)/ppu.cpp \
            $(SRC_DIR)/apu.cpp

SRCS = $(SRC_DIR)/main.cpp $(CORE_SRCS)

OBJS      = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
CORE_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CORE_SRCS))

TEST_RUNNER = $(BUILD_DIR)/run_rom
STAT_TEST   = $(BUILD_DIR)/stat_test
JOYPAD_TEST = $(BUILD_DIR)/joypad_test
PPU_TEST    = $(BUILD_DIR)/ppu_test
CART_TEST   = $(BUILD_DIR)/cartridge_test
APU_TEST    = $(BUILD_DIR)/apu_test
BOOT_TEST   = $(BUILD_DIR)/boot_test
TEST_ROMS   = 01 02 03 04 05 06 07 08 09 10 11 instr_timing halt_bug cpu_instrs snd01 snd02 snd03 snd04 snd05 snd06 snd07 snd08 snd09 snd10 snd11 snd12

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(SDL2_LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) $(SDL2_CFLAGS) -c $< -o $@

$(TEST_RUNNER): tests/run_rom.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/run_rom.cpp $(CORE_OBJS) -o $@

$(STAT_TEST): tests/stat_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/stat_test.cpp $(CORE_OBJS) -o $@

$(JOYPAD_TEST): tests/joypad_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/joypad_test.cpp $(CORE_OBJS) -o $@

$(PPU_TEST): tests/ppu_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/ppu_test.cpp $(CORE_OBJS) -o $@

$(CART_TEST): tests/cartridge_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/cartridge_test.cpp $(CORE_OBJS) -o $@

$(APU_TEST): tests/apu_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/apu_test.cpp $(CORE_OBJS) -o $@

$(BOOT_TEST): tests/boot_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) tests/boot_test.cpp $(CORE_OBJS) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TEST_RUNNER) $(STAT_TEST) $(JOYPAD_TEST) $(PPU_TEST) $(CART_TEST) $(APU_TEST) $(BOOT_TEST)
	@sh tests/fetch_roms.sh
	@fail=0; \
	if $(STAT_TEST); then echo "PASS stat_test"; else echo "FAIL stat_test"; fail=1; fi; \
	if $(JOYPAD_TEST); then echo "PASS joypad_test"; else echo "FAIL joypad_test"; fail=1; fi; \
	if $(PPU_TEST); then echo "PASS ppu_test"; else echo "FAIL ppu_test"; fail=1; fi; \
	if $(CART_TEST); then echo "PASS cartridge_test"; else echo "FAIL cartridge_test"; fail=1; fi; \
	if $(APU_TEST); then echo "PASS apu_test"; else echo "FAIL apu_test"; fail=1; fi; \
	if $(BOOT_TEST); then echo "PASS boot_test"; else echo "FAIL boot_test"; fail=1; fi; \
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
