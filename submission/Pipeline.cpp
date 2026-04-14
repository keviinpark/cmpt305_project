#include "Pipeline.h"
#include <iostream>

int Pipeline::get_ex_cycles(InstructionType type)
{
    if (type == FP_INST && (depth_config == 2 || depth_config == 4))
    {
        return 2;
    }
    return 1;
}

int Pipeline::get_mem_cycles(InstructionType type)
{
    if (type == LOAD_INST && (depth_config == 3 || depth_config == 4))
    {
        return 3;
    }
    return 1;
}

bool Pipeline::is_done()
{
    for (int i = 0; i < 6; ++i)
    {
        if (!pipeline_stages[i].empty())
            return false;
    }
    return true;
}

void Pipeline::advance_pipeline(long long *instruction_type_count)
{
    process_WB(instruction_type_count);
    process_MEM();
    process_EX();
    process_ID();

    if (clear_branch_stall)
    {
        branch_stall = false;
        clear_branch_stall = false;
    }

    process_IF();
}

void Pipeline::process_WB(long long *instruction_type_count)
{
    // Retire instructions from WB stage
    while (!pipeline_stages[5].empty())
    {
        Instruction *instruction = pipeline_stages[5].front();
        instruction_type_count[instruction->instruction_type]++;
        pipeline_stages[5].pop_front();
    }

    // Pull max 2 instructions from MEM to WB if finished
    int moved = 0;
    while (!pipeline_stages[4].empty() && moved < 2 && pipeline_stages[5].size() < 2)
    {
        Instruction *instruction = pipeline_stages[4].front();
        if (instruction->mem_started && instruction->cycles_remaining == 0)
        {
            instruction->current_stage = WB_STAGE;
            pipeline_stages[5].push_back(instruction);
            pipeline_stages[4].pop_front();
            moved++;
        }
        else
            break;
    }
}

void Pipeline::process_MEM()
{
    // Update cycles remaining for instructions in MEM stage
    for (Instruction *instruction : pipeline_stages[4])
    {
        if (!instruction->mem_started)
        {
            instruction->mem_started = true;
            instruction->cycles_remaining = get_mem_cycles(instruction->instruction_type);
        }
        if (instruction->cycles_remaining > 0)
        {
            instruction->cycles_remaining--;
        }
    }

    // Check for structural hazards in MEM stage to determine if we can move instructions from EX to MEM
    bool read_port_busy = false;
    bool write_port_busy = false;
    for (Instruction *instruction : pipeline_stages[4])
    {
        if (instruction->cycles_remaining > 0)
        {
            if (instruction->instruction_type == LOAD_INST)
            {
                read_port_busy = true;
            }

            if (instruction->instruction_type == STORE_INST)
            {
                write_port_busy = true;
            }
        }
    }

    // Pull max 2 instructions from EX to MEM if finished and no structural hazards
    int moved = 0;
    while (!pipeline_stages[3].empty() && moved < 2 && pipeline_stages[4].size() < 2)
    {
        Instruction *instruction = pipeline_stages[3].front();
        if (instruction->ex_started && instruction->cycles_remaining == 0)
        {
            if (instruction->instruction_type == LOAD_INST && read_port_busy)
            {
                break;
            }
            if (instruction->instruction_type == STORE_INST && write_port_busy)
            {
                break;
            }

            if (instruction->instruction_type == LOAD_INST)
            {
                read_port_busy = true;
            }
            if (instruction->instruction_type == STORE_INST)
            {
                write_port_busy = true;
            }

            instruction->current_stage = MEM_STAGE;
            instruction->mem_started = false;
            pipeline_stages[4].push_back(instruction);
            pipeline_stages[3].pop_front();
            moved++;
        }
        else
        {
            break;
        }
    }
}

void Pipeline::process_EX()
{

    // Update cycles remaining for instructions in EX stage
    for (Instruction *instruction : pipeline_stages[3])
    {
        if (!instruction->ex_started)
        {
            instruction->ex_started = true;
            instruction->cycles_remaining = get_ex_cycles(instruction->instruction_type);
        }
        if (instruction->cycles_remaining > 0)
        {
            instruction->cycles_remaining--;
        }

        if (instruction->instruction_type == BRANCH_INST && instruction->cycles_remaining == 0)
        {
            clear_branch_stall = true;
        }
    }
}

void Pipeline::process_ID()
{
    // Check for hazards in EX stage to determine if we can move instructions from ID to EX
    bool alu_busy = false;
    bool fp_busy = false;
    bool branch_busy = false;
    for (Instruction *instruction : pipeline_stages[3])
    {
        if (instruction->cycles_remaining > 0)
        {
            if (instruction->instruction_type == INT_INST)
            {
                alu_busy = true;
            }
            if (instruction->instruction_type == FP_INST)
            {
                fp_busy = true;
            }
            if (instruction->instruction_type == BRANCH_INST)
            {
                branch_busy = true;
            }
        }
    }

    // Pull max 2 instructions from ID to EX if no hazards
    int moved = 0;
    while (!pipeline_stages[2].empty() && moved < 2 && pipeline_stages[3].size() < 2)
    {
        Instruction *instruction = pipeline_stages[2].front();

        bool data_ready = true;
        for (Instruction *dependency : instruction->dependencies)
        {
            if (!dependency)
            {
                continue;
            }
            if (dependency->current_stage < MEM_STAGE)
            {
                data_ready = false;
                break;
            }
        }
        if (!data_ready)
        {
            break;
        }

        bool unit_avail = true;
        if (instruction->instruction_type == INT_INST)
        {
            unit_avail = !alu_busy;
        }
        else if (instruction->instruction_type == FP_INST)
        {
            unit_avail = !fp_busy;
        }
        else if (instruction->instruction_type == BRANCH_INST)
        {
            unit_avail = !branch_busy;
        }

        if (unit_avail)
        {
            if (instruction->instruction_type == INT_INST)
            {
                alu_busy = true;
            }
            if (instruction->instruction_type == FP_INST)
            {
                fp_busy = true;
            }
            if (instruction->instruction_type == BRANCH_INST)
            {
                branch_busy = true;
            }

            instruction->current_stage = EX_STAGE;
            instruction->ex_started = false;
            pipeline_stages[3].push_back(instruction);
            pipeline_stages[2].pop_front();
            moved++;
        }
        else
        {
            break;
        }
    }

    // Pull max 2 instructions from IF to ID if available
    while (!pipeline_stages[1].empty() && pipeline_stages[2].size() < 2)
    {
        Instruction *instruction = pipeline_stages[1].front();
        instruction->current_stage = ID_STAGE;
        pipeline_stages[2].push_back(instruction);
        pipeline_stages[1].pop_front();
    }
}

void Pipeline::process_IF()
{
    if (branch_stall)
    {
        return;
    }

    // Pull max 2 instructions from IF to ID if available
    int moved = 0;
    while (!pipeline_stages[0].empty() && moved < 2 && pipeline_stages[1].size() < 2)
    {
        Instruction *instruction = pipeline_stages[0].front();
        instruction->current_stage = IF_STAGE;
        pipeline_stages[1].push_back(instruction);
        pipeline_stages[0].pop_front();
        moved++;

        if (instruction->instruction_type == BRANCH_INST)
        {
            branch_stall = true;
            break;
        }
    }
}

bool Pipeline::is_stalled()
{
    return branch_stall;
}

void Pipeline::insert_instruction(Instruction *instruction)
{
    instruction->current_stage = NOT_STARTED;
    instruction->ex_started = false;
    instruction->mem_started = false;
    pipeline_stages[0].push_back(instruction);
}