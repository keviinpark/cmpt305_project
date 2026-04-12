#include "Pipeline.h"
#include <iostream>

int Pipeline::get_ex_cycles(InstructionType type) 
{
    if (type == FP_INST && (depth_config == 2 || depth_config == 4)) {
        return 2;
    }
    return 1; /* Default is 1 cycle per stage */
}

int Pipeline::get_mem_cycles(InstructionType type)
{
    if (type == LOAD_INST && (depth_config == 3 || depth_config == 4)) {
        return 3;
    }
    return 1; /* Default is 1 cycle per stage */
}

bool Pipeline::check_unit_avail(InstructionType instruction_type)
{
    switch (instruction_type)
    {
        case INT_INST: return !alu_busy;
        case FP_INST: return !fp_busy;
        case BRANCH_INST: return !branch_busy;
        case LOAD_INST: return !l1_read_busy;
        case STORE_INST: return !l1_write_busy;
        default: return true;
    }
}

void Pipeline::reserve_unit(InstructionType instruction_type)
{
    switch (instruction_type)
    {
        case INT_INST: alu_busy = true; break;
        case FP_INST: fp_busy = true; break;
        case BRANCH_INST: break;
        case LOAD_INST: break;
        case STORE_INST: break;
        default: break;
    }
}

bool Pipeline::is_stalled()
{
    return branch_stall;
}

void Pipeline::insert_instruction(Instruction* instruction)
{
    instruction->current_stage = NOT_STARTED;
    pipeline_stages[0].push_back(instruction);
}

bool Pipeline::is_done() 
{
    bool empty = true;
    for (int i = 0; i < 5; ++i)
    {
        if (!pipeline_stages[i].empty()) {
            empty = false;
        }
    }
    
    if (!pipeline_stages[0].empty()) {
        empty = false;
    }
    
    return empty;
}


void Pipeline::process_IF()
{

    if (branch_stall) return; /* Stop from moving currently fetched instructions from IF to ID if there is a branch */

    int moved = 0;

    while (!pipeline_stages[0].empty() && moved < 2)
    {
        Instruction* instruction = pipeline_stages[0].front();

        if (instruction->instruction_type == BRANCH_INST) {
            branch_stall = true;
            instruction->current_stage = IF_STAGE;
            pipeline_stages[1].push_back(instruction);
            pipeline_stages[0].pop_front();
            break; 
        }

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

        bool unit_available = check_unit_avail(instruction->instruction_type);

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
        if (instruction->current_stage != EX_STAGE)
        {
            instruction->current_stage = EX_STAGE;
            instruction->cycles_remaining = get_ex_cycles(instruction->instruction_type) - 1;
        }

        if (instruction->cycles_remaining > 0)
        {
            instruction->cycles_remaining--;
            // Stall until EX cycles are done
            break;
        }

        if (instruction->instruction_type == BRANCH_INST) {
            branch_stall = false;
        }
        else if (instruction->instruction_type == INT_INST) {
            alu_busy = false;
        } else if (instruction->instruction_type == FP_INST) {
            fp_busy = false;
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
        if (instruction->current_stage != MEM_STAGE)
        {
            // Structural hazard: max one load and one store entering MEM per cycle.
            if (instruction->instruction_type == LOAD_INST && read_port_used) break;
            if (instruction->instruction_type == STORE_INST && write_port_used) break;

            instruction->current_stage = MEM_STAGE;
            instruction->cycles_remaining = get_mem_cycles(instruction->instruction_type) - 1;
        }

        // Mark port as used whether this is a fresh arrival or a continuation
        if (instruction->instruction_type == LOAD_INST)  read_port_used  = true;
        if (instruction->instruction_type == STORE_INST) write_port_used = true;

        if (instruction->cycles_remaining > 0)
        {
            instruction->cycles_remaining--;
            // Stall until MEM cycles are done
            break;
        }

        pipeline_stages[4].push_back(instruction);
        pipeline_stages[3].pop_front();
        moved++;
    }
}

void Pipeline::process_WB(long long* instruction_type_count)
{
    int retired = 0;
    while (!pipeline_stages[4].empty() && retired < 2) {
        Instruction* instruction = pipeline_stages[4].front();
        instruction->current_stage = WB_STAGE; // Retired

        instruction_type_count[instruction->instruction_type]++;
        
        pipeline_stages[4].pop_front();
        retired++;
    }
}