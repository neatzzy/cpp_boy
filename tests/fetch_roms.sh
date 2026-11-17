#!/bin/sh
# Downloads Blargg test ROMs into tests/roms (gitignored).
set -e
BASE=https://raw.githubusercontent.com/retrio/gb-test-roms/master
DIR=$(dirname "$0")/roms
mkdir -p "$DIR"

fetch() {
    [ -f "$DIR/$1" ] || curl -sfL -o "$DIR/$1" "$BASE/$2"
}

fetch 01.gb "cpu_instrs/individual/01-special.gb"
fetch 02.gb "cpu_instrs/individual/02-interrupts.gb"
fetch 03.gb "cpu_instrs/individual/03-op%20sp%2Chl.gb"
fetch 04.gb "cpu_instrs/individual/04-op%20r%2Cimm.gb"
fetch 05.gb "cpu_instrs/individual/05-op%20rp.gb"
fetch 06.gb "cpu_instrs/individual/06-ld%20r%2Cr.gb"
fetch 07.gb "cpu_instrs/individual/07-jr%2Cjp%2Ccall%2Cret%2Crst.gb"
fetch 08.gb "cpu_instrs/individual/08-misc%20instrs.gb"
fetch 09.gb "cpu_instrs/individual/09-op%20r%2Cr.gb"
fetch 10.gb "cpu_instrs/individual/10-bit%20ops.gb"
fetch 11.gb "cpu_instrs/individual/11-op%20a%2C(hl).gb"
fetch instr_timing.gb "instr_timing/instr_timing.gb"
fetch halt_bug.gb "halt_bug.gb"
fetch cpu_instrs.gb "cpu_instrs/cpu_instrs.gb"
