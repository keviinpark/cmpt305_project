#ifndef SIMULATION_H
#define SIMULATION_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include "Instruction.h"
#include "Pipeline.h"

/**
 * This class represents the overall simulation.
 */
class Simulation
{
    private:
        std::vector<Instruction*> instructions;
        std::string trace_file;
        long long start_inst;
        long long inst_count;
        int pipeline_depth;
        long long cycle_count;
        Pipeline cpu;
        long long instruction_type_count[6] = {0}; //  INT, FP, BR, LD, ST

        /* Pipeline Depth affects CPU frequency */
        double get_frequency() {
            if (pipeline_depth == 2) return 1.2;
            if (pipeline_depth == 3) return 1.7;
            if (pipeline_depth == 4) return 1.8;
            return 1.0;
        }

        bool load_trace();

    public:
        Simulation(std::string& trace_file, long long start_inst, long long inst_count, int pipeline_depth);
        ~Simulation();
        void print_stats();

        void run_simulation();
};

#endif /* SIMULATION_H */
