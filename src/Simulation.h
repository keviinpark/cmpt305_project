#include "Instruction.h"
#include "Pipeline.h"


/**
 * This class represents the overall simulation.
 * 
 */
class Simulation
{
    private:
        Pipeline cpu;
        std::vector<Instruction*> instructions;
        std::string trace_file;
        long long cycle_count;

    public:
        Simulation(const std::string& trace_file, long long start_inst, long long inst_count, int pipeline_depth) : trace_file(trace_file) {}

        void run_simulation()
        {
            /* Open trace file */
            
            /* Main simulation loop */
            while(!cpu_is_done() || !trace_reached_end())
            {
                cpu.advance_pipeline();
            
                if (!cpu.is_stalled())
                {
                    for (int i = 0; i < 2; i++)
                    {
                        Instruction* next_instruction = get_next_instruction();
                        cpu.insert_instruction(next_instruction);

                        if (next_instruction->type == 3)
                        {
                            cpu.set_branch_stall(true);
                            break;
                        }
                    }
                }

                cycle_count++;
            }
        }
};