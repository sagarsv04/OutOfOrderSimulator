/*
 *  Contains APEX Forwarding implementation
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "forwarding.h"



void add_bubble_to_stage(APEX_CPU* cpu, int stage_index, int flushed) {
	// Add bubble to cpu stage
	 if (flushed){
			 strcpy(cpu->stage[stage_index].opcode, "NOP"); // add a Bubble
			 cpu->code_memory_size = cpu->code_memory_size + 1;
			 cpu->stage[stage_index].empty = 1;
			 // this is because while checking for forwarding we dont look if EX_TWO or MEM_TWO has NOP
			 // we simply compare rd value to know if this register is wat we are looking for
			 // assuming no source register will be negative
			 cpu->stage[stage_index].rd = -99;
	 }
	if ((stage_index > F) && (stage_index < NUM_STAGES) && !(flushed)) {
		// No adding Bubble in Fetch and WB stage
		if (cpu->stage[stage_index].executed) {
			strcpy(cpu->stage[stage_index].opcode, "NOP"); // add a Bubble
			cpu->code_memory_size = cpu->code_memory_size + 1;
			// this is because while checking for forwarding we dont look if EX_TWO or MEM_TWO has NOP
			// we simply compare rd value to know if this register is wat we are looking for
			// assuming no source register will be negative
			cpu->stage[stage_index].rd = -99;
		}
		else {
			; // Nothing let it execute its current instruction
		}
	}
	else {
		;
	}
}


APEX_Forward get_cpu_forwarding_status(APEX_CPU* cpu, CPU_Stage* stage) {
	// depending on instruction check if forwarding can happen
	APEX_Forward forwarding;
	forwarding.status = 0;
	forwarding.unstall = 0;
	forwarding.rd_from = -1;
	forwarding.rs1_from = -1;
	forwarding.rs2_from = -1;
	// check rd value is computed or not
	// if((cpu->stage[EX_TWO].executed) || (cpu->stage[MEM_TWO].executed)) {
	// 	if ((strcmp(stage->opcode, "BZ") != 0)&&(strcmp(stage->opcode, "BNZ") != 0)) {
	// 		// check current rs1 with EX_TWO rd
	// 		if ((stage->inst_type != HALT) && (stage->inst_type != NOP) && (stage->inst_type != 0) {
	// 				// firts check forwarding from EX_TWO, also check if docode rs1 is also not presend in EX_ONE
	// 				if ((stage->rs1 == cpu->stage[EX_TWO].rd)&&(cpu->stage[EX_TWO].executed)&&(stage->rs1 != cpu->stage[EX_ONE].rd)){
	// 					// forwarding can be done
	// 					if (!(strcmp(cpu->stage[EX_TWO].opcode, "LOAD") == 0)) {
	// 						// dont forward as load rd values only gets filled by memory_two state
	// 						forwarding.rs1_from = EX_TWO;
	// 					}
	// 				}
	// 				// also check if docode rs1 is also not presend in MEM_ONE
	// 				else if ((stage->rs1 == cpu->stage[MEM_ONE].rd)&&(cpu->stage[MEM_ONE].executed)&&(stage->rs1 != cpu->stage[EX_TWO].rd)) {
	// 					// forwarding can be done
	// 					forwarding.rs1_from = MEM_ONE;
	// 				}
	// 				// also check if docode rs1 is also not presend in MEM_TWO
	// 				else if ((stage->rs1 == cpu->stage[MEM_TWO].rd)&&(cpu->stage[MEM_TWO].executed)&&(stage->rs1 != cpu->stage[MEM_ONE].rd)) {
	// 					// forwarding can be done
	// 					forwarding.rs1_from = MEM_TWO;
	// 				}
	// 				// cahnge status of forwarding
	// 				if (forwarding.rs1_from > 0) {
	// 					forwarding.status = 1;
	// 				}
	// 				if (!get_reg_status(cpu, stage->rs1)) {
	// 					forwarding.unstall = 1;
	// 				}
	// 		}
	// 		if ((stage->inst_type == STR) || (stage->inst_type == LDR) ||
	// 			(stage->inst_type == ADD) || (stage->inst_type == SUB) ||
	// 			(stage->inst_type == MUL) || (stage->inst_type == DIV) ||
	// 			(stage->inst_type == AND) || (stage->inst_type == OR) || (stage->inst_type == EX-OR)) {
	// 			// firts check forwarding from EX_TWO
	// 			// also check if docode rs2 is also not presend in EX_ONE
	// 			if ((stage->rs2 == cpu->stage[EX_TWO].rd)&&(cpu->stage[EX_TWO].executed)&&(stage->rs2 != cpu->stage[EX_ONE].rd)) {
	// 				// forwarding can be done
	// 				forwarding.rs2_from = EX_TWO;
	// 			}
	// 			// also check if docode rs1 is also not presend in MEM_ONE
	// 			else if ((stage->rs2 == cpu->stage[MEM_ONE].rd)&&(cpu->stage[MEM_ONE].executed)&&(stage->rs2 != cpu->stage[EX_TWO].rd)) {
	// 				// forwarding can be done
	// 				forwarding.rs2_from = MEM_ONE;
	// 			}
	// 			// also check if docode rs2 is also not presend in MEM_TWO
	// 			else if ((stage->rs2 == cpu->stage[MEM_TWO].rd)&&(cpu->stage[MEM_TWO].executed)&&(stage->rs2 != cpu->stage[MEM_ONE].rd)) {
	// 				// forwarding can be done
	// 				forwarding.rs2_from = MEM_TWO;
	// 			}
	//
	// 			if ((forwarding.rs1_from > 0)&&(forwarding.rs2_from > 0)) {
	// 				forwarding.status = 1;
	// 			}
	// 			else {
	// 				forwarding.status = 0;
	// 			}
	//
	// 			if ((!get_reg_status(cpu, stage->rs1)) || (!get_reg_status(cpu, stage->rs2))) {
	// 				forwarding.unstall = 1;
	// 			}
	// 			else {
	// 				forwarding.unstall = 0;
	// 			}
	// 		}
	// 		if ((stage->inst_type == STORE) || (stage->inst_type == STR)) {
	// 			// firts check forwarding from EX_TWO
	// 			// also check if docode rd is also not presend in EX_ONE
	// 			if ((stage->rd == cpu->stage[EX_TWO].rd)&&(cpu->stage[EX_TWO].executed)&&(stage->rd != cpu->stage[EX_ONE].rd)) {
	// 				// forwarding can be done
	// 				forwarding.rd_from = EX_TWO;
	// 			}
	// 			// also check if docode rs1 is also not presend in MEM_ONE
	// 			else if ((stage->rs2 == cpu->stage[MEM_ONE].rd)&&(cpu->stage[MEM_ONE].executed)&&(stage->rs2 != cpu->stage[EX_TWO].rd)) {
	// 				// forwarding can be done
	// 				forwarding.rs2_from = MEM_ONE;
	// 			}
	// 			// also check if docode rd is also not presend in MEM_TWO
	// 			else if ((stage->rd == cpu->stage[MEM_TWO].rd)&&(cpu->stage[MEM_TWO].executed)&&(stage->rd != cpu->stage[MEM_ONE].rd)) {
	// 				// forwarding can be done
	// 				forwarding.rd_from = MEM_TWO;
	// 			}
	//
	// 			if ((forwarding.rd_from > 0)&&(forwarding.rs1_from > 0)&&(forwarding.rs2_from > 0)) {
	// 				forwarding.status = 1;
	// 			}
	// 			else {
	// 				forwarding.status = 0;
	// 			}
	// 			if ((!get_reg_status(cpu, stage->rd)) || (!get_reg_status(cpu, stage->rs1)) || (!get_reg_status(cpu, stage->rs2))) {
	// 				forwarding.unstall = 1;
	// 			}
	// 			else {
	// 				forwarding.unstall = 0;
	// 			}
	// 		}
	// 	}
	// 	else {
	// 		if (!((strcmp(cpu->stage[EX_ONE].opcode, "ADD") == 0) ||
	// 				(strcmp(cpu->stage[EX_ONE].opcode, "ADDL") == 0) ||
	// 				(strcmp(cpu->stage[EX_ONE].opcode, "SUB") == 0) ||
	// 				(strcmp(cpu->stage[EX_ONE].opcode, "SUBL") == 0) ||
	// 				(strcmp(cpu->stage[EX_ONE].opcode, "MUL") == 0) ||
	// 				(strcmp(cpu->stage[EX_TWO].opcode, "DIV") == 0))
	// 				&&
	// 				((strcmp(cpu->stage[EX_TWO].opcode, "ADD") == 0) ||
	// 				(strcmp(cpu->stage[EX_TWO].opcode, "ADDL") == 0) ||
	// 				(strcmp(cpu->stage[EX_TWO].opcode, "SUB") == 0) ||
	// 				(strcmp(cpu->stage[EX_TWO].opcode, "SUBL") == 0) ||
	// 				(strcmp(cpu->stage[EX_TWO].opcode, "MUL") == 0) ||
	// 				(strcmp(cpu->stage[EX_TWO].opcode, "DIV") == 0))) {
	//
	// 			if (cpu->stage[EX_TWO].executed) {
	// 				// forwarding can be done
	// 				forwarding.status = 1;
	// 			}
	// 		}
	// 	}
	// }
	return forwarding;
}
