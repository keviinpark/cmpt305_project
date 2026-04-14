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
    for (int i = 0; i < 5; ++i)
    {
        if (!pipeline_stages[i].empty()) {
            return false;
        }
    }
    return true;
}


void Pipeline::process_IF()
{

    if (branch_stall) return; /* Stop from moving currently fetched instructions from IF to ID if there is a branch */

    int moved = 0;
    while (!pipeline_stages[0].empty() && moved < 2)
    {
        Instruction* instruction = pipeline_stages[0].front();
        instruction->current_stage = IF_STAGE;
        pipeline_stages[1].push_back(instruction);
        pipeline_stages[0].pop_front();
        moved++;

        if (instruction->instruction_type == BRANCH_INST) {
            branch_stall = true;
            break;
        }
    }
}

void Pipeline::process_ID()
{
    bool int_used = false;
    bool fp_used = false;
    bool branch_used = false;
    int moved = 0;
    while (!pipeline_stages[1].empty() && moved < 2)
    {
        Instruction* instruction = pipeline_stages[1].front();
        instruction->current_stage = ID_STAGE;

        /* Check data hazards:
         * Use completed_stage (not current_stage) so that an instruction whose
         * dependency exits EX/MEM in the same cycle (via process_EX/process_MEM
         * which run before process_ID) is correctly seen as ready.
         */
        bool data_ready = true;
        for (Instruction* dependency : instruction->dependencies)
        {
            if (!dependency) continue;

            if (dependency->instruction_type == LOAD_INST || dependency->instruction_type == STORE_INST)
            {
                /* Satisfied after dep completes MEM stage */
                if (dependency->completed_stage < MEM_STAGE) {
                    data_ready = false;
                    break;
                }
            } else {
                /* Satisfied after dep completes EX stage */
                if (dependency->completed_stage < EX_STAGE) {
                    data_ready = false;
                    break;
                }
            }
        }
        bool unit_available = true;

        switch (instruction->instruction_type)
        {
            case INT_INST:
                unit_available = !int_used;
                break;
            case FP_INST:
                unit_available = !fp_used;
                break;
            case BRANCH_INST:
                unit_available = !branch_used;
                break;
            default:
                break;
        }

        if (data_ready && unit_available)
        {
            if (instruction->instruction_type == INT_INST) int_used = true;
            if (instruction->instruction_type == FP_INST) fp_used = true;
            if (instruction->instruction_type == BRANCH_INST) branch_used = true;

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
    bool int_used = false;
    bool fp_used = false;
    bool branch_used = false;

    int moved = 0;
    while (!pipeline_stages[2].empty() && moved < 2)
    {
        Instruction* instruction = pipeline_stages[2].front();
        if (instruction->current_stage != EX_STAGE)
        {
            /* Structural hazard: only one of each functional unit per cycle */
            if (instruction->instruction_type == INT_INST && int_used) break;
            if (instruction->instruction_type == FP_INST && fp_used) break;
            if (instruction->instruction_type == BRANCH_INST && branch_used) break;

            instruction->current_stage = EX_STAGE;
            instruction->cycles_remaining = get_ex_cycles(instruction->instruction_type) - 1;
        }

        /* Mark unit as used whether this is a new arrival or continuation */
        if (instruction->instruction_type == INT_INST) int_used = true;
        if (instruction->instruction_type == FP_INST) fp_used = true;
        if (instruction->instruction_type == BRANCH_INST) branch_used = true;

        if (instruction->cycles_remaining > 0)
        {
            instruction->cycles_remaining--;
            // Stall until EX cycles are done
            break;
        }

        if (instruction->instruction_type == BRANCH_INST) {
            clear_branch_stall = true;
        }

        /* Mark EX as completed so data-hazard checks in process_ID
         * (which runs after process_EX) see this instruction as done with EX. */
        instruction->completed_stage = EX_STAGE;
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

        // Mark port as used whether this is a new arrival or continuation (depth)
        if (instruction->instruction_type == LOAD_INST)  read_port_used  = true;
        if (instruction->instruction_type == STORE_INST) write_port_used = true;

        if (instruction->cycles_remaining > 0)
        {
            instruction->cycles_remaining--;
            // Stall until MEM cycles are done
            break;
        }

        /* Mark MEM as completed so data-hazard checks in process_ID
         * (which runs after process_MEM) see this instruction as done with MEM. */
        instruction->completed_stage = MEM_STAGE;
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