#ifndef PIPELINE_H
#define PIPELINE_H

#include <deque>
#include <unordered_map>
#include <vector>
#include "Instruction.h"

/**
 * This class represents the 5-stage pipeline of the simulator.
 *
 */
class Pipeline
{
private:
    /* 5 stages of the pipeline */
    std::deque<Instruction *> pipeline_stages[6];

    bool branch_stall;
    bool clear_branch_stall;

    int depth_config;
    int get_ex_cycles(InstructionType instruction_type);
    int get_mem_cycles(InstructionType instruction_type);

public:
    Pipeline(int d_config) : branch_stall(false), clear_branch_stall(false), depth_config(d_config) {}

    void advance_pipeline(long long *instruction_type_count);

    void process_IF();
    void process_ID();
    void process_EX();
    void process_MEM();
    void process_WB(long long *instruction_type_count);

    bool is_stalled();
    void insert_instruction(Instruction *instruction);
    bool is_done();
};

#endif /* PIPELINE_H */