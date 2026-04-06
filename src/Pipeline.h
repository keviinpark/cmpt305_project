#include <vector>
#include <deque>
#include <unordered_map>
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

        
    public:
        Pipeline () : alu_busy(false), fp_busy(false), branch_busy(false), l1_read_busy(false), l1_write_busy(false), branch_stall(false) {}

        void advance_pipeline()
        {
            process_WB();
            process_MEM();
            process_EX();
            process_ID();
            process_IF();
        }

        void process_IF();
        void process_ID();
        void process_EX();
        void process_MEM();
        void process_WB();

};