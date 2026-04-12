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

        /* Maps PC to most recent dynamic instruction instance */
        std::unordered_map<uint32_t, Instruction*> last_instance;

        /* Flags for resource availability */
        bool alu_busy, fp_busy, branch_busy, l1_read_busy, l1_write_busy;

        bool branch_stall;

        int depth_config;
        int get_ex_cycles(InstructionType instruction_type);
        int get_mem_cycles(InstructionType instruction_type);

        bool check_unit_avail(InstructionType instruction_type);
        void reserve_unit(InstructionType instruction_type);

        
    public:
        Pipeline (int d_config) : alu_busy(false), fp_busy(false), branch_busy(false), l1_read_busy(false), l1_write_busy(false), branch_stall(false), depth_config(d_config) {}

        void advance_pipeline(long long* instruction_type_count)
        {
            process_WB(instruction_type_count);
            process_MEM();
            process_EX();
            process_ID();
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