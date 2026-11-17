#pragma once
#include <cstdint>
#include <vector>

// DMG audio: 4 channels, frame sequencer, stereo mixer.
// Owns 0xFF10-0xFF3F (registers + wave RAM). Output: interleaved L/R floats at 44100 Hz.
class APU {
public:
    std::vector<float> samples; // L,R,L,R...; the frontend drains it

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
    void tick(int cycles);

private:
    struct Channel {
        bool enabled = false;
        bool lengthEnabled = false;
        int length = 0;
        int volume = 0;
        int envTimer = 0;
        int timer = 0;
        int pos = 0;            // duty step / wave sample index
    };
    Channel ch[4];

    uint8_t regs[0x30] = {};    // 0xFF10-0xFF3F
    bool power = false;
    int frameTimer = 0;
    int frameStep = 0;          // next frame-sequencer step to run (0-7)
    int sampleTimer = 0;

    // ch1 sweep
    bool sweepEnabled = false;
    bool sweepNegated = false;  // a negate calculation happened since trigger
    int sweepTimer = 0;
    int shadowFreq = 0;

    uint16_t lfsr = 0x7FFF;

    uint8_t& reg(uint16_t addr) { return regs[addr - 0xFF10]; }
    int freq(int n);
    void setFreq(int n, int f);
    bool dacOn(int n);
    int lengthMax(int n) { return n == 2 ? 256 : 64; }
    void trigger(int n);
    void writeNRx4(int n, uint8_t value);
    int sweepCalc();
    void clockLength();
    void clockSweep();
    void clockEnvelope();
    void stepChannels();
    int output(int n);
    void mix();
};
