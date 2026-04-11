#include "Simulation.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
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

long long get_parse_debug_limit()
{
	const char* env_value = std::getenv("SIM_DEBUG_PARSE_N");
	if (env_value == nullptr)
	{
		return 0;
	}

	try
	{
		const long long value = std::stoll(env_value);
		return (value > 0) ? value : 0;
	}
	catch (const std::exception&)
	{
		return 0;
	}
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
	  cycle_count(0),
	  cpu(pipeline_depth)

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
	const long long parse_debug_limit = get_parse_debug_limit();
	if (parse_debug_limit > 0)
	{
		std::cerr << "[parse-debug] showing first " << parse_debug_limit
				  << " parsed instructions" << std::endl;
	}

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
		instruction->instruction_type = static_cast<InstructionType>(type);

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

		if (parse_debug_limit > 0 && loaded < parse_debug_limit)
		{
			std::cerr << "[parse-debug] idx=" << trace_index
					  << " pc=0x" << std::hex << std::uppercase << instruction->program_counter
					  << std::dec << " type=" << instruction->instruction_type
					  << " dep-count=" << instruction->dependencies.size()
					  << " dep-pc-count=" << dependency_pcs.size()
					  << std::endl;
		}

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

double Simulation::get_frequency_ghz(int depth_config)
{
	switch (depth_config)
	{
		case 1: return 1.0;
		case 2: return 1.2;
		case 3: return 1.7;
		case 4: return 1.8;
		default: return 1.0;
	}
}

void Simulation::print_final_report(std::size_t simulated_count) const
{
	std::size_t histogram[6] = {0, 0, 0, 0, 0, 0};
	for (const Instruction* inst : instructions)
	{
		if (inst == nullptr)
		{
			continue;
		}

		const int type = static_cast<int>(inst->instruction_type);
		if (type >= 1 && type <= 5)
		{
			histogram[type]++;
		}
	}

	const double freq_ghz = get_frequency_ghz(pipeline_depth);
	const double cycle_time_seconds = 1.0 / (freq_ghz * 1e9);
	const double exec_time_ms = static_cast<double>(cycle_count) * cycle_time_seconds * 1000.0;

	const double total = (simulated_count == 0) ? 1.0 : static_cast<double>(simulated_count);

	std::cout << "=== Simulation Report ===" << std::endl;
	std::cout << "Trace file: " << trace_file << std::endl;
	std::cout << "Start instruction: " << start_inst << std::endl;
	std::cout << "Simulated instruction count: " << simulated_count << std::endl;
	std::cout << "Pipeline depth config (D): " << pipeline_depth << std::endl;
	std::cout << "Cycle count: " << cycle_count << std::endl;
	std::cout << std::fixed << std::setprecision(6)
			  << "Execution time (ms): " << exec_time_ms << std::endl;

	std::cout << std::setprecision(2);
	std::cout << "%int: " << (100.0 * static_cast<double>(histogram[INT_INST]) / total) << std::endl;
	std::cout << "%FP: " << (100.0 * static_cast<double>(histogram[FP_INST]) / total) << std::endl;
	std::cout << "%branch: " << (100.0 * static_cast<double>(histogram[BRANCH_INST]) / total) << std::endl;
	std::cout << "%load: " << (100.0 * static_cast<double>(histogram[LOAD_INST]) / total) << std::endl;
	std::cout << "%store: " << (100.0 * static_cast<double>(histogram[STORE_INST]) / total) << std::endl;
}

void Simulation::run_simulation()
{
	if (pipeline_depth < 1 || pipeline_depth > 4)
	{
		std::cerr << "Error: pipeline depth configuration D must be between 1 and 4." << std::endl;
		return;
	}

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

				if (next_instruction->instruction_type == BRANCH_INST)
				{
					cpu.set_branch_stall(true);
					break;
				}
			}
		}

		++cycle_count;

		if (cycle_count == std::numeric_limits<long long>::max())
		{
			std::cerr << "Error: cycle counter overflow." << std::endl;
			return;
		}
	}

	print_final_report(total);
}