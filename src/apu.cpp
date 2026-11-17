#include "apu.h"

uint8_t APU::read(uint16_t) { return 0xFF; }
void APU::write(uint16_t, uint8_t) {}
void APU::tick(int) {}
