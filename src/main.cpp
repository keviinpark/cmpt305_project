#include "Simulation.h"
#include <iostream>
#include <string>


int main(int argc, char* argv[]) {
    // Expecting: ./cpu_simulator trace_file_path start_inst inst_count D
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <trace_file> <start_inst> <inst_count> <D>" << std::endl;
        return 1;
    }

    std::string trace_file = argv[1];
    
    long long start_inst = std::stoll(argv[2]);
    long long inst_count  = std::stoll(argv[3]);
    int pipeline_depth   = std::stoi(argv[4]); 

    if (pipeline_depth < 1 || pipeline_depth > 4)
	{
		std::cerr << "Error: pipeline depth D must be between 1 and 4." << std::endl;
		return -1;
	}

    // Initialize and run
    Simulation sim(trace_file, start_inst, inst_count, pipeline_depth);
    sim.run_simulation();
    sim.print_stats();

    return 0;
}