#ifndef SIMULATION_H
#define SIMULATION_H

#include <string>
#include <vector>
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
        long long start_inst;
        long long inst_count;
        int pipeline_depth;
        long long cycle_count;

    public:
        Simulation(const std::string& trace_file, long long start_inst, long long inst_count, int pipeline_depth)
            : trace_file(trace_file),
              start_inst(start_inst),
              inst_count(inst_count),
              pipeline_depth(pipeline_depth),
              cycle_count(0) {}

        ~Simulation()
        {
            for (Instruction* inst : instructions)
            {
                delete inst;
            }
        }

        void run_simulation()
        {
            long long issued = 0;
            while (issued < inst_count || !cpu.is_done())
            {
                cpu.advance_pipeline();

                if (!cpu.is_stalled() && issued < inst_count)
                {
                    for (int i = 0; i < 2; i++)
                    {
                        if (issued >= inst_count)
                        {
                            break;
                        }

                        Instruction* next_instruction = new Instruction();

                        /* To-do: input parsing */
                        next_instruction->program_counter = static_cast<uint32_t>(start_inst + issued);
                        next_instruction->instruction_type = static_cast<InstructionType>((issued % 5) + 1);
                        next_instruction->dependencies = {};
                        /* Instruction current stage is determined by pipeline*/

                        instructions.push_back(next_instruction);

                        cpu.insert_instruction(next_instruction);
                        issued++;

                        /* Any branch instruction delays instruction fetch until after branch executes */
                        if (next_instruction->instruction_type == BRANCH_INST)
                        {
                            cpu.set_branch_stall(true);
                            break;
                        }
                    }
                }

                cycle_count++;
            }

            (void)trace_file;
            (void)pipeline_depth;
        }
};

#endif /* SIMULATION_H */