#ifndef SIMULATION_H
#define SIMULATION_H

#include <cstdint>
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

        bool parse_trace_window();
        static bool parse_trace_line(const std::string& line,
                                     uint32_t& program_counter,
                                     int& instruction_type,
                                     std::vector<uint32_t>& dependency_pcs);

    public:
        Simulation(const std::string& trace_file, long long start_inst, long long inst_count, int pipeline_depth);
        ~Simulation();

        void run_simulation();
};

#endif /* SIMULATION_H */