# cpp_boy

A Game Boy (DMG) emulator written in C++23, with SDL2 for video and audio.

Work in progress: the CPU, timer and interrupts pass the Blargg test ROMs, and
joypad, sprites, window and MBC1/MBC5 cartridges with battery saves are
implemented, plus 4-channel audio. See
[DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) for the roadmap.

## Status

| Component | State |
|---|---|
| CPU (all opcodes, CB prefix, interrupts, EI delay, HALT bug, illegal opcodes) | Done, passes Blargg `cpu_instrs` 01-11 |
| Timer (DIV/TIMA/TMA/TAC) | Done, passes `instr_timing` and `02-interrupts` |
| PPU background | Done |
| PPU STAT interrupts (LYC, modes 0/1/2) | Done |
| Joypad (`0xFF00`, joypad interrupt) | Done, covered by `joypad_test` |
| PPU sprites (OBJ, 8x8 and 8x16) | Done, covered by `ppu_test` |
| PPU window | Done, covered by `ppu_test` |
| Cartridge banking (MBC1, MBC5), external RAM, battery saves | Done, covered by `cartridge_test` and combined `cpu_instrs` |
| MBC2, MBC3 (RTC) | Not supported: runs as ROM-only with a warning |
| Audio (APU: 2 square, wave, noise, mixer) | Done, covered by `apu_test` and Blargg `dmg_sound` 01-08, 11 |
| Wave RAM access while channel 3 plays | Not emulated (`dmg_sound` 09, 10, 12 fail) |

Battery-backed cartridges save external RAM to `<rom name>.sav` next to the ROM
on exit and load it on start.

## Requirements

- A C++23 compiler (`g++` 13 or newer)
- [SDL2](https://www.libsdl.org/) and `pkg-config`

macOS: `brew install sdl2 pkg-config`
Debian/Ubuntu: `sudo apt install libsdl2-dev pkg-config g++`

## Build and run

```sh
make
./emulator path/to/game.gb
```

Controls:

| Game Boy | Key |
|---|---|
| D-pad | Arrow keys |
| A | `Z` |
| B | `X` |
| Start | `Enter` |
| Select | `Right Shift` or `Backspace` |
| Quit | `Esc` (or close the window) |

ROMs are not included; `*.gb` files are gitignored.

## Tests

```sh
make test
```

Builds and runs, in order:

- `stat_test`: STAT interrupt line (edge detection, sources) and read-only STAT/LY bits
- `joypad_test`: `0xFF00` register behavior
- `ppu_test`: sprite (OBJ) and window rendering
- `cartridge_test`: MBC1/MBC5 banking, external RAM, `.sav` round-trip
- `apu_test`: register masks, power, length, sweep, mixer output
- Blargg ROMs: `cpu_instrs` 01-11 (individual and combined), `instr_timing`, `halt_bug`, `dmg_sound` 01-08 and 11

The Blargg ROMs are downloaded into `tests/roms/` on first run (needs `curl`)
and executed headless (no SDL window). Output is `PASS`/`FAIL` per test. For a
failing ROM, the final screen text is saved to `build/<rom>.log`.

How it works: Blargg ROMs print their verdict as ASCII tiles on the background
map, so `tests/run_rom.cpp` runs the emulator core without SDL and reads the
verdict from VRAM. The other tests are standalone programs that exit non-zero
on assertion failure.

## Layout

```
src/cpu.cpp        CPU: registers, opcodes, interrupts
src/memory.cpp     Address bus, echo RAM, IO register routing
src/ppu.cpp        Scanline renderer (background, window, sprites), LCD modes, STAT
src/timer.cpp      16-bit system counter driving DIV and TIMA
src/input.cpp      Joypad register (0xFF00) and joypad interrupt
src/apu.cpp        Audio: channels, frame sequencer, stereo mixer (44.1 kHz)
src/cartridge.cpp  ROM loading, MBC1/MBC5 banking, external RAM, .sav files
src/main.cpp       SDL frontend (video, audio, input) and main loop
include/            Headers (cpu.h, memory.h, ppu.h, timer.h, input.h, apu.h, cartridge.h)
tests/run_rom.cpp  Headless Blargg ROM runner
tests/stat_test.cpp, joypad_test.cpp, ppu_test.cpp, cartridge_test.cpp, apu_test.cpp   Standalone unit tests
tests/fetch_roms.sh  Downloads the Blargg ROMs
```

## License

See [LICENSE](LICENSE).
