#include "Simulation.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace
{
std::string trim(const std::string& text)
{
	const std::size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
	{
		return "";
	}

	const std::size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}
}

Simulation::Simulation(const std::string& trace_file,
					   long long start_inst,
					   long long inst_count,
					   int pipeline_depth)
	: trace_file(trace_file),
	  start_inst(start_inst),
	  inst_count(inst_count),
	  pipeline_depth(pipeline_depth),
	  cycle_count(0)
{
}

Simulation::~Simulation()
{
	for (Instruction* inst : instructions)
	{
		delete inst;
	}
}

bool Simulation::parse_trace_line(const std::string& line,
								  uint32_t& program_counter,
								  int& instruction_type,
								  std::vector<uint32_t>& dependency_pcs)
{
	dependency_pcs.clear();

	if (line.empty())
	{
		return false;
	}

	std::stringstream line_stream(line);
	std::string token;
	std::vector<std::string> tokens;

	while (std::getline(line_stream, token, ','))
	{
		tokens.push_back(trim(token));
	}

	if (tokens.size() < 2)
	{
		return false;
	}

	try
	{
		program_counter = static_cast<uint32_t>(std::stoull(tokens[0], nullptr, 16));
		instruction_type = std::stoi(tokens[1]);
	}
	catch (const std::exception&)
	{
		return false;
	}

	if (instruction_type < 1 || instruction_type > 5)
	{
		return false;
	}

	for (std::size_t i = 2; i < tokens.size(); ++i)
	{
		if (tokens[i].empty())
		{
			continue;
		}

		try
		{
			dependency_pcs.push_back(static_cast<uint32_t>(std::stoull(tokens[i], nullptr, 16)));
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	return true;
}

bool Simulation::parse_trace_window()
{
	std::ifstream input(trace_file);
	if (!input.is_open())
	{
		std::cerr << "Error: failed to open trace file: " << trace_file << std::endl;
		return false;
	}

	std::string line;
	long long trace_index = 0;
	long long loaded = 0;
	std::unordered_map<uint32_t, Instruction*> last_instance;

	while (std::getline(input, line))
	{
		if (trace_index < start_inst)
		{
			++trace_index;
			continue;
		}

		if (loaded >= inst_count)
		{
			break;
		}

		uint32_t pc = 0;
		int type = 0;
		std::vector<uint32_t> dependency_pcs;
		if (!parse_trace_line(line, pc, type, dependency_pcs))
		{
			std::cerr << "Warning: skipping malformed trace line at index "
					  << trace_index << std::endl;
			++trace_index;
			continue;
		}

		Instruction* instruction = new Instruction();
		instruction->program_counter = pc;
		instruction->instruction_type = type;

		for (uint32_t dep_pc : dependency_pcs)
		{
			const auto it = last_instance.find(dep_pc);
			if (it != last_instance.end())
			{
				instruction->dependencies.push_back(it->second);
			}
		}

		instructions.push_back(instruction);
		last_instance[pc] = instruction;

		++trace_index;
		++loaded;
	}

	if (loaded < inst_count)
	{
		std::cerr << "Warning: requested " << inst_count
				  << " instructions, but only loaded " << loaded
				  << " from trace starting at " << start_inst << std::endl;
	}

	return !instructions.empty();
}

void Simulation::run_simulation()
{
	if (!parse_trace_window())
	{
		std::cerr << "Error: no instructions loaded for simulation." << std::endl;
		return;
	}

	std::size_t issued = 0;
	const std::size_t total = instructions.size();

	while (issued < total || !cpu.is_done())
	{
		cpu.advance_pipeline();

		if (!cpu.is_stalled() && issued < total)
		{
			for (int i = 0; i < 2 && issued < total; ++i)
			{
				Instruction* next_instruction = instructions[issued];
				cpu.insert_instruction(next_instruction);
				++issued;

				if (next_instruction->instruction_type == 3)
				{
					cpu.set_branch_stall(true);
					break;
				}
			}
		}

		++cycle_count;
	}

	(void)pipeline_depth;
}