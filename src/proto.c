#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

// Sign Extension
// fills zeroes for positive numbers, ones for negative numbers
uint16_t sign_extend(uint16_t x, int bit_count){
    if ((x >> (bit_count-1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

// Update Flags
// checks if a number is positive, negative or zero and updates the condition flag accordingly
void update_flags(uint16_t r){
    if (registers[r] == 0) {
        registers[R_COND] = FL_ZRO;
    } else if (registers[r] >> 15) { // 1 in the left most bit indicates a negative number
        registers[R_COND] = FL_NEG;
    } else {
        registers[R_COND] = FL_POS;
    }
}


// Main function
int main(int argc, char *argv[])
{
    // Show the usage of the command
    if (argc < 2) {
        printf("proto [image-file1]...\n");
        exit(1);
    }

    // Checking if all given image files are valid
    for (int j = 1; j < argc; j++) {
        if (!read_image(argv[j])) {
            printf("ERROR : failed to load image %s\n", argv[j]);
            exit(1);
        }
    }

    // TODO: Setup

    // Initializing the condition flag to zero
    registers[R_COND] = FL_ZRO;

    // Setting the PC up to starting position
    enum{ PC_START = 0x3000 };
    registers[R_PC] = PC_START;

    int running = 1;
    while (running) {
        // Get the next operation
        uint16_t instr = mem_read(registers[R_PC]++);
        uint16_t op = instr >> 12;

        // Switch case to handle the operation input
        switch (op) {
            case OP_ADD:
                break;
            case OP_AND:
                break;
            case OP_NOT:
                break;
            case OP_BR:
                break;
            case OP_JMP:
                break;
            case OP_JSR:
                break;
            case OP_LD:
                break;
            case OP_LDI:
                break;
            case OP_LDR:
                break;
            case OP_LEA:
                break;
            case OP_ST:
                break;
            case OP_STI:
                break;
            case OP_STR:
                break;
            case OP_TRAP:
                break;
            case OP_RES:
            case OP_RTI:
            default:
                // TODO: Bad Code
                break;
        }
    }
    // Shutdown the PC
}
