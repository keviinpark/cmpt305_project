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
        std::deque<Instruction*> pipeline_stages[5]; 

        bool branch_stall;
        bool clear_branch_stall;

        int depth_config;
        int get_ex_cycles(InstructionType instruction_type);
        int get_mem_cycles(InstructionType instruction_type);

        
    public:
        Pipeline (int d_config) : branch_stall(false), clear_branch_stall(false), depth_config(d_config) {}

        /**
         * Phase 1: Advance WB, MEM, EX, ID stages and update stall state.
         * Call this before feeding new instructions into the pipeline.
         */
        void advance_back_end(long long* instruction_type_count)
        {
            if (clear_branch_stall)
            {
                branch_stall = false;
                clear_branch_stall = false;
            }
            process_WB(instruction_type_count);
            process_MEM();
            process_EX();
            process_ID();
        }

        /**
         * Phase 2: Run the IF stage (fetch waiting instructions).
         * Call this AFTER inserting new instructions so they can be fetched
         * in the same cycle that the branch stall clears.
         */
        void advance_front_end()
        {
            process_IF();
        }

        void process_IF();
        void process_ID();
        void process_EX();
        void process_MEM();
        void process_WB(long long* instruction_type_count);

        bool is_stalled();
        void insert_instruction(Instruction* instruction);
        bool is_done();

};

    #endif /* PIPELINE_H */