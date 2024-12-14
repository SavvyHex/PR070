#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

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

// Trap Routines
// predefined commonly used routines
enum{
    TRAP_GETC = 0x20,   // Reads a single ASCII character
    TRAP_OUT = 0x21,    // Outputs a character
    TRAP_PUTS = 0x22,   // Prints a null-terminated string
    TRAP_IN = 0x23,     // Prompt for input character
    TRAP_PUTSP = 0x24,  // Outputs a string
    TRAP_HALT = 0x25,   // Halts (stops) the program
}; 

// Memory Mapped Registers
// used to read and write to registers
enum{
    MR_KBSR = 0xFE00, // KeyBoard Status Register
    MR_KBDR = 0xFE02, // KeyBoard Data Register
};

struct termios original_tio;

void disable_input_buffering(){
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering(){
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

uint16_t check_key(){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

void handle_interrupt(int signal){
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

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

// Swap each uint16 that is loaded
// LC3 is big-endian but modern computers are little-endian
uint16_t swap16(uint16_t x){
    return (x << 8) | (x >> 8);
}

// Reading image files
// a function to read image files
void read_image_file(FILE* file){
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);
    uint16_t max_read = MAX_MEMORY - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);
    while (read --> 0) {
        *p = swap16(*p);
        ++p;
    }
}

// Reading images
int read_image(const char* image_path){
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0;};
    read_image_file(file);
    fclose(file);
    return 1;
}

// Writing to memory
void mem_write(uint16_t address, uint16_t val){
    memory[address] = val;
}

// Reading from memory
uint16_t mem_read(uint16_t address){
    if (address == MR_KBSR) {
        if (check_key()) {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        } else {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
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

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

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

        uint16_t r0, r1, r2, imm_flag, pc_offset;
        uint16_t* c;
        // Switch case to handle the operation input
        switch (op) {
            case OP_ADD:
                r0 = (instr >> 9) & 0x7; // Destination register
                r1 = (instr >> 6) & 0x7; // First operand
                imm_flag =  (instr >> 5) & 0x1; // Checking if we're in immediate mode
                if (imm_flag) {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    registers[r0] = registers[r1] + imm5;
                } else {
                    uint16_t r2 = instr & 0x7;
                    registers[r0] = registers[r1] + registers[r2];
                }
                update_flags(r0);
                break;
            case OP_AND:
                r0 = (instr >> 9) & 0x7; // Destination register
                r1 = (instr >> 6) & 0x7; // First operand
                imm_flag =  (instr >> 5) & 0x1; // Checking if we're in immediate mode
                if (imm_flag) {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    registers[r0] = registers[r1] & imm5;
                } else {
                    uint16_t r2 = instr & 0x7;
                    registers[r0] = registers[r1] & registers[r2];
                }
                update_flags(r0);
                break;
            case OP_NOT:
                r0 = (instr >> 9) & 0x7; // Destination register
                r1 = (instr >> 6) & 0x7; // First operand
                registers[r0] = ~registers[r1];
                update_flags(r0);
                break;
            case OP_BR:
                pc_offset = sign_extend(instr & 0x1FF, 9);
                uint16_t cond_flag = (instr >> 9) & 0x7;
                if (cond_flag & registers[R_COND]) {
                    registers[R_PC] += pc_offset;
                }
                break;
            case OP_JMP:
                r1 = (instr >> 6) & 0x7; // First operand
                registers[R_PC] += registers[r1];
                break;
            case OP_JSR:
                uint16_t long_flag = (instr >> 11) & 1;
                registers[R_R7] = registers[R_PC];
                if (long_flag) {
                    uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
                    registers[R_PC] = registers[r1];
                }
                break;
            case OP_LD:
                r0 = (instr >> 9) & 0x7; // Destination register
                pc_offset = sign_extend(instr & 0x1FF, 9);
                registers[r0] = mem_read(registers[R_PC] + pc_offset);
                update_flags(r0);
                break;
            case OP_LDI:
                r0 = (instr >> 9) & 0x7; // Destination register
                pc_offset = mem_read(mem_read(registers[R_PC] + pc_offset));
                update_flags(r0);
                break;
            case OP_LDR:
                r0 = (instr >> 9) & 0x7; // Destination register
                r1 = (instr >> 6) & 0x7; // First operand
                uint16_t offset = sign_extend(instr & 0x3F, 6);
                registers[r0] = mem_read(registers[r1] + offset);
                update_flags(r0);
                break;
            case OP_LEA:
                r0 = (instr >> 9) & 0x7; // Destination register
                pc_offset = sign_extend(instr & 0x1FF, 9);
                registers[r0] = registers[R_PC] + pc_offset;
                update_flags(r0);
                break;
            case OP_ST:
                r0 = (instr >> 9) & 0x7; // Destination register
                pc_offset = sign_extend(instr & 0x1FF, 9);
                mem_write(registers[R_PC] + pc_offset, registers[r0]);
                break;
            case OP_STI:
                r0 = (instr >> 9) & 0x7; // Destination register
                pc_offset = sign_extend(instr & 0x1FF, 9);
                mem_write(mem_read(registers[R_PC] + pc_offset), registers[r0]);
                break;
            case OP_STR:
                r0 = (instr >> 9) & 0x7; // Destination register
                r1 = (instr >> 6) & 0x7; // First operand
                offset = sign_extend(instr & 0x3F, 6);
                mem_write(registers[r1] + offset, registers[r0]);
                break;
            case OP_TRAP:
                registers[R_R7] = registers[R_PC];
                switch (instr & 0xFF) {
                    case TRAP_GETC:
                        registers[R_R0] = (uint16_t)getchar();
                        update_flags(R_R0);
                        break;
                    case TRAP_OUT:
                        putc((char)registers[R_R0], stdout);
                        break;
                    case TRAP_PUTS:
                        c = memory + registers[R_R0];
                        while (*c) {
                            putc((char)*c, stdout);
                            ++c;
                        }
                        fflush(stdout);
                        break;
                    case TRAP_IN:
                        printf("Enter a character : ");
                        char in_c = getchar();
                        putc(in_c, stdout);
                        fflush(stdout);
                        registers[R_R0] = (uint16_t)c;
                        update_flags(R_R0);
                        break;
                    case TRAP_PUTSP:
                        c = memory + registers[R_R0];
                        while (*c) {
                            char char1 = (*c) & 0xFF;
                            putc(char1, stdout);
                            char char2 = (*c) >> 8;
                            if (char2) putc(char2, stdout);
                            ++c;
                        }
                        fflush(stdout);
                        break;
                    case TRAP_HALT:
                        puts("HALT");
                        fflush(stdout);
                        running = 0;
                        break;
                }
                break;
            case OP_RES:
            case OP_RTI:
            default:
                abort();
                break;
        }
    }
    restore_input_buffering();
}
