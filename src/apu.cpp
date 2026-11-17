#include "apu.h"

namespace {
constexpr int CPU_HZ = 4194304;
constexpr int SAMPLE_HZ = 44100;
constexpr int FRAME_SEQ_PERIOD = CPU_HZ / 512; // 8192 T-cycles

// Bits that read back as 1 for 0xFF10-0xFF2F (write-only / unused).
constexpr uint8_t READ_MASK[0x20] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF,  // NR10-NR14
    0xFF, 0x3F, 0x00, 0xFF, 0xBF,  // 0xFF15, NR21-NR24
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF,  // NR30-NR34
    0xFF, 0xFF, 0x00, 0x00, 0xBF,  // 0xFF1F, NR41-NR44
    0x00, 0x00, 0x70,              // NR50-NR52
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF27-0xFF2F
};

constexpr uint8_t DUTY[4] = {0x01, 0x81, 0x87, 0x7E}; // bit i = output at step i
} // namespace

int APU::freq(int n) {
    uint16_t base = 0xFF10 + 5 * n;
    return reg(base + 3) | ((reg(base + 4) & 7) << 8);
}

void APU::setFreq(int n, int f) {
    uint16_t base = 0xFF10 + 5 * n;
    reg(base + 3) = f & 0xFF;
    reg(base + 4) = (reg(base + 4) & 0xF8) | ((f >> 8) & 7);
}

bool APU::dacOn(int n) {
    if (n == 2) return reg(0xFF1A) & 0x80;
    return reg(0xFF12 + 5 * n) & 0xF8;
}

// DMG: while ch3 plays, wave RAM accesses hit the byte it is reading, but only
// on the cycle it reads it; otherwise reads give 0xFF and writes are dropped.
uint8_t* APU::waveByte(uint16_t addr) {
    if (!ch[2].enabled) return &reg(addr);
    return waveJustRead ? &reg(0xFF30 + ch[2].pos / 2) : nullptr;
}

uint8_t APU::read(uint16_t addr) {
    if (addr >= 0xFF30) { uint8_t* b = waveByte(addr); return b ? *b : 0xFF; }
    if (addr == 0xFF26) {
        uint8_t status = 0x70 | (power ? 0x80 : 0);
        for (int n = 0; n < 4; n++) if (ch[n].enabled) status |= 1 << n;
        return status;
    }
    return reg(addr) | READ_MASK[addr - 0xFF10];
}

void APU::write(uint16_t addr, uint8_t value) {
    if (addr >= 0xFF30) { if (uint8_t* b = waveByte(addr)) *b = value; return; }

    if (addr == 0xFF26) {
        bool on = value & 0x80;
        if (power && !on) {
            for (uint16_t a = 0xFF10; a <= 0xFF25; a++) reg(a) = 0;
            for (Channel& c : ch) { int len = c.length; c = Channel{}; c.length = len; } // DMG keeps length
            sweepEnabled = false;
        } else if (!power && on) {
            frameStep = 0;
            frameTimer = 0;
        }
        power = on;
        return;
    }

    if (addr > 0xFF25) return; // unmapped 0xFF27-0xFF2F
    int n = (addr - 0xFF10) / 5;
    int r = (addr - 0xFF10) % 5;

    // DMG: length counters stay writable while powered off.
    if (!power) {
        if (r == 1 && n < 4) ch[n].length = lengthMax(n) - (n == 2 ? value : (value & 0x3F));
        return;
    }

    reg(addr) = value;
    if (n >= 4) return; // NR50/NR51
    switch (r) {
        case 0:
            if (n == 0 && sweepNegated && !(value & 0x08)) ch[0].enabled = false;
            if (n == 2 && !dacOn(2)) ch[2].enabled = false;
            break;
        case 1:
            ch[n].length = lengthMax(n) - (n == 2 ? value : (value & 0x3F));
            break;
        case 2:
            if (n != 2 && !dacOn(n)) ch[n].enabled = false;
            break;
        case 4:
            writeNRx4(n, value);
            break;
    }
}

void APU::writeNRx4(int n, uint8_t value) {
    Channel& c = ch[n];
    bool wasEnabled = c.lengthEnabled;
    c.lengthEnabled = value & 0x40;
    bool nextSkipsLength = frameStep & 1;

    // Enabling length in the half of the period that won't clock it clocks it once now.
    if (nextSkipsLength && !wasEnabled && c.lengthEnabled && c.length > 0) {
        if (--c.length == 0 && !(value & 0x80)) c.enabled = false;
    }
    if (value & 0x80) trigger(n);
}

void APU::trigger(int n) {
    Channel& c = ch[n];
    bool wasOn = c.enabled;
    c.enabled = true;
    if (c.length == 0) {
        c.length = lengthMax(n);
        if (c.lengthEnabled && (frameStep & 1)) c.length--;
    }

    int f = freq(n);
    if (n == 3) {
        uint8_t nr43 = reg(0xFF22);
        int r = nr43 & 7;
        c.timer = (r ? r * 16 : 8) << (nr43 >> 4);
        lfsr = 0x7FFF;
    } else if (n == 2) {
        // DMG: retrigger on a sample-read cycle corrupts the first bytes of wave RAM.
        if (wasOn && c.timer == 0) {
            int next = ((c.pos + 1) >> 1) & 0xF;
            if (next < 4) reg(0xFF30) = reg(0xFF30 + next);
            else for (int i = 0; i < 4; i++) reg(0xFF30 + i) = reg(0xFF30 + (next & ~3) + i);
        }
        c.timer = 2047 - f + 3; // first read 6 T-cycles later than a normal period
        c.pos = 0;
        waveJustRead = false;
    } else {
        c.timer = (2048 - f) * 4;
    }

    if (n != 2) {
        uint8_t nrx2 = reg(0xFF12 + 5 * n);
        c.volume = nrx2 >> 4;
        c.envTimer = nrx2 & 7;
    }

    if (n == 0) {
        uint8_t nr10 = reg(0xFF10);
        int period = (nr10 >> 4) & 7, shift = nr10 & 7;
        shadowFreq = f;
        sweepTimer = period ? period : 8;
        sweepEnabled = period || shift;
        sweepNegated = false;
        if (shift) sweepCalc();
    }

    if (!dacOn(n)) c.enabled = false;
}

int APU::sweepCalc() {
    uint8_t nr10 = reg(0xFF10);
    int delta = shadowFreq >> (nr10 & 7);
    int f;
    if (nr10 & 0x08) { f = shadowFreq - delta; sweepNegated = true; }
    else f = shadowFreq + delta;
    if (f > 2047) ch[0].enabled = false;
    return f;
}

void APU::clockLength() {
    for (Channel& c : ch)
        if (c.lengthEnabled && c.length > 0 && --c.length == 0) c.enabled = false;
}

void APU::clockSweep() {
    if (--sweepTimer > 0) return;
    uint8_t nr10 = reg(0xFF10);
    int period = (nr10 >> 4) & 7, shift = nr10 & 7;
    sweepTimer = period ? period : 8;
    if (!sweepEnabled || !period) return;
    int f = sweepCalc();
    if (f <= 2047 && shift) {
        shadowFreq = f;
        setFreq(0, f);
        sweepCalc(); // second overflow check with the new frequency
    }
}

void APU::clockEnvelope() {
    for (int n : {0, 1, 3}) {
        uint8_t nrx2 = reg(0xFF12 + 5 * n);
        int period = nrx2 & 7;
        Channel& c = ch[n];
        if (!period || --c.envTimer > 0) continue;
        c.envTimer = period;
        if ((nrx2 & 0x08) && c.volume < 15) c.volume++;
        else if (!(nrx2 & 0x08) && c.volume > 0) c.volume--;
    }
}

void APU::stepChannels() {
    if ((waveHalf = !waveHalf)) stepWave();
    for (int n : {0, 1, 3}) {
        Channel& c = ch[n];
        if (--c.timer > 0) continue;
        if (n == 3) {
            uint8_t nr43 = reg(0xFF22);
            int r = nr43 & 7;
            c.timer = (r ? r * 16 : 8) << (nr43 >> 4);
            int x = (lfsr ^ (lfsr >> 1)) & 1;
            lfsr = (lfsr >> 1) | (x << 14);
            if (nr43 & 0x08) lfsr = (lfsr & ~0x40) | (x << 6);
        } else {
            c.timer = (2048 - freq(n)) * 4;
            c.pos = (c.pos + 1) & 7;
        }
    }
}

void APU::stepWave() {
    Channel& c = ch[2];
    waveJustRead = false;
    if (!c.enabled) return;
    if (c.timer > 0) { c.timer--; return; }
    c.timer = 2047 - freq(2);
    c.pos = (c.pos + 1) & 31;
    waveJustRead = true;
}

int APU::output(int n) {
    const Channel& c = ch[n];
    if (!c.enabled) return 0;
    if (n == 3) return (~lfsr & 1) ? c.volume : 0;
    if (n == 2) {
        int code = (reg(0xFF1C) >> 5) & 3;
        if (!code) return 0;
        uint8_t b = reg(0xFF30 + c.pos / 2);
        int sample = (c.pos & 1) ? (b & 0x0F) : (b >> 4);
        return sample >> (code - 1);
    }
    int duty = reg(0xFF11 + 5 * n) >> 6;
    return (DUTY[duty] >> c.pos) & 1 ? c.volume : 0;
}

void APU::mix() {
    uint8_t nr50 = reg(0xFF24), nr51 = reg(0xFF25);
    float left = 0, right = 0;
    for (int n = 0; n < 4; n++) {
        float v = output(n) / 15.0f;
        if (nr51 & (0x10 << n)) left += v;
        if (nr51 & (0x01 << n)) right += v;
    }
    samples.push_back(left / 4 * (((nr50 >> 4) & 7) + 1) / 8);
    samples.push_back(right / 4 * ((nr50 & 7) + 1) / 8);
}

void APU::tick(int cycles) {
    for (int i = 0; i < cycles; i++) {
        if (power) {
            stepChannels();
            if (++frameTimer == FRAME_SEQ_PERIOD) {
                frameTimer = 0;
                if (!(frameStep & 1)) clockLength();
                if (frameStep == 2 || frameStep == 6) clockSweep();
                if (frameStep == 7) clockEnvelope();
                frameStep = (frameStep + 1) & 7;
            }
        }
        sampleTimer += SAMPLE_HZ;
        if (sampleTimer >= CPU_HZ) {
            sampleTimer -= CPU_HZ;
            mix();
        }
    }
}
