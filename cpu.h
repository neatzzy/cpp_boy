#pragma once
#include "memory.h"
#include <cstdint>
#include <cstdio>
#include <stdexcept>

struct CPU{
    // 8-bit registers
    uint8_t A, F;
    uint8_t B, C;
    uint8_t D, E;
    uint8_t H, L;
    // 16-bit registers
    uint16_t SP;
    // Stack Pointer starts at top of memory
    uint16_t PC;
    // Internal control flags
    bool IME;
    // Interrupt Master Enable
    bool enableIME_next;
    // Used to delay enabling IME until after the next instruction

    bool halted;
    // Indicates if the CPU is halted
    bool stopped;
    // Indicates if the CPU is stopped

    // Memory reference
    Memory& memory;
    // Constructor
    CPU(Memory& mem);

    // Debug
    void dumpRegisters();

    void push(uint16_t value);

    uint16_t pop();

    void setCarryFlag(bool value);

    void setHalfCarryFlag(bool value);

    void setZeroFlag(bool value);

    void setSubtractFlag(bool value);

    void di();

    void ei();

    void haltCPU();

    void stopCPU();

    bool interruptPending();

    void handleInterrupt();

    int step();

    uint16_t read16Immediate();

    uint8_t read8Immediate();

    uint8_t inc8(uint8_t value);

    uint8_t dec8(uint8_t value);

    void addHL(uint16_t value);

    void rlca();

    void rrca();

    void rla();

    void rra();

    void rl(uint8_t reg);

    void rlc(uint8_t reg);

    void rr(uint8_t reg);

    void rrc(uint8_t reg);

    void sla(uint8_t reg);

    void sra(uint8_t reg);

    void srl(uint8_t reg);

    void swap(uint8_t reg);

    void daa();

    void cpl();

    void scf();

    void ccf();

    void jr(int8_t offset);

    void ret(uint8_t condition);

    void ret();

    void reti();

    void jp(uint16_t addr);

    bool checkCondition(uint8_t condition);

    void call(uint8_t condition);

    void call();

    void rst(uint8_t target);

    void addA(uint8_t operand);

    void adcA(uint8_t operand);

    void subA(uint8_t operand);

    void sbcA(uint8_t operand);

    void andA(uint8_t operand);

    void xorA(uint8_t operand);

    void orA(uint8_t operand);

    void cpA(uint8_t operand);

    void addSP(int8_t offset);

    // Helper functions to access 16-bit registers
    uint16_t getAF() const;

    uint16_t getBC() const;

    uint16_t getDE() const;

    uint16_t getHL() const;

    void writeAF(uint16_t value);

    void writeBC(uint16_t value);

    void writeDE(uint16_t value);

    void writeHL(uint16_t value);

    // Functions to read/write registers based on opcode encoding
    uint8_t readReg(uint8_t code);

    uint16_t readReg16(uint8_t code);

    void writeReg(uint8_t code, uint8_t value);

    void writeReg16(uint8_t code, uint16_t value);

    void executeALU(uint8_t operation, uint8_t operand);

    int handleLoad(uint8_t opcode);

    int handleArithmetic8Bit(uint8_t opcode);

    int handleArithmetic16Bit(uint8_t opcode);

    int handleBitwise(uint8_t opcode);

    int handleJumpAndSubroutine(uint8_t opcode);

    int handleCarryFlag(uint8_t opcode);

    int handleStack(uint8_t opcode);

    int executeCBOpcode(uint8_t opcode);

    int executeOpcode(uint8_t opcode);

};
