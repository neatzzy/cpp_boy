#include "cpu.h"
#include <stdexcept>

CPU::CPU(Memory& mem) : memory(mem) {}

void CPU::dumpRegisters() {
        printf("Instruction at PC: %02X\n", memory.read(PC));
        printf("AF: %02X%02X BC: %02X%02X DE: %02X%02X HL: %02X%02X SP: %04X PC: %04X\n", A, F, B, C, D, E, H, L, SP, PC);
    }

void CPU::push(uint16_t value) {
        memory.push(value, SP);
    }

uint16_t CPU::pop() {
        return memory.pop(SP);
    }

void CPU::setCarryFlag(bool value) {
        if(value) {
            F |= 0b00010000; // Set carry flag
        } else {
            F &= ~0b00010000; // Clear carry flag
        }
    }

void CPU::setHalfCarryFlag(bool value) {
        if(value) {
            F |= 0b00100000; // Set half-carry flag
        } else {
            F &= ~0b00100000; // Clear half-carry flag
        }
    }

void CPU::setZeroFlag(bool value) {
        if(value) {
            F |= 0b10000000; // Set zero flag
        } else {
            F &= ~0b10000000; // Clear zero flag
        }
    }

void CPU::setSubtractFlag(bool value) {
        if(value) {
            F |= 0b01000000; // Set subtract flag
        } else {
            F &= ~0b01000000; // Clear subtract flag
        }
    }

void CPU::di() {
        // Implementation to disable interrupts
        IME = false;
    }

void CPU::ei() {
        // Implementation to enable interrupts
        enableIME_next = true; // Enable IME after the next instruction
    }

void CPU::haltCPU() {
        // Implementation to halt the CPU
        halted = true;
    }

void CPU::stopCPU() {
        // Implementation to stop the CPU
        stopped = true;
    }

bool CPU::interruptPending() {
        uint8_t IF = memory.read(0xFF0F); // Interrupt Flag
        uint8_t IE = memory.read(0xFFFF); // Interrupt Enable

        return (IF & IE) != 0; // Check if any enabled interrupt is pending
    }

void CPU::handleInterrupt() {
        uint8_t IF = memory.read(0xFF0F); // Interrupt Flag
        uint8_t IE = memory.read(0xFFFF); // Interrupt Enable

        for (int i = 0; i < 5; i++) {
            if ((IF & IE) & (1 << i)) {
                // Clear the interrupt flag for this interrupt
                memory.write(0xFF0F, IF & ~(1 << i));

                // Push PC onto stack
                push(PC);

                // Jump to the appropriate interrupt vector
                PC = 0x40 + i * 8;

                IME = false; // Disable further interrupts until re-enabled by EI
                break;
            }
        }
    }

void CPU::step() {
        if(halted) {
            if(interruptPending()) halted = false;
            else return;
        }

        uint8_t opcode = memory.read(PC++);
        executeOpcode(opcode);

        if(enableIME_next) {
            IME = true;
            enableIME_next = false;
        }

        if(IME) {
            // Check for interrupts and handle them
            if(interruptPending()) {
                handleInterrupt();
            }
        }
    }

uint16_t CPU::read16Immediate() {
        // Implementation to read a 16-bit immediate value from memory
        uint16_t value = memory.read16(PC);
        PC += 2;
        return value;
    }

uint8_t CPU::read8Immediate() {
        // Implementation to read an 8-bit immediate value from memory
        uint8_t value = memory.read(PC);
        PC += 1;
        return value;
    }

uint8_t CPU::inc8(uint8_t value) {
        // Increment an 8-bit value and set flags accordingly
        uint8_t result = value + 1;
        setZeroFlag(result == 0);
        setHalfCarryFlag((value & 0xF) == 0xF);
        setSubtractFlag(false);
        return result;
    }

uint8_t CPU::dec8(uint8_t value) {
        // Decrement an 8-bit value and set flags accordingly
        uint8_t result = value - 1;
        setZeroFlag(result == 0);
        setHalfCarryFlag((value & 0xF) == 0);
        setSubtractFlag(true);
        return result;
    }

void CPU::addHL(uint16_t value) {
        // Add value to HL and set flags accordingly
        uint32_t result = getHL() + value;
        setCarryFlag(result > 0xFFFF);
        setHalfCarryFlag(((getHL() & 0xFFF) + (value & 0xFFF)) > 0xFFF);
        setSubtractFlag(false);
        writeHL(result & 0xFFFF);
    }

void CPU::rlca() {
        // Rotate A left with carry
        bool carry = A & 0x80;
        A = (A << 1) | (carry ? 1 : 0);
        setCarryFlag(carry);
        setZeroFlag(A == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::rrca() {
        // Rotate A right with carry
        bool carry = A & 0x01;
        A = (A >> 1) | (carry ? 0x80 : 0);
        setCarryFlag(carry);
        setZeroFlag(A == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::rla() {
        // Rotate A left
        bool carry = A & 0x80;
        A = (A << 1) | (F & 0b00010000 ? 1 : 0);
        setCarryFlag(carry);
        setZeroFlag(A == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::rra() {
        // Rotate A right
        bool carry = A & 0x01;
        A = (A >> 1) | (F & 0b00010000 ? 0x80 : 0);
        setCarryFlag(carry);
        setZeroFlag(A == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::rl(uint8_t reg) {
        // Rotate r8 left
        uint8_t value = readReg(reg);
        bool carry = value & 0x80;
        value = (value << 1) | (F & 0b00010000 ? 1 : 0);
        writeReg(reg, value);
        setCarryFlag(carry);
        setZeroFlag(value == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::rlc(uint8_t reg) {
        // Rotate r8 left with carry
        uint8_t value = readReg(reg);
        bool carry = value & 0x80;
        value = (value << 1) | (carry ? 1 : 0);
        writeReg(reg, value);
        setCarryFlag(carry);
        setZeroFlag(value == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::rr(uint8_t reg) {
        // Rotate r8 right
        uint8_t value = readReg(reg);
        bool carry = value & 0x01;
        value = (value >> 1) | (carry ? 0x80 : 0);
        writeReg(reg, value);
        setCarryFlag(carry);
        setZeroFlag(value == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::rrc(uint8_t reg) {
        // Rotate r8 right with carry
        uint8_t value = readReg(reg);
        bool carry = value & 0x01;
        value = (value >> 1) | (carry ? 0x80 : 0);
        writeReg(reg, value);
        setCarryFlag(carry);
        setZeroFlag(value == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);

    }

void CPU::sla(uint8_t reg) {
        // Shift r8 left into carry
        uint8_t value = readReg(reg);
        bool carry = value & 0x80;
        value <<= 1;
        writeReg(reg, value);
        setCarryFlag(carry);
        setZeroFlag(value == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::sra(uint8_t reg) {
        // Shift r8 right into carry, MSB does not change
        uint8_t value = readReg(reg);
        bool carry = value & 0x01;
        value = (value >> 1) | (value & 0x80);
        writeReg(reg, value);
        setCarryFlag(carry);
        setZeroFlag(value == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::srl(uint8_t reg) {
        // Shift r8 right into carry, MSB becomes 0
        uint8_t value = readReg(reg);
        bool carry = value & 0x01;
        value >>= 1;
        writeReg(reg, value);
        setCarryFlag(carry);
        setZeroFlag(value == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::swap(uint8_t reg) {
        // Swap upper and lower nibbles of r8
        uint8_t value = readReg(reg);
        value = (value << 4) | (value >> 4);
        writeReg(reg, value);
        setCarryFlag(false);
        setZeroFlag(value == 0);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::daa() {
        // Decimal Adjust Accumulator
        uint8_t correction = 0;
        if ((F & 0b01000000) || (A & 0x0F) > 9) {
            correction |= 0x06;
        }
        if ((F & 0b00010000) || A > 0x99) {
            correction |= 0x60;
        }
        if (F & 0b01000000) {
            subA(correction);
        } else {
            addA(correction);
        }
        setZeroFlag(A == 0);
        setHalfCarryFlag(false);


    }

void CPU::cpl() {
        // Complement A
        A = ~A;
        setSubtractFlag(true);
        setHalfCarryFlag(true);
    }

void CPU::scf() {
        // Set Carry Flag
        setCarryFlag(true);
        setSubtractFlag(false);
        setHalfCarryFlag(false);
    }

void CPU::ccf() {
        // Complement Carry Flag
            F ^= 0b00010000; // Toggle carry flag
            setSubtractFlag(false);
            setHalfCarryFlag(false);
    }

void CPU::jr(int8_t offset) {
        // Jump relative
        PC += offset;
    }

void CPU::jr(uint8_t condition, int8_t offset) {
        // Jump relative if condition is true
        if (checkCondition(condition)) jr(offset);
    }

void CPU::ret(uint8_t condition) {
        // Return if condition is true
            if (checkCondition(condition)) ret();
    }

void CPU::ret() {
        // Return from subroutine
        PC = pop();
    }

void CPU::reti() {
        // Return from interrupt
        ei(); // Re-enable interrupts
        ret();
    }

void CPU::jp(uint8_t condition) {
        // Jump if condition is true
            if (checkCondition(condition)) jp(read16Immediate());
    }

void CPU::jp(uint16_t addr) {
        // Jump to address
        PC = addr;
    }

bool CPU::checkCondition(uint8_t condition) {
        // Check condition based on flags
        switch(condition) {
            case 0: return (F & 0b10000000) == 0; // NZ
            case 1: return (F & 0b10000000) != 0; // Z
            case 2: return (F & 0b00010000) == 0; // NC
            case 3: return (F & 0b00010000) != 0; // C
            default: throw "Invalid condition code";
        }
    }

void CPU::call(uint8_t condition) {
        // Call subroutine if condition is true
        if (checkCondition(condition)) call();
    }

void CPU::call() {
        // Call subroutine
        uint16_t addr = read16Immediate();
        push(PC); 
        jp(addr);
    }

void CPU::rst(uint8_t target) {
        // Call to RST vector
        call(target * 8);
    }

void CPU::addA(uint8_t operand) {
        // Add operand to A
            uint8_t result = A + operand;
            setCarryFlag(result > 0xFF);
            setHalfCarryFlag(((A & 0xF) + (operand & 0xF)) > 0xF);
            setZeroFlag(result == 0);
            setSubtractFlag(false);
            A = result;
    }

void CPU::adcA(uint8_t operand) {
        // Add operand and carry flag to A
        uint8_t result = A + operand + (F & 0b00010000 ? 1 : 0);
        setCarryFlag(result > 0xFF);
        setHalfCarryFlag(((A & 0xF) + (operand & 0xF) + (F & 0b00010000 ? 1 : 0)) > 0xF);
        setZeroFlag(result == 0);
        setSubtractFlag(false);
        A = result;
    }

void CPU::subA(uint8_t operand) {
        // Subtract operand from A
        uint8_t result = A - operand;
        setCarryFlag(result > A);
        setHalfCarryFlag((A & 0xF) < (operand & 0xF));
        setZeroFlag(result == 0);
        setSubtractFlag(true);
        A = result;
    }

void CPU::sbcA(uint8_t operand) {
        // Subtract operand and carry flag from A
        uint8_t result = A - operand - (F & 0b00010000 ? 1 : 0);
        setCarryFlag(result > A);
        setHalfCarryFlag((A & 0xF) < (operand & 0xF) + (F & 0b00010000 ? 1 : 0));
        setZeroFlag(result == 0);
        setSubtractFlag(true);
        A = result;
    }

void CPU::andA(uint8_t operand) {
        // AND operand with A
        A &= operand;
        setCarryFlag(false);
        setHalfCarryFlag(true);
        setZeroFlag(A == 0);
        setSubtractFlag(false);
    }

void CPU::xorA(uint8_t operand) {
        // XOR operand with A
        A ^= operand;
        setCarryFlag(false);
        setHalfCarryFlag(false);
        setZeroFlag(A == 0);
        setSubtractFlag(false);
    }

void CPU::orA(uint8_t operand) {
        // OR operand with A
        A |= operand;
        setCarryFlag(false);
        setHalfCarryFlag(false);
        setZeroFlag(A == 0);
        setSubtractFlag(false);
    }

void CPU::cpA(uint8_t operand) {
        // Compare operand with A (sets flags but does not store result)
        uint8_t result = A - operand;
        setCarryFlag(result > A);
        setHalfCarryFlag((A & 0xF) < (operand & 0xF));
        setZeroFlag(result == 0);
        setSubtractFlag(true);
    }

void CPU::addSP(int8_t offset) {
        // Add signed offset to SP
        uint16_t result = SP + offset;
        setCarryFlag((SP & 0xFF) + (offset & 0xFF) > 0xFF);
        setHalfCarryFlag((SP & 0xF) + (offset & 0xF) > 0xF);
        setZeroFlag(false);
        setSubtractFlag(false);
        SP = result;
    }

uint16_t CPU::getAF() const { return (A << 8) | F; }

uint16_t CPU::getBC() const { return (B << 8) | C; }

uint16_t CPU::getDE() const { return (D << 8) | E; }

uint16_t CPU::getHL() const { return (H << 8) | L; }

void CPU::writeAF(uint16_t value) { A = value >> 8; F = value & 0xFF; }

void CPU::writeBC(uint16_t value) { B = value >> 8; C = value & 0xFF; }

void CPU::writeDE(uint16_t value) { D = value >> 8; E = value & 0xFF; }

void CPU::writeHL(uint16_t value) { H = value >> 8; L = value & 0xFF; }

uint8_t CPU::readReg(uint8_t code) {
        switch(code){
            case 0: return B;
            case 1: return C;
            case 2: return D;
            case 3: return E;
            case 4: return H;
            case 5: return L;
            case 6: 
                // Read from memory address pointed by HL
                return memory.read(getHL()); 
            case 7: return A;
            default: throw "Invalid register code";
        }
    }

uint16_t CPU::readReg16(uint8_t code) {
        switch(code){
            case 0: return getBC();
            case 1: return getDE();
            case 2: return getHL();
            case 3: return SP;
            default: throw "Invalid register code";
        }
    }

void CPU::writeReg(uint8_t code, uint8_t value) {
        switch(code){
            case 0: B = value; break;
            case 1: C = value; break;
            case 2: D = value; break;
            case 3: E = value; break;
            case 4: H = value; break;
            case 5: L = value; break;
            case 6: 
                // Write to memory address pointed by HL
                memory.write(getHL(), value);
                break;
            case 7: A = value; break;
            default: throw "Invalid register code";
        }
    }

void CPU::writeReg16(uint8_t code, uint16_t value) {
        switch(code){
            case 0: writeBC(value); break;
            case 1: writeDE(value); break;
            case 2: writeHL(value); break;
            case 3: SP = value; break;
            default: throw "Invalid register code";
        }
    }

void CPU::executeALU(uint8_t operation, uint8_t operand) {
        switch(operation){
            case 0x00: addA(operand); break;
            case 0x01: adcA(operand); break;
            case 0x02: subA(operand); break;
            case 0x03: sbcA(operand); break;
            case 0x04: andA(operand); break;
            case 0x05: xorA(operand); break;
            case 0x06: orA(operand); break;
            case 0x07: cpA(operand); break;
            default: throw "Invalid ALU operation code";
        }
    }

bool CPU::handleLoad(uint8_t opcode) {
        // ==============================
        // LD [HL+], A
        // 00 10 0010
        // ==============================
        if (opcode == 0x22) {
            memory.write(getHL(), A);
            writeHL(getHL() + 1);
            return true;
        }

        // ==============================
        // LD [HL-], A
        // 00 11 0010
        // ==============================
        if (opcode == 0x32) {
            memory.write(getHL(), A);
            writeHL(getHL() - 1);
            return true;
        }

        // ==============================
        // LD A, [HL+]
        // 00 10 1010
        // ==============================
        if (opcode == 0x2A) {
            A = memory.read(getHL());
            writeHL(getHL() + 1);
            return true;
        }

        // ==============================
        // LD A, [HL-]
        // 01 11 1010
        // ==============================
        if (opcode == 0x3A) {
            A = memory.read(getHL());
            writeHL(getHL() - 1);
            return true;
        }

        // ==============================
        // LD [HL], r8
        // 00 110 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b01110000) {
            uint8_t src = opcode & 0b111;
            memory.write(getHL(), readReg(src));
            return true;
        }

        // ==============================
        // LD r8, [HL]
        // 0x46, 0x4E, 0x56, 0x5E, 0x66, 0x6E, 0x7E
        // ==============================
        if ((opcode & 0b11111000) == 0b01000000) {
            uint8_t dest = (opcode >> 3) & 0b111;
            writeReg(dest, memory.read(getHL()));
            return true;
        }

        // ==============================
        // LD r8,r8
        // 01 DDD SSS
        // ==============================
        if ((opcode & 0b11000000) == 0b01000000) {
            if (opcode == 0x76) { // HALT
                haltCPU();
                return true;
            }
            uint8_t dest = (opcode >> 3) & 0b111;
            uint8_t src  = opcode & 0b111;
            writeReg(dest, readReg(src));
            return true;
        }

        // ==============================
        // LD r8, imm8
        // 00 DDD 110
        // ==============================
        if ((opcode & 0b11000111) == 0b00000110) {
            uint8_t dest = (opcode >> 3) & 0b111;
            writeReg(dest, read8Immediate());
            return true;
        }

        // ==============================
        // LD [r16], A
        // 00 RR 0010
        // ==============================
        if ((opcode & 0b11001111) == 0b00000010) {
            uint8_t reg = (opcode >> 4) & 0b11;
            memory.write(readReg16(reg), A);
            return true;
        }

        // ==============================
        // LD [imm16], A
        // 11101010
        // ==============================
        if (opcode == 0xEA) {
            uint16_t addr = read16Immediate();
            memory.write(addr, A);
            return true;
        }

        // ==============================
        // LDH [imm16], A
        // 11100000
        // ==============================
        if (opcode == 0xE0) {
            uint8_t addr = 0xFF00 + read8Immediate();
            memory.write(addr, A);
            return true;
        }

        // ==============================
        // LDH [C], A
        // 11100010
        // ==============================
        if (opcode == 0xE2) {
            uint16_t addr = 0xFF00 + C;
            memory.write(addr, A);
            return true;
        }

        // ==============================
        // LD A, [imm16]
        // 11111010
        // ==============================
        if (opcode == 0xFA) {
            uint16_t addr = read16Immediate();
            A = memory.read(addr);
            return true;
        }

        // ==============================
        // LDH A, [imm16]
        // 11110000
        // ==============================
        if (opcode == 0xF0) {
            uint16_t addr = 0xFF00 + read8Immediate();
            A = memory.read(addr);
            return true;
        }

        // ==============================
        // LDH A, [C]
        // 11110010
        // ==============================
        if (opcode == 0xF2) {
            uint16_t addr = 0xFF00 + C;
            A = memory.read(addr);
            return true;
        }

        // ==============================
        // LD r16, imm16
        // 00 RR 0001
        // ==============================
        if ((opcode & 0b11001111) == 0b00000001) {
            uint8_t reg = (opcode >> 4) & 0b11;
            writeReg16(reg, read16Immediate());
            return true;
        }

        return false;
    }

bool CPU::handleArithmetic8Bit(uint8_t opcode) {
        // ==============================
        // ADC A, r8
        // 10001 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b10001000) {
            uint8_t src = opcode & 0b111;
            if(src == 0b110){ // ADC A, [HL]
                executeALU(0x01, memory.read(getHL()));
            } else 
            executeALU(0x01, readReg(src));
            return true;
        }

        // ==============================
        // ADC A, imm8
        // 11001110
        // ==============================
        if (opcode == 0xCE) {
            executeALU(0x01, read8Immediate());
            return true;
        }

        // ==============================
        // ADD A, r8
        // 10000 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b10000000) {
            uint8_t src = opcode & 0b111;
            if(src == 0b110){ // ADD A, [HL]
                executeALU(0x00, memory.read(getHL()));
            } else
            executeALU(0x00, readReg(src));
            return true;
        }

        // ==============================
        // ADD A, imm8
        // 11000110
        // ==============================
        if (opcode == 0xC6) {
            executeALU(0x00, read8Immediate());
            return true;
        }

        // ==============================
        // CP A, r8
        // 10111 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b10111000) {
            uint8_t src = opcode & 0b111;
            if(src == 0b110){ // CP A, [HL]
                executeALU(0x07, memory.read(getHL()));
            } else
            executeALU(0x07, readReg(src));
            return true;
        }

        // ==============================
        // CP A, imm8
        // 11111110
        // ==============================
        if (opcode == 0xFE) {
            executeALU(0x07, read8Immediate());
            return true;
        }

        // ==============================
        // DEC r8
        // 00 RRR 101
        // ==============================
        if ((opcode & 0b11000111) == 0b00000101) {
            uint8_t reg = (opcode >> 3) & 0b111;
            if(reg == 0b110){ // DEC [HL]
                uint8_t value = memory.read(getHL());
                uint8_t result = dec8(value);
                memory.write(getHL(), result);
                return true;
            }
            writeReg(reg, dec8(readReg(reg)));
            return true;
        }

        // ==============================
        // INC r8
        // 00 RRR 100
        // ==============================
        if ((opcode & 0b11000111) == 0b00000100) {
            uint8_t reg = (opcode >> 3) & 0b111;
            if(reg == 0b110){ // INC [HL]
                uint8_t value = memory.read(getHL());
                uint8_t result = inc8(value);
                memory.write(getHL(), result);
                return true;
            }
            writeReg(reg, inc8(readReg(reg)));
            return true;
        }

        // ==============================
        // SBC A, r8
        // 10011 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b10011000) {
            uint8_t src = opcode & 0b111;
            if(src == 0b110){ // SBC A, [HL]
                executeALU(0x03, memory.read(getHL()));
            } else
            executeALU(0x03, readReg(src));
            return true;
        }

        // ==============================
        // SBC A, imm8
        // 11011110
        // ==============================
        if (opcode == 0xDE) {
            executeALU(0x03, read8Immediate());
            return true;
        }

        // ==============================
        // SUB A, r8
        // 10010 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b10010000) {
            uint8_t src = opcode & 0b111;
            if(src == 0b110){ // SUB A, [HL]
                executeALU(0x02, memory.read(getHL()));
            } else {
                executeALU(0x02, readReg(src));
            }
            return true;
        }

        // ==============================
        // SUB A, imm8
        // 11010110
        // ==============================
        if (opcode == 0xD6) {
            executeALU(0x02, read8Immediate());
            return true;
        }

        return false;
    }

bool CPU::handleArithmetic16Bit(uint8_t opcode) {
        // ==============================
        // ADD HL, r16
        // 00 RR 1001
        // ==============================
        if ((opcode & 0b11001111) == 0b00001001) {
            uint8_t reg = (opcode >> 4) & 0b11;
            addHL(readReg16(reg));
            return true;
        }

        // ==============================
        // DEC r16
        // 00 RR 1011
        // ==============================
        if ((opcode & 0b11001111) == 0b00001011) {
            uint8_t reg = (opcode >> 4) & 0b11;
            writeReg16(reg, readReg16(reg) - 1);
            return true;
        }

        // ==============================
        // INC r16
        // 00 RR 0011
        // ==============================
        if ((opcode & 0b11001111) == 0b00000011) {
            uint8_t reg = (opcode >> 4) & 0b11;
            writeReg16(reg, readReg16(reg) + 1);
            return true;
        }
        return false;
    }

bool CPU::handleBitwise(uint8_t opcode) {
        // ==============================
        // AND A, r8
        // 10100 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b10100000) {
            uint8_t src = opcode & 0b111;
            if(src == 0b110){ // AND A, [HL]
                executeALU(0x04, memory.read(getHL()));
            } else {
                executeALU(0x04, readReg(src));
            }
            return true;
        }

        // ==============================
        // AND A, imm8
        // 11100110
        // ==============================
        if (opcode == 0xE6) {
            executeALU(0x04, read8Immediate());
            return true;
        }

        // ==============================
        // CPL
        // 00101111
        // ==============================
        if (opcode == 0x2F) {
            cpl();
            return true;
        }

        // ==============================
        // OR A, r8
        // 10110 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b10110000) {
            uint8_t src = opcode & 0b111;
            if(src == 0b110){ // OR A, [HL]
                executeALU(0x06, memory.read(getHL()));
            } else {
                executeALU(0x06, readReg(src));
            }
            return true;
        }

        // ==============================
        // OR A, imm8
        // 11110110
        // ==============================
        if (opcode == 0xF6) {
            executeALU(0x06, read8Immediate());
            return true;
        }

        // ==============================
        // XOR A, r8
        // 10101 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b10101000) {
            uint8_t src = opcode & 0b111;
            if(src == 0b110){ // XOR A, [HL]
                executeALU(0x05, memory.read(getHL()));
            } else {
                executeALU(0x05, readReg(src));
            }
            return true;
        }

        // ==============================
        // XOR A, imm8
        // 11101110
        // ==============================
        if (opcode == 0xEE) {
            executeALU(0x05, read8Immediate());
            return true;
        }

        return false;
    }

bool CPU::handleBitFlag(uint8_t opcode) {
        // ==============================
        // BIT b3, r8
        // 01 BBB RRR
        // ==============================
        if ((opcode & 0b11000111) == 0b01000000) {
            uint8_t bit = (opcode >> 3) & 0b111;
            uint8_t src = opcode & 0b111;
            uint8_t value;
            if(src == 0b110){ // BIT b3, [HL]
                value = memory.read(getHL());
            } else {
                value = readReg(src);
            }
            setZeroFlag((value & (1 << bit)) == 0);
            setSubtractFlag(false);
            setHalfCarryFlag(true);
            return true;
        }
        
        // ==============================
        // RES b3, r8
        // 10 BBB RRR
        // ==============================
        if ((opcode & 0b11000111) == 0b10000000) {
            uint8_t bit = (opcode >> 3) & 0b111;
            uint8_t src = opcode & 0b111;
            uint8_t value;
            if(src == 0b110){ // RES b3, [HL]
                value = memory.read(getHL());
                value &= ~(1 << bit);
                memory.write(getHL(), value);
            } else {
                value = readReg(src);
                value &= ~(1 << bit);
                writeReg(src, value);
            }
            return true;
        }

        // ==============================
        // SET b3, r8
        // 11 BBB RRR
        // ==============================
        if ((opcode & 0b11000111) == 0b11000000) {
            uint8_t bit = (opcode >> 3) & 0b111;
            uint8_t src = opcode & 0b111;
            uint8_t value;
            if(src == 0b110){ // SET b3, [HL]
                value = memory.read(getHL());
                value |= (1 << bit);
                memory.write(getHL(), value);
            } else {
                value = readReg(src);
                value |= (1 << bit);
                writeReg(src, value);
            }
            return true;
        }

        return false;
    }

bool CPU::handleBitShift(uint8_t opcode) {
        // ==============================
        // RL r8
        // 00010 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b00010000) {
            uint8_t reg = opcode & 0b111;
            if(reg == 0b110){ // RL [HL]
                uint8_t value = memory.read(getHL());
                bool carry = value & 0x80;
                value = (value << 1) | (F & 0b00010000 ? 1 : 0);
                memory.write(getHL(), value);
                setCarryFlag(carry);
                setZeroFlag(value == 0);
                setSubtractFlag(false);
                setHalfCarryFlag(false);
            } else {
                rl(reg);
            }
            return true;
        }

        // ==============================
        // RLA
        // 00010111
        // ==============================
        if (opcode == 0x17) {
            rla();
            return true;
        }

        // ==============================
        // RLC r8
        // 00000 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b00000000) {
            uint8_t reg = opcode & 0b111;
            if(reg == 0b110){ // RLC [HL]
                uint8_t value = memory.read(getHL());
                bool carry = value & 0x80;
                value = (value << 1) | (carry ? 1 : 0);
                memory.write(getHL(), value);
                setCarryFlag(carry);
                setZeroFlag(value == 0);  
                setSubtractFlag(false);
                setHalfCarryFlag(false);
            } else {
                rlc(reg);
            }
            return true;
        }    
        
        // ==============================
        // RLCA
        // 00000111
        // ==============================
        if (opcode == 0x07) {
            rlca();
            return true;
        }

        // ==============================
        // RR r8
        // 00011 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b00011000) {
            uint8_t reg = opcode & 0b111;
            if(reg == 0b110){ // RR [HL]
                uint8_t value = memory.read(getHL());
                bool carry = value & 0x01;
                value = (value >> 1) | (carry ? 0x80 : 0);
                memory.write(getHL(), value);
                setCarryFlag(carry);
                setZeroFlag(value == 0);
                setSubtractFlag(false);
                setHalfCarryFlag(false);
            } else {
                rr(reg);
            }
            return true;
        }

        // ==============================
        // RRA
        // 00011111
        // ==============================
        if (opcode == 0x1F) {
            rra();
            return true;
        }

        // ==============================
        // RRC r8
        // 00001 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b00001000) {
            uint8_t reg = opcode & 0b111;
            if(reg == 0b110){ // RRC [HL]
                uint8_t value = memory.read(getHL());
                bool carry = value & 0x01;
                value = (value >> 1) | (carry ? 0x80 : 0);
                memory.write(getHL(), value);
                setCarryFlag(carry);
                setZeroFlag(value == 0);
                setSubtractFlag(false);
                setHalfCarryFlag(false);
            } else {
                rrc(reg);
            }
            return true;
        }

        // ==============================
        // RRCA
        // 00001111
        // ==============================
        if (opcode == 0x0F) {
            rrca();
            return true;
        }

        // ==============================
        // SLA r8
        // 00100 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b00100000) {
            uint8_t reg = opcode & 0b111;
            if(reg == 0b110){ // SLA [HL]
                uint8_t value = memory.read(getHL());
                bool carry = value & 0x80;
                value <<= 1;
                memory.write(getHL(), value);
                setCarryFlag(carry);
                setZeroFlag(value == 0);
                setSubtractFlag(false);
                setHalfCarryFlag(false);
            } else {
                sla(reg);
            }
            return true;
        }

        // ==============================
        // SRA r8
        // 00101 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b00101000) {
            uint8_t reg = opcode & 0b111;
            if(reg == 0b110){ // SRA [HL]
                uint8_t value = memory.read(getHL());
                bool carry = value & 0x01;
                value = (value >> 1) | (value & 0x80);
                memory.write(getHL(), value);
                setCarryFlag(carry);
                setZeroFlag(value == 0);
                setSubtractFlag(false);
                setHalfCarryFlag(false);
            } else {
                sra(reg);
            }
            return true;
        }

        // ==============================
        // SRL r8
        // 00111 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b00111000) {
            uint8_t reg = opcode & 0b111;
            if(reg == 0b110){ // SRL [HL]
                uint8_t value = memory.read(getHL());
                bool carry = value & 0x01;
                value >>= 1;
                memory.write(getHL(), value);
                setCarryFlag(carry);
                setZeroFlag(value == 0);
                setSubtractFlag(false);
                setHalfCarryFlag(false);
            } else {
                srl(reg);
            }
            return true;
        }

        // ==============================
        // SWAP r8
        // 00110 RRR
        // ==============================
        if ((opcode & 0b11111000) == 0b00110000) {
            uint8_t reg = opcode & 0b111;
            if(reg == 0b110){ // SWAP [HL]
                uint8_t value = memory.read(getHL());
                value = (value << 4) | (value >> 4);
                memory.write(getHL(), value);
                setCarryFlag(false);
                setZeroFlag(value == 0);
                setSubtractFlag(false);
                setHalfCarryFlag(false);
            } else {
                swap(reg);
            }
            return true;
        }
        return false;
    }

bool CPU::handleJumpAndSubroutine(uint8_t opcode) {
        // ==============================
        // CALL imm16
        // 110 01 101
        // ==============================
        if (opcode == 0xCD) {
            call();
            return true;
        }

        // ==============================
        // CALL cond, imm16
        // 110 CC 100
        // ==============================
        if ((opcode & 0b11001111) == 0b11000100) {
            uint8_t condition = (opcode >> 4) & 0b11;
            call(condition);
            return true;
        }

        // ==============================
        // JP HL
        // 111 01 001
        // ==============================
        if (opcode == 0xE9) {
            PC = getHL();
            return true;
        }

        // ==============================
        // JP imm16
        // 110 00 011
        // ==============================
        if (opcode == 0xC3) {
            jp(read16Immediate());
            return true;
        }

        // ==============================
        // JP cond, imm16
        // 110 CC 010
        // ==============================
        if ((opcode & 0b11001111) == 0b11000010) {
            uint8_t condition = (opcode >> 4) & 0b11;
            jp(condition);
            return true;
        }

        // ==============================
        // JR imm8
        // 000 11 000
        // ==============================
        if (opcode == 0x18) {
            int8_t offset = (int8_t)read8Immediate();
            jr(offset);
            return true;
        }

        // ==============================
        // JR cond, imm8
        // 001 CC 000
        // ==============================
        if ((opcode & 0b11100111) == 0b00100000) {
            uint8_t condition = (opcode >> 3) & 0b11;
            int8_t offset = (int8_t)read8Immediate();
            jr(condition, offset);
            return true;
        }

        // ==============================
        // RET cond
        // 110 CC 000
        // ==============================
        if ((opcode & 0b11001111) == 0b11000000) {
            uint8_t condition = (opcode >> 4) & 0b11;
            ret(condition);
            return true;
        }

        // ==============================
        // RET
        // 110 01 001
        // ==============================
        if (opcode == 0xC9) {
            ret();
            return true;
        }

        // ==============================
        // RETI
        // 110 11 001
        // ==============================
        if (opcode == 0xD9) {
            reti();
            return true;
        }

        // ==============================
        // RST vec
        // 11 VVV 111
        // ==============================
        if ((opcode & 0b11000111) == 0b11000111) {
            uint8_t target = (opcode >> 3) & 0b111;
            rst(target);
            return true;
        }


        return false;
    }

bool CPU::handleCarryFlag(uint8_t opcode) {
        // ==============================
        // CCF
        // 00111111
        // ==============================
        if (opcode == 0x3F) {
            ccf();
            return true;
        }

        // ==============================
        // SCF
        // 00110111
        // ==============================
        if (opcode == 0x37) {
            scf();
            return true;
        }

        return false;
    }

bool CPU::handleStack(uint8_t opcode) {
        // ==============================
        // ADD SP, imm8
        // 11101000
        // ==============================
        if (opcode == 0xE8) {
            int8_t offset = (int8_t)read8Immediate();
            addSP(offset);
            return true;
        }
        
        // ==============================
        // LD imm16, SP
        // 00 00 1000
        // ==============================
        if (opcode == 0x08) {
            uint16_t addr = read16Immediate();
            memory.write(addr, SP & 0xFF);
            memory.write(addr + 1, SP >> 8);
            return true;
        }

        // ==============================
        // LD SP, HL
        // 11111001
        // ==============================
        if (opcode == 0xF9) {
            SP = getHL();
            return true;
        }

        // ==============================
        // POP AF
        // 11 11 0001
        // ==============================
        if (opcode == 0xF1) {
            uint8_t low = memory.read(SP++);
            uint8_t high = memory.read(SP++);
            A = high;
            F = low & 0xF0; // Lower 4 bits of F are always 0
            setZeroFlag((F & 0x80) != 0);
            setSubtractFlag((F & 0x40) != 0);
            setHalfCarryFlag((F & 0x20) != 0);
            setCarryFlag((F & 0x10) != 0);
            return true;
        }

        // ==============================
        // POP r16
        // 11 RR 0001
        // ==============================
        if ((opcode & 0b11001111) == 0b11000001) {
            uint8_t reg = (opcode >> 4) & 0b11;
            uint8_t low = memory.read(SP++);
            uint8_t high = memory.read(SP++);
            writeReg16(reg, (high << 8) | low);
            return true;
        }

        // ==============================
        // PUSH AF
        // 11 11 0101
        // ==============================
        if (opcode == 0xF5) {
            memory.write(--SP, A);
            memory.write(--SP, F & 0xF0);
            return true;
        }

        // ==============================
        // PUSH r16
        // 11 RR 0101
        // ==============================
        if ((opcode & 0b11001111) == 0b11000101) {
            uint8_t reg = (opcode >> 4) & 0b11;
            uint16_t value = readReg16(reg);
            memory.write(--SP, value >> 8);
            memory.write(--SP, value & 0xFF);
            return true;
        }

        return false;
    }

void CPU::executeOpcode(uint8_t opcode) {
        if (handleLoad(opcode)) return;
        if (handleArithmetic8Bit(opcode)) return;
        if (handleArithmetic16Bit(opcode)) return;
        if (handleBitwise(opcode)) return;
        if (handleBitFlag(opcode)) return;
        if (handleBitShift(opcode)) return;
        if (handleJumpAndSubroutine(opcode)) return;
        if (handleCarryFlag(opcode)) return;
        if (handleStack(opcode)) return;

        // ==============================
        // Misc / Unique Instructions
        // ==============================
        switch(opcode) {
            case 0x00: /* NOP */ break;
            case 0x27: daa(); break;
            case 0x10: stopCPU(); break;
            case 0xF3: di(); break;
            case 0xB3: ei(); break;
            default: throw "Unimplemented opcode: " + std::to_string(opcode);
        }
    }

