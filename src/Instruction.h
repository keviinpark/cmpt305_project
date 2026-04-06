#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>
#include <vector>

struct Instruction
{
    /* Hexadecimal value of instruction address */
    uint32_t program_counter;

    /* 1: Integer instruction
     * 2: Floating point instruction
     * 3: Branch
     * 4: Load
     * 5: Store
     */
    int instruction_type;

    /* List of PCs that this instruction depends on */
    std::vector<uint32_t> dependencies;

    /* 0: Not started
     * 1: IF
     * 2: ID
     * 3: EX
     * 4: MEM
     * 5: WB
     */
    int current_stage = 0; 
}; 

#endif /* INSTRUCTION_H */