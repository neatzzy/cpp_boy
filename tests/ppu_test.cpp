// PPU sprite (OBJ) and Window rendering checks. Exit 0 on success (asserts abort).
#undef NDEBUG
#include "memory.h"
#include "ppu.h"
#include <cassert>
#include <cstdio>

static const uint32_t W = 0xFFFFFFFF, L = 0xFFAAAAAA, D = 0xFF555555, B = 0xFF000000;

struct Rig {
    Memory memory;
    PPU ppu;
    Rig() : ppu(memory) {
        memory.ppu = &ppu;
        memory.write_direct(0xFF47, 0xE4); // BGP: 0->W 1->L 2->D 3->B
        memory.write_direct(0xFF48, 0xE4); // OBP0 identity
        memory.write_direct(0xFF49, 0x1B); // OBP1 inverted
        memory.write_direct(0xFF40, 0x93); // LCD on, BG on, 0x8000 data, OBJ off
    }
    void tileRow(int tile, int row, uint8_t lo, uint8_t hi) {
        ppu.writeVRAM(0x8000 + tile * 16 + row * 2, lo);
        ppu.writeVRAM(0x8000 + tile * 16 + row * 2 + 1, hi);
    }
    // Whole tile: every row has the same bits.
    void tile(int t, uint8_t lo, uint8_t hi) { for (int r = 0; r < 8; r++) tileRow(t, r, lo, hi); }
    void sprite(int i, int y, int x, int t, int attr) {
        ppu.writeOAM(0xFE00 + i * 4, y + 16);
        ppu.writeOAM(0xFE00 + i * 4 + 1, x + 8);
        ppu.writeOAM(0xFE00 + i * 4 + 2, t);
        ppu.writeOAM(0xFE00 + i * 4 + 3, attr);
    }
    void objOn(bool tall = false) { memory.write_direct(0xFF40, 0x93 | 0x02 | (tall ? 0x04 : 0)); }
    uint32_t px(int x, int y) { return ppu.framebuffer[y * 160 + x]; }
    void line(int ly) { memory.write_direct(0xFF44, ly); ppu.renderScanline(); }
};

int main() {
    // Plain sprite: tile 1 = colour 3 on leftmost pixel only, 8x8 at (10,20).
    { Rig r; r.tile(1, 0x80, 0x80); r.sprite(0, 20, 10, 1, 0); r.objOn(); r.line(20);
      assert(r.px(10, 20) == B); assert(r.px(11, 20) == W); assert(r.px(9, 20) == W);
      r.line(28); assert(r.px(10, 28) == W); }   // below the sprite

    // X flip: leftmost pixel moves to rightmost.
    { Rig r; r.tile(1, 0x80, 0x80); r.sprite(0, 20, 10, 1, 0x20); r.objOn(); r.line(20);
      assert(r.px(17, 20) == B); assert(r.px(10, 20) == W); }

    // Y flip: only tile row 0 is set; flipped it appears on the last sprite line.
    { Rig r; r.tileRow(1, 0, 0xFF, 0xFF); r.sprite(0, 20, 10, 1, 0x40); r.objOn();
      r.line(20); assert(r.px(10, 20) == W);
      r.line(27); assert(r.px(10, 27) == B); }

    // 8x16: second half uses tile|1, low bit of index ignored.
    { Rig r; r.tile(2, 0xFF, 0x00); r.tile(3, 0x00, 0xFF); r.sprite(0, 20, 10, 3, 0); r.objOn(true);
      r.line(20); assert(r.px(10, 20) == L);   // tile 2 (index & 0xFE)
      r.line(28); assert(r.px(10, 28) == D);   // tile 3 (index | 1)
      r.line(36); assert(r.px(10, 36) == W); }

    // 8x16 with Y flip swaps the halves.
    { Rig r; r.tile(2, 0xFF, 0x00); r.tile(3, 0x00, 0xFF); r.sprite(0, 20, 10, 2, 0x40); r.objOn(true);
      r.line(20); assert(r.px(10, 20) == D); r.line(28); assert(r.px(10, 28) == L); }

    // Transparency: colour 0 pixels keep the BG.
    { Rig r; r.tile(0, 0xFF, 0xFF); r.tile(1, 0x80, 0x00); r.sprite(0, 20, 10, 1, 0); r.objOn(); r.line(20);
      assert(r.px(10, 20) == L);  // sprite colour 1
      assert(r.px(11, 20) == B);  // transparent -> BG colour 3 (tile 0)
    }

    // OBP1 selected by attr bit 4.
    { Rig r; r.tile(1, 0x80, 0x80); r.sprite(0, 20, 10, 1, 0x10); r.objOn(); r.line(20);
      assert(r.px(10, 20) == W); }  // colour 3 -> shade 0 in OBP1

    // BG priority: sprite hides behind BG colours 1-3, shows over colour 0.
    { Rig r; r.tile(1, 0xFF, 0x00);          // sprite colour 1 everywhere
      r.tile(0, 0x00, 0x00);                 // BG tile 0 = colour 0
      r.tile(5, 0xFF, 0xFF);                 // BG colour 3
      r.ppu.writeVRAM(0x9800 + 2 * 32 + 2, 5); // map row 2 (y 16..23), col 2 (x 16..23)
      r.sprite(0, 20, 12, 1, 0x80); r.objOn(); r.line(20);
      assert(r.px(12, 20) == L);             // over BG colour 0
      assert(r.px(16, 20) == B); }           // behind BG colour 3

    // 10 sprites per line: the 11th is not drawn.
    { Rig r; r.tile(1, 0xFF, 0xFF);
      for (int i = 0; i < 11; i++) r.sprite(i, 20, i * 9, 1, 0);
      r.objOn(); r.line(20);
      assert(r.px(9 * 9, 20) == B); assert(r.px(10 * 9, 20) == W); }

    // Overlap: lower X wins even with higher OAM index; ties go to lower index.
    { Rig r; r.tile(1, 0xFF, 0x00); r.tile(2, 0x00, 0xFF);
      r.sprite(0, 20, 14, 1, 0);  // OAM 0, X 14 (colour 1)
      r.sprite(1, 20, 10, 2, 0);  // OAM 1, X 10 (colour 2)
      r.objOn(); r.line(20);
      assert(r.px(14, 20) == D);  // x=14 covered by both: lower X (OAM 1) wins
      assert(r.px(18, 20) == L);  // only OAM 0
      Rig t; t.tile(1, 0xFF, 0x00); t.tile(2, 0x00, 0xFF);
      t.sprite(0, 20, 10, 1, 0); t.sprite(1, 20, 10, 2, 0); t.objOn(); t.line(20);
      assert(t.px(10, 20) == L); }

    // Window: WX=7 -> x=0; BG visible left of WX-7.
    { Rig r; r.tile(0, 0x00, 0x00); r.tile(1, 0xFF, 0xFF);
      r.ppu.writeVRAM(0x9C00, 1);
      r.memory.write_direct(0xFF40, 0x93 | 0x20 | 0x40);
      r.memory.write_direct(0xFF4A, 0); r.memory.write_direct(0xFF4B, 7); r.line(0);
      assert(r.px(0, 0) == B); assert(r.px(7, 0) == B); assert(r.px(8, 0) == W);
      Rig q; q.tile(0, 0x00, 0x00); q.tile(1, 0xFF, 0xFF);
      q.ppu.writeVRAM(0x9C00 + 2, 1);
      q.memory.write_direct(0xFF40, 0x93 | 0x20 | 0x40);
      q.memory.write_direct(0xFF4A, 0); q.memory.write_direct(0xFF4B, 7 + 40); q.line(0);
      assert(q.px(39, 0) == W); assert(q.px(40, 0) == W); assert(q.px(56, 0) == B);
      // Window above WY is not drawn.
      Rig s; s.tile(1, 0xFF, 0xFF); s.ppu.writeVRAM(0x9C00, 1);
      s.memory.write_direct(0xFF40, 0x93 | 0x20 | 0x40);
      s.memory.write_direct(0xFF4A, 5); s.memory.write_direct(0xFF4B, 7); s.line(4);
      assert(s.px(0, 4) == W); }

    // Window line counter only advances on lines where the window was drawn.
    { Rig r; r.tileRow(1, 0, 0xFF, 0x00); r.tileRow(1, 1, 0x00, 0xFF); r.tileRow(1, 2, 0xFF, 0xFF);
      r.ppu.writeVRAM(0x9C00, 1);
      r.memory.write_direct(0xFF4A, 0); r.memory.write_direct(0xFF4B, 7);
      r.memory.write_direct(0xFF40, 0x93 | 0x20 | 0x40); r.line(0);   // window line 0
      assert(r.px(0, 0) == L);
      r.memory.write_direct(0xFF40, 0x93 | 0x40); r.line(1);          // window off: counter holds
      r.memory.write_direct(0xFF40, 0x93 | 0x20 | 0x40); r.line(2);   // window line 1, not 2
      assert(r.px(0, 2) == D);
      r.line(3); assert(r.px(0, 3) == B);                              // window line 2
    }

    std::printf("ppu_test OK\n");
    return 0;
}
