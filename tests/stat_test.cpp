// Standalone check for STAT interrupt-line behaviour (edge detection + sources)
// and for the read-only bits of STAT/LY. Exit 0 on success (asserts abort).
#undef NDEBUG
#include "memory.h"
#include "ppu.h"
#include <cassert>
#include <cstdio>

static const int FRAME_CYCLES = 154 * 456;

// Ticks one full frame in 4-cycle steps and counts STAT (IF bit 1) requests,
// clearing IF after each tick so every request is observed individually.
static int statRequestsPerFrame(uint8_t stat_enable, uint8_t lyc) {
    Memory memory;
    PPU ppu(memory);
    memory.ppu = &ppu;
    memory.write(0xFF45, lyc);
    memory.write(0xFF41, stat_enable);
    memory.write_direct(0xFF0F, 0);

    int count = 0;
    for (int c = 0; c < FRAME_CYCLES; c += 4) {
        ppu.tick(4);
        if (memory.read_direct(0xFF0F) & 0x02) count++;
        memory.write_direct(0xFF0F, 0);
    }
    return count;
}

int main() {
    // LYC source: exactly one request when LY hits LYC (not one per tick).
    assert(statRequestsPerFrame(0x40, 10) == 1);
    // Mode 0 source: once per visible line.
    assert(statRequestsPerFrame(0x08, 200) == 144);
    // Mode 1 source: once per frame (entering VBlank).
    assert(statRequestsPerFrame(0x10, 200) == 1);
    // No source enabled: no request, even with LY==LYC.
    assert(statRequestsPerFrame(0x00, 10) == 0);

    // Register write masks: STAT bits 0-2 read-only, LY read-only.
    Memory memory;
    memory.write_direct(0xFF41, 0x85);
    memory.write(0xFF41, 0xFF);
    assert(memory.read_direct(0xFF41) == (0x85 | 0x78));
    memory.write_direct(0xFF44, 42);
    memory.write(0xFF44, 7);
    assert(memory.read_direct(0xFF44) == 42);

    std::printf("stat_test OK\n");
    return 0;
}
