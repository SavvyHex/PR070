#include <stdint.h>

// Storage
#define MAX_MEMORY (1<<16)
uint16_t memory[MAX_MEMORY];

// Registers
// There are 10 registers, each of 16 bits
// R0 to R7 are general purpose registers, used to perform any program calculations
// Each index is auto-incremented
enum{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,       // Program Counter : address of next instruction to execute
    R_COND,     // Condition Flag : positive, negative, zero
    R_COUNT     // Number of registers
};

// Storing the registers
uint16_t registers[R_COUNT];

// Condition flags
// Stored in R_COND
// Gives info about most recently executed calculation
enum{
    FL_POS = 1 << 0,    // Positive
    FL_ZRO = 1 << 1,    // Zero
    FL_NEG = 1 << 2,    // Negative
};

// Instruction Set
// Each instruction has an OPcode and parameters
// There are 16 OPcodes
enum{
    OP_BR = 0,  // Branch
    OP_ADD,     // Add
    OP_LD,      // Load
    OP_ST,      // Store
    OP_JSR,     // Jump Register
    OP_AND,     // Bitwise and
    OP_LDR,     // Load register
    OP_STR,     // Store register
    OP_RTI,     // Return from interrupt (unused)
    OP_NOT,     // Bitwise not
    OP_LDI,     // Load indirect
    OP_STI,     // Store indirect
    OP_JMP,     // Jump register
    OP_RES,     // Reserved (unused)
    OP_LEA,     // Load Effective Address
    OP_TRAP,    // Execute trap
};

