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
fetch snd01.gb "dmg_sound/rom_singles/01-registers.gb"
fetch snd02.gb "dmg_sound/rom_singles/02-len%20ctr.gb"
fetch snd03.gb "dmg_sound/rom_singles/03-trigger.gb"
fetch snd04.gb "dmg_sound/rom_singles/04-sweep.gb"
fetch snd05.gb "dmg_sound/rom_singles/05-sweep%20details.gb"
fetch snd06.gb "dmg_sound/rom_singles/06-overflow%20on%20trigger.gb"
fetch snd07.gb "dmg_sound/rom_singles/07-len%20sweep%20period%20sync.gb"
fetch snd08.gb "dmg_sound/rom_singles/08-len%20ctr%20during%20power.gb"
fetch snd09.gb "dmg_sound/rom_singles/09-wave%20read%20while%20on.gb"
fetch snd10.gb "dmg_sound/rom_singles/10-wave%20trigger%20while%20on.gb"
fetch snd11.gb "dmg_sound/rom_singles/11-regs%20after%20power.gb"
fetch snd12.gb "dmg_sound/rom_singles/12-wave%20write%20while%20on.gb"
