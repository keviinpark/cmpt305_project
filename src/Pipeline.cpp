#include "Pipeline.h"

bool Pipeline::check_unit_availability(int instruction_type) const
{
    switch (instruction_type)
    {
        case 1: return !alu_busy;
        case 2: return !fp_busy;
        case 3: return !branch_busy;
        case 4: return !l1_read_busy;
        case 5: return !l1_write_busy;
        default: return true;
    }
}

void Pipeline::reserve_unit(int instruction_type)
{
    switch (instruction_type)
    {
        case 1: alu_busy = true; break;
        case 2: fp_busy = true; break;
        case 3: branch_busy = true; break;
        case 4: l1_read_busy = true; break;
        case 5: l1_write_busy = true; break;
        default: break;
    }
}

void Pipeline::clear_unit_lock(const Instruction* instruction)
{
    if (instruction == nullptr)
    {
        return;
    }

    switch (instruction->instruction_type)
    {
        case 1: alu_busy = false; break;
        case 2: fp_busy = false; break;
        case 3: branch_busy = false; break;
        case 4: l1_read_busy = false; break;
        case 5: l1_write_busy = false; break;
        default: break;
    }
}

bool Pipeline::is_stalled() const
{
    return branch_stall;
}

void Pipeline::set_branch_stall(bool stall)
{
    branch_stall = stall;
}

void Pipeline::insert_instruction(Instruction* instruction)
{
    if (instruction == nullptr)
    {
        return;
    }

    instruction->current_stage = 0;
    pipeline_stages[0].push_back(instruction);
}

bool Pipeline::is_done() const
{
    for (const auto& stage : pipeline_stages)
    {
        if (!stage.empty())
        {
            return false;
        }
    }
    return true;
}


void Pipeline::process_IF()
{
    if (branch_stall)
    {
        return;
    }

    int moved = 0;

    while (!pipeline_stages[0].empty() && moved < 2)
    {
        Instruction* instruction = pipeline_stages[0].front();
        instruction->current_stage = 1; // Move to ID stage
        pipeline_stages[1].push_back(instruction);
        pipeline_stages[0].pop_front();
        moved++;
    }
    
}

void Pipeline::process_ID()
{
    int moved = 0;
    while (!pipeline_stages[1].empty() && moved < 2)
    {
        Instruction* instruction = pipeline_stages[1].front();

        bool data_ready = true;
        for (Instruction* dependency : instruction->dependencies)
        {
            if (dependency->current_stage < 4)
            {
                data_ready = false;
                break;
            }
        }

        bool unit_available = check_unit_availability(instruction->instruction_type);

        if (data_ready && unit_available)
        {
            reserve_unit(instruction->instruction_type);

            instruction->current_stage = 2; // Move to EX stage
            pipeline_stages[2].push_back(instruction);
            pipeline_stages[1].pop_front();
            moved++;
        }
        else
        {
            /* Stall */
            break;
        }
    }
}

void Pipeline::process_EX()
{
    int moved = 0;
    while (!pipeline_stages[2].empty() && moved < 2)
    {
        Instruction* instruction = pipeline_stages[2].front();

        // Resolve branch and release fetch stall after EX.
        if (instruction->instruction_type == 3) {
            branch_stall = false;
        }

        instruction->current_stage = 3;
        pipeline_stages[3].push_back(instruction);
        pipeline_stages[2].pop_front();
        moved++;
    }
}

void Pipeline::process_MEM()
{
    int moved = 0;
    while (!pipeline_stages[3].empty() && moved < 2) {
        Instruction* instruction = pipeline_stages[3].front();

        // Structural Hazard: Only 1 Read Port and 1 Write Port
        if (instruction->instruction_type == 4 && l1_read_busy) break;
        if (instruction->instruction_type == 5 && l1_write_busy) break;

        // Lock ports
        if (instruction->instruction_type == 4) l1_read_busy = true;
        if (instruction->instruction_type == 5) l1_write_busy = true;

        // Move to WB
        instruction->current_stage = 4;
        pipeline_stages[4].push_back(instruction);
        pipeline_stages[3].pop_front();
        moved++;
    }
}

void Pipeline::process_WB()
{
    int retired = 0;
    while (!pipeline_stages[4].empty() && retired < 2) {
        Instruction* instruction = pipeline_stages[4].front();

        // Free up structural units (if they weren't freed in EX/MEM)
        // Note: In this simple model, we clear busy flags here 
        // to allow new instructions to enter EX in the NEXT cycle.
        clear_unit_lock(instruction);

        instruction->current_stage = 5; // Retired
        pipeline_stages[4].pop_front();
        retired++;
    }
}
