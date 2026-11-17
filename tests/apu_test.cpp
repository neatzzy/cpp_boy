// APU register and channel behaviour. Exit 0 on success (asserts abort).
#undef NDEBUG
#include "apu.h"
#include "memory.h"
#include <cassert>

static const int STEP = 8192; // one frame-sequencer step (512 Hz)

static bool on(Memory& m, int ch) { return m.read(0xFF26) & (1 << ch); }

static void powerCycle(Memory& m) {
    m.write(0xFF26, 0x00);
    m.write(0xFF26, 0x80);
}

int main() {
    Memory memory;
    APU apu;
    memory.apu = &apu;

    // Read masks: unused / write-only bits read back as 1.
    const uint8_t masks[0x16] = {
        0x80, 0x3F, 0x00, 0xFF, 0xBF,  // NR10-NR14
        0xFF, 0x3F, 0x00, 0xFF, 0xBF,  // 0xFF15, NR21-NR24
        0x7F, 0xFF, 0x9F, 0xFF, 0xBF,  // NR30-NR34
        0xFF, 0xFF, 0x00, 0x00, 0xBF,  // 0xFF1F, NR41-NR44
        0x00, 0x00,                    // NR50-NR51
    };
    memory.write(0xFF26, 0x80);
    for (int i = 0; i < 0x16; i++) {
        memory.write(0xFF10 + i, 0x00);
        assert(memory.read(0xFF10 + i) == masks[i]);
    }
    for (uint16_t a = 0xFF27; a <= 0xFF2F; a++) assert(memory.read(a) == 0xFF);
    assert(memory.read(0xFF26) == 0xF0); // powered, no channel active

    // Power off clears registers and ignores writes; wave RAM survives.
    memory.write(0xFF24, 0x77);
    memory.write(0xFF30, 0xAB);
    memory.write(0xFF26, 0x00);
    assert(memory.read(0xFF26) == 0x70);
    assert(memory.read(0xFF24) == 0x00);
    memory.write(0xFF24, 0x77);
    assert(memory.read(0xFF24) == 0x00);
    assert(memory.read(0xFF30) == 0xAB);
    memory.write(0xFF26, 0x80);
    memory.write(0xFF24, 0x77);
    assert(memory.read(0xFF24) == 0x77);

    // Trigger with DAC on sets the status bit; DAC off clears it.
    memory.write(0xFF17, 0xF0);
    memory.write(0xFF19, 0x80);
    assert(on(memory, 1));
    memory.write(0xFF17, 0x00);
    assert(!on(memory, 1));
    memory.write(0xFF19, 0x80); // trigger with DAC off: stays off
    assert(!on(memory, 1));

    // Length counter: length 2 expires on the second length clock (step 2).
    powerCycle(memory);
    memory.write(0xFF16, 0x3E);
    memory.write(0xFF17, 0xF0);
    memory.write(0xFF19, 0xC0);
    apu.tick(2 * STEP);
    assert(on(memory, 1));
    apu.tick(STEP);
    assert(!on(memory, 1));

    // Sweep overflow on trigger disables ch1 immediately.
    powerCycle(memory);
    memory.write(0xFF10, 0x01);
    memory.write(0xFF12, 0xF0);
    memory.write(0xFF13, 0xFF);
    memory.write(0xFF14, 0x87);
    assert(!on(memory, 0));

    // Sweep overflow on the first sweep clock (step 2).
    powerCycle(memory);
    memory.write(0xFF10, 0x11);
    memory.write(0xFF12, 0xF0);
    memory.write(0xFF13, 0x00);
    memory.write(0xFF14, 0x85);
    assert(on(memory, 0));
    apu.tick(2 * STEP);
    assert(on(memory, 0));
    apu.tick(STEP);
    assert(!on(memory, 0));

    // Mixer: ch2 panned to both sides produces sound; NR51=0 is silence.
    auto play = [&](uint8_t nr51) {
        powerCycle(memory);
        memory.write(0xFF24, 0x77);
        memory.write(0xFF25, nr51);
        memory.write(0xFF16, 0x80);
        memory.write(0xFF17, 0xF0);
        memory.write(0xFF18, 0x00);
        memory.write(0xFF19, 0x87);
        apu.samples.clear();
        apu.tick(70224);
        bool loud = false;
        for (float s : apu.samples) loud |= s != 0.0f;
        return loud;
    };
    assert(play(0x22));
    assert(apu.samples.size() >= 2 * 730 && apu.samples.size() <= 2 * 740); // ~738 stereo frames per video frame
    assert(!play(0x00));

    // Wave RAM while ch3 plays (DMG): CPU sees only the byte being read this cycle.
    auto startWave = [&](uint16_t f, int cycles) {
        powerCycle(memory);
        for (int i = 0; i < 16; i++) memory.write(0xFF30 + i, i * 0x11);
        memory.write(0xFF1A, 0x80);
        memory.write(0xFF1D, f & 0xFF);
        memory.write(0xFF1E, 0x80 | (f >> 8));
        apu.tick(cycles);
    };
    auto waveRam = [&](int i) { memory.write(0xFF1A, 0x00); return memory.read(0xFF30 + i); };

    startWave(0x700, 0);
    assert(memory.read(0xFF30) == 0xFF);  // no sample read this cycle
    memory.write(0xFF3F, 0x5A);           // ignored
    assert(waveRam(15) == 0xFF);

    // Max frequency reads a sample every 2 T-cycles; first read 6 T-cycles late, at index 1.
    startWave(0x7FF, 6);
    assert(memory.read(0xFF30) == 0xFF);
    startWave(0x7FF, 8);
    assert(memory.read(0xFF3F) == 0x00);  // index 1 -> byte 0
    startWave(0x7FF, 12);
    assert(memory.read(0xFF35) == 0x11);  // index 3 -> byte 1
    memory.write(0xFF30, 0x5A);           // lands in the byte being read
    assert(waveRam(1) == 0x5A);

    // Retrigger on a read cycle corrupts the start of wave RAM.
    startWave(0x7FF, 10);                 // index 2: next byte is 1
    memory.write(0xFF1E, 0x87);
    assert(waveRam(0) == 0x11 && waveRam(1) == 0x11 && waveRam(4) == 0x44);
    startWave(0x7FF, 24);                 // index 9: next byte is 5 -> copy block 4-7
    memory.write(0xFF1E, 0x87);
    for (int i = 0; i < 8; i++) assert(waveRam(i) == ((i & 3) + 4) * 0x11);
    startWave(0x700, 24);                 // not on a read cycle: untouched
    memory.write(0xFF1E, 0x87);
    for (int i = 0; i < 16; i++) assert(waveRam(i) == i * 0x11);
    return 0;
}
