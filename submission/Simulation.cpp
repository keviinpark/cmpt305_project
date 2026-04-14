#include "Simulation.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>

Simulation::Simulation(std::string &trace_file,
                       long long start_inst,
                       long long inst_count,
                       int pipeline_depth)
    : trace_file(trace_file),
      start_inst(start_inst),
      inst_count(inst_count),
      pipeline_depth(pipeline_depth),
      cycle_count(0),
      cpu(pipeline_depth)

{
}

Simulation::~Simulation()
{
    for (Instruction *inst : instructions)
    {
        delete inst;
    }
}

bool Simulation::load_trace()
{

    std::ifstream file(trace_file);
    if (!file.is_open())
    {
        std::cerr << "Error: Couldn't open file: " << trace_file << std::endl;
        return false;
    }
    std::unordered_map<uint64_t, Instruction *> last_seen;

    std::string line;
    long long current_idx = 0;
    long long loaded_so_far = 0;

    while (std::getline(file, line) && loaded_so_far < inst_count)
    {
        if (current_idx < start_inst)
        {
            current_idx++;
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ','))
        {
            tokens.push_back(token);
        }

        if (tokens.size() < 2)
            continue; // Basic safety check

        Instruction *instruction = new Instruction();
        instruction->program_counter = std::stoull(tokens[0], nullptr, 16);
        instruction->instruction_type = static_cast<InstructionType>(std::stoi(tokens[1]));

        for (size_t i = 2; i < tokens.size(); ++i)
        {
            if (tokens[i].empty())
                continue;

            uint64_t dep_pc = std::stoull(tokens[i], nullptr, 16);

            if (last_seen.find(dep_pc) != last_seen.end())
            {
                instruction->dependencies.push_back(last_seen[dep_pc]);
            }
        }

        last_seen[instruction->program_counter] = instruction;

        instructions.push_back(instruction);
        loaded_so_far++;
        current_idx++;
    }

    file.close();
    return true;
}

void Simulation::print_stats()
{

    double freq_ghz = get_frequency();
    double execution_time_ms = (cycle_count / freq_ghz) / 1e6;
    long long total_instructions = 0;
    for (int i = 1; i <= 5; i++)
    {
        total_instructions += instruction_type_count[i];
    }

    std::cout << "===== Final Statistics =====" << std::endl;
    std::cout << "Total cycles: " << cycle_count << std::endl;
    std::cout << "Frequency: " << freq_ghz << " GHz" << std::endl;
    std::cout << "Execution time: " << execution_time_ms << " ms" << std::endl;

    std::cout << "\nInstruction type histogram:" << std::endl;
    const char *names[] = {"", "Integer", "Floating Point", "Branch", "Load", "Store"};

    double percentage;
    for (int i = 1; i <= 5; ++i)
    {
        if (total_instructions > 0)
        {
            percentage = (instruction_type_count[i] * 100.0) / total_instructions;
        }
        else
        {
            percentage = 0.0;
        }

        printf("%-15s: %lld (%.2f%%)\n", names[i], instruction_type_count[i], percentage);
    }
}

void Simulation::run_simulation()
{
    if (!load_trace())
    {
        std::cerr << "Error: No instructions loaded for simulation." << std::endl;
        return;
    }

    else
    {
        std::cout << "Loaded " << instructions.size() << " instructions for simulation." << std::endl;
    }

    std::size_t issued = 0;
    std::size_t total = instructions.size();

    while (issued < total || !cpu.is_done())
    {
        if (!cpu.is_stalled() && issued < total)
        {
            for (int i = 0; i < 2 && issued < total; ++i)
            {
                cpu.insert_instruction(instructions[issued]);
                ++issued;
            }
        }

        cpu.advance_pipeline(instruction_type_count);
        ++cycle_count;
    }
}