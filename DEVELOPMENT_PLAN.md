# Plano de Desenvolvimento — cpp_boy (Game Boy Emulator)

Atualizado em 2026-11-11, com base em auditoria do commit `9984582`.

## Estado atual (medido, não presumido)

Build limpo (`make`, zero warnings com `-Wall`). ~2.300 linhas, 12 commits, zero testes.

Resultado de cada ROM local, rodada headless (núcleo sem SDL, leitura do tilemap
como texto):

| ROM | Header | Resultado |
|---|---|---|
| `cpu_instrs.gb` | MBC1, 64KB | `01:ok`, depois trava em STOP (sem banking de ROM) |
| `instr_timing.gb` | MBC1, 32KB | `Failed #255` (timer) |
| `halt_bug.gb` | MBC1+RAM, 32KB | `Failed` (apesar do commit `9984582`) |
| `interrupt_time.gb` | MBC1+RAM, 32KB | `Failed` — ROM é CGB-only, falha esperada em DMG |
| `tetris.gb` | ROM only | LCD desligado, preso em loop de reset |
| `mario.gb` | MBC1, 64KB | CPU travada em opcode ilegal em `0xFF48` (sem banking) |
| `pokemon.gb` (Yellow) | **MBC5**+RAM+BATTERY, 1MB | não sai do boot |

Nenhuma ROM passa nem é jogável hoje.

### Bugs confirmados

1. **Joypad lê "tudo pressionado"** — `0xFF00` cai em `ram[]` (`src/memory.cpp:33`),
   nibble baixo = 0 = todos os botões apertados. Tetris interpreta
   A+B+Start+Select como soft reset, em loop. Confirmado: forçando `0xFF00 → 0xFF`
   o Tetris chega à tela de título.
2. **STOP trava para sempre** — `src/cpu.cpp:126` retorna 4 enquanto `stopped`, e
   nada limpa a flag (não existe joypad). É o que mata `cpu_instrs.gb`.
3. **Timer impreciso** — `src/timer.cpp:35-36`: usa `if` em vez de `while` e
   reatribui o contador em vez de somar o período (perde o resto dos ciclos a cada
   tick de TIMA). Escrita em DIV (`src/memory.cpp:39`) não zera `div_counter`.
   Provável causa do `Failed #255`.
4. **Interrupt STAT em tempestade** — `src/ppu.cpp:59-62` pede interrupt a cada
   `tick()` enquanto `LY == LYC`, sem detecção de borda. Deveria disparar uma vez.
5. **BIOS obrigatória e inútil** — `main.cpp:28` aborta se `gb_bios.bin` não
   existir; o arquivo é carregado em `ram[0..255]`, mas leituras `< 0x8000` vão
   sempre para o cartucho. `*.bin` está no `.gitignore`: clone limpo não roda.
6. **Sem limite de velocidade** — renderer sem `SDL_RENDERER_PRESENTVSYNC` nem
   delay (`main.cpp:39`); roda o mais rápido que a máquina permite.
7. **`printf` de registradores a cada frame** (`main.cpp:62`) — resto de debug.
8. **Registradores somente-leitura são graváveis** — jogo pode escrever em LY
   (`0xFF44`) e nos bits de modo do STAT (`0xFF41`).

### Lacunas (não implementado)

- Joypad (`src/input.cpp` vazio, fora do `Makefile`)
- PPU: sprites, window, interrupts STAT de modo 0/1/2
- MBC (qualquer um) e save RAM
- Serial, APU

### Higiene

- `Makefile`: sem `-O2`, sem `-MMD` (mudar header não recompila), sem `-Wextra`.
- Código morto: `Memory::push/pop` (duplicam `CPU::push/pop`, sem chamadas),
  `PPU::readRegister/writeRegister` declarados e nunca definidos, `frame_ready`
  nunca lido.
- `*.gb` no `.gitignore`: ROMs de teste só existem localmente, CI precisa baixá-las.
- Commits fora do padrão Conventional Commits.

## Fase 0 — Harness de teste ✅ (concluída 2026-11-11)

`make test` baixa as ROMs (`tests/fetch_roms.sh`), roda `tests/run_rom.cpp`
headless e grava a tela de cada falha em `build/<rom>.log`. Baseline:

| ROM | Resultado |
|---|---|
| 01, 03, 05, 06, 07, 08, 10 | PASS |
| 02-interrupts | `Timer doesn't work` / `Failed #4` |
| 04-op r,imm | `DE` (SBC A,n) |
| 09-op r,r | `98 99 9A 9B 9C 9D` (SBC A,r) |
| 11-op a,(hl) | `9E` (SBC A,(HL)) |
| instr_timing | `Failed #255` (timer) |
| halt_bug | `Failed` |

Achado novo: as três falhas de 04/09/11 são todas `SBC` — um único bug em
`CPU::sbcA`. Entra como item 1 da Fase 1.

Plano original da fase:

Motivo: o commit do HALT bug declarou a correção sem verificação, e ela falha.
Sem teste automatizado, cada fase seguinte repete isso.

- `tests/run_rom.cpp`: núcleo sem SDL, roda N frames, lê o tilemap `0x9800` como
  ASCII (tiles Blargg = ASCII) e retorna exit code por "Passed"/"Failed". ~40 linhas.
- `make test`: compila o harness com os `.cpp` do núcleo (tudo menos `main.cpp`)
  e roda as ROMs.
- Baixar as ROMs individuais do Blargg (`cpu_instrs/individual/*.gb`, 32KB, não
  precisam de MBC) — destrava validar a CPU inteira já.
- Remover `interrupt_time.gb` da lista (CGB-only).
- `Makefile`: `-O2 -MMD -Wextra`.

Pulado: serial e CI. Texto na tela já dá pass/fail; adicionar GitHub Actions
quando `make test` estiver verde.

## Fase 1 — Corrigir o que já existe ✅ (concluída 2026-11-11)

Resultado: `make test` 100% verde (stat_test + 01–11, instr_timing, halt_bug).
Causas raiz: `sbcA` lia o carry-in depois de sobrescrevê-lo; timer reescrito
com contador de sistema de 16 bits e borda de descida; `halt_bug` não era HALT,
era o IF devolvendo bits 5–7 zerados (hardware devolve 1); STAT com linha única
e detecção de borda. Pendente da fase: nada. `cpu_instrs.gb` combinado ainda
precisa de MBC1 (Fase 3). Simplificações conhecidas: reload de TIMA sem atraso
de 1 M-cycle (Fase 5).

Plano original da fase:

0. `CPU::sbcA`: alvo 04, 09, 11 verdes.
1. Timer: contador interno de 16 bits compartilhado por DIV e TIMA; escrita em
   DIV zera tudo. Alvo: `instr_timing.gb` e 02 verdes.
2. HALT bug: investigar causa com o timer já correto. Alvo: `halt_bug.gb` verde.
3. `cpu_instrs` individuais 01–11 verdes.
4. STAT: borda de subida única (linha STAT interna), cobrindo LYC + modos 0/1/2.
5. `0xFF00` devolve `0xCF` (nada pressionado) até o joypad real existir.
6. Remover `loadBIOS` de `main.cpp`, o `printf` por frame e o código morto.
7. Ligar vsync (`SDL_RENDERER_PRESENTVSYNC`).

## Fase 2 — Tetris jogável ✅ (implementada 2026-11-11; falta validação visual)

Joypad, sprites e window com testes (`joypad_test`, `ppu_test`), `make test` verde.
Pendente: confirmar `tetris.gb` jogável rodando `./emulator` com janela.

1. Joypad: registro `0xFF00` (seleção bits 4/5, 0 = pressionado), interrupt bit 4,
   eventos SDL em `main.cpp`. STOP passa a acordar com botão.
2. Sprites: OAM, 8x8 e 8x16, flip X/Y, OBP0/OBP1, prioridade, 10 por linha.
3. Window: LCDC bits 5/6, WX/WY, contador de linha interno.

Verificação: `tetris.gb` jogável do título ao game over.

## Fase 3 — Cartuchos ✅ (implementada 2026-11-17; falta validação visual)

MBC1, MBC5, RAM externa e `.sav` com `cartridge_test`; `cpu_instrs.gb`
combinado passa. Bug extra corrigido: KEY1 (`0xFF4D`) lia 0 e o ROM achava
que era CGB. Headless 1200 frames: Mario e Pokémon ligam o LCD e desenham sem
travar. Pendente: jogar com janela e confirmar save do Pokémon entre execuções.
`mem_timing` falha (Fase 5).

1. **MBC1** — destrava `mario.gb` e o `cpu_instrs.gb` combinado.
2. **MBC5 + RAM + bateria** (`.sav` ao lado da ROM, carrega no início, salva ao
   sair) — destrava `pokemon.gb`.

Pulado: MBC3/RTC. Nenhuma ROM do repo usa; adicionar quando aparecer uma.

## Fase 4 — Áudio (APU)

4 canais + NR50/NR51/NR52, saída via SDL Audio. Não bloqueia jogabilidade.

## Fase 5 — Precisão (opcional)

Delay de 1 M-cycle no reload de TIMA, DMA cycle-accurate, boot ROM real +
`0xFF50`, PPU FIFO, testes mooneye-gb.
