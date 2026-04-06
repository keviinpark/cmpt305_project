#include "Instruction.h"
#include "Pipeline.h"


class Simulation
{
    private:
        Pipeline cpu;
        std::vector<Instruction*> instructions;
        std::string trace_file;

    public:
        Simulation(const std::string& trace_file, long long start_inst, long long inst_count, int pipeline_depth) : trace_file(trace_file) {}

        void run_simulation()
        {
            /* Open trace file */

            /* Main simulation loop */

            /* while (!cpu_is_done() || !!trace_reached_end())
            {
                if (cpu.has_space_in_pipeline())
                {
                    Instruction next_instruction = get_next_instruction();
                    cpu.insert_instruction(next_instruction);
                }

                cpu.advance_pipeline();
            } */
        }

        // print_stats();
};