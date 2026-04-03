#include <vector>
#include <deque>
#include <unordered_map>
#include "Instruction.h"

class Pipeline 
{
    private: 
        /* Represents the 5 stages of the pipeline */
        std::deque<Instruction*> pipeline_stages[5]; 

        /* Maps PC to most recent dynamic instruction instance */
        std::unordered_map<uint32_t, int> instruction_status;

        bool alu_busy, fp_busy, branch_busy, l1_read_busy, l1_write_busy;

        long long cycle_count = 0;
    
    public:
        Pipeline () : alu_busy(false), fp_busy(false), branch_busy(false), l1_read_busy(false), l1_write_busy(false) {}

        void advance_pipeline()
        {
            process_WB();
            process_MEM();
            process_EX();
            process_ID();
            process_IF();

            cycle_count++;
        }

        void process_WB();
        void process_MEM();
        void process_EX();
        void process_ID();
        void process_IF();
};