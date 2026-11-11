# cpp_boy

A Game Boy (DMG) emulator written in C++23, with SDL2 for video.

Work in progress: the CPU, timer and interrupts pass the Blargg test ROMs, but
the emulator is not yet able to play commercial games. See
[DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) for the roadmap.

## Status

| Component | State |
|---|---|
| CPU (all opcodes, CB prefix, interrupts, EI delay, HALT bug, illegal opcodes) | Done, passes Blargg `cpu_instrs` 01-11 |
| Timer (DIV/TIMA/TMA/TAC) | Done, passes `instr_timing` and `02-interrupts` |
| PPU background | Done |
| PPU STAT interrupts (LYC, modes 0/1/2) | Done |
| Joypad, sprites, window | In progress (Phase 2) |
| Cartridge banking (MBC1, MBC5) and battery saves | Not started (Phase 3) |
| Audio (APU) | Not started (Phase 4) |

Until MBC support lands, only 32 KB ROM-only cartridges run (e.g. Tetris).

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

Press `ESC` to quit. ROMs are not included; `*.gb` files are gitignored.

## Tests

```sh
make test
```

Downloads the Blargg test ROMs into `tests/roms/` on first run, executes them
headless (no SDL window) and prints `PASS`/`FAIL` per ROM. For a failing ROM,
the final screen text is saved to `build/<rom>.log`.

How it works: Blargg ROMs print their verdict as ASCII tiles on the background
map, so `tests/run_rom.cpp` runs the emulator core without SDL and reads the
verdict from VRAM. `tests/stat_test.cpp` covers the STAT interrupt line.

## Layout

```
src/cpu.cpp        CPU: registers, opcodes, interrupts
src/memory.cpp     Address bus, echo RAM, IO register routing
src/ppu.cpp        Scanline renderer, LCD modes, STAT
src/timer.cpp      16-bit system counter driving DIV and TIMA
src/cartridge.cpp  ROM loading
src/main.cpp       SDL frontend and main loop
tests/             Headless ROM runner, unit tests, ROM downloader
```

## License

See [LICENSE](LICENSE).
