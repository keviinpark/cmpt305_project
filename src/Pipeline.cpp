#include "Pipeline.h"

bool Pipeline::check_unit_availability(int instruction_type) const
{
    switch (instruction_type)
    {
        case 1: return !alu_busy;
        case 2: return !fp_busy;
        case 3: return !branch_busy;
        case 4: return true;
        case 5: return true;
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
        case 4: break;
        case 5: break;
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

    instruction->current_stage = NOT_STARTED;
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
    int moved = 0;

    while (!pipeline_stages[0].empty() && moved < 2)
    {
        Instruction* instruction = pipeline_stages[0].front();
        instruction->current_stage = IF_STAGE;
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
        instruction->current_stage = ID_STAGE;

        /* Check data hazards */
        bool data_ready = true;
        for (Instruction* dependency : instruction->dependencies)
        {
            if (dependency->current_stage < MEM_STAGE)
            {
                data_ready = false;
                break;
            }
        }

        bool unit_available = check_unit_availability(instruction->instruction_type);

        if (data_ready && unit_available)
        {
            reserve_unit(instruction->instruction_type);

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
        instruction->current_stage = EX_STAGE;

        if (instruction->instruction_type == BRANCH_INST) {
            branch_stall = false;
        }

        pipeline_stages[3].push_back(instruction);
        pipeline_stages[2].pop_front();
        moved++;
    }
}

void Pipeline::process_MEM()
{
    bool read_port_used = false;
    bool write_port_used = false;

    int moved = 0;
    while (!pipeline_stages[3].empty() && moved < 2) {
        Instruction* instruction = pipeline_stages[3].front();
        instruction->current_stage = MEM_STAGE;

    // Structural hazard: max one load and one store entering MEM per cycle.
    if (instruction->instruction_type == LOAD_INST && read_port_used) break;
    if (instruction->instruction_type == STORE_INST && write_port_used) break;

    if (instruction->instruction_type == LOAD_INST) read_port_used = true;
    if (instruction->instruction_type == STORE_INST) write_port_used = true;

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
        instruction->current_stage = WB_STAGE; // Retired

        clear_unit_lock(instruction);
        
        pipeline_stages[4].pop_front();
        retired++;
    }
}
