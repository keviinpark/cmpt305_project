#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>
#include <vector>


/**
 * This struct represents a single instruction in the pipeline.
 * 
 */
struct Instruction
{
    /* Instruction address in hex */
    uint32_t program_counter;

    /**
     * 1: Integer instruction
     * 2: Floating point instruction
     * 3: Branch
     * 4: Load
     * 5: Store
     */
    int instruction_type;

    /** 
     * List of instructions that this instruction depends on 
     * May be size 0-4
     */
    std::vector<Instruction*> dependencies;

    /**
     * 0: Not started
     * 1: Instruction Fetch (IF)
     * 2: Instruction Decode and Read Operands (ID)
     * 3: Execute (EX)
     * 4: Memory Access (MEM)
     * 5: Write Back Results/Retire (WB)
     */
    int current_stage = 0; 
}; 


#endif /* INSTRUCTION_H */