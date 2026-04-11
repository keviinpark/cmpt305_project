#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>
#include <vector>
#include <stdio.h>


typedef enum {
    INT_INST = 1,
    FP_INST = 2,
    BRANCH_INST = 3,
    LOAD_INST = 4, 
    STORE_INST = 5
} InstructionType;

typedef enum {
    NOT_STARTED = 0,
    IF_STAGE = 1,
    ID_STAGE = 2,
    EX_STAGE = 3,
    MEM_STAGE = 4,
    WB_STAGE = 5
} InstructionStage;


/**
 * This struct represents a single instruction in the pipeline.
 * 
 */
struct Instruction
{
    /* Instruction address in hex */
    uint64_t program_counter;

    InstructionType instruction_type;

    /** 
     * List of instructions that this instruction depends on 
     * May be size 0-4
     */
    std::vector<Instruction*> dependencies;

    InstructionStage current_stage = NOT_STARTED; 

    /* Number of cycles remaining in the current stage depending on pipeline depth */
    int cycles_remaining = 0;
}; 


#endif /* INSTRUCTION_H */