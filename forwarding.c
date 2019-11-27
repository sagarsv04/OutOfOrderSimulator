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



void clear_stage_entry(APEX_CPU* cpu, int stage_index){
	// this is to clear any previous entries in stage and avoid conflicts between diff instructions
	cpu->stage[stage_index].rd = -9999;
	cpu->stage[stage_index].rd_valid = 0;;
	cpu->stage[stage_index].rs1 = -9999;
	cpu->stage[stage_index].rs1_valid = 0;
	cpu->stage[stage_index].rs2 = -9999;
	cpu->stage[stage_index].rs2_valid = 0;
	cpu->stage[stage_index].inst_type = -9999;
	cpu->stage[stage_index].pc = -9999;
}


void add_bubble_to_stage(APEX_CPU* cpu, int stage_index, int flushed) {
	// Add bubble to cpu stage
	// NOTE: don't call this function with stage_index F
	clear_stage_entry(cpu, stage_index);
	strcpy(cpu->stage[stage_index].opcode, "NOP"); // add a Bubble
	cpu->stage[stage_index].inst_type = NOP;
	cpu->code_memory_size = cpu->code_memory_size + 1;
	if (flushed){
		// this is used to tell a diff between Branch flushed and software delaying
		cpu->stage[stage_index].empty = 1;
	}
}


void push_stages(APEX_CPU* cpu) {

	if (!cpu->stage[F].stalled) {
		clear_stage_entry(cpu, DRF);
		cpu->stage[DRF] = cpu->stage[F];
		cpu->stage[DRF].executed = 0;
	}
	else {
		add_bubble_to_stage(cpu, DRF, 0);
	}
	// cpu->stage[WB].executed = 0;
	// cpu->stage[MEM_TWO] = cpu->stage[MEM_ONE];
	// cpu->stage[MEM_TWO].executed = 0;
	// cpu->stage[MEM_ONE] = cpu->stage[EX_TWO];
	// cpu->stage[MEM_ONE].executed = 0;
	// cpu->stage[EX_TWO] = cpu->stage[EX_ONE];
	// cpu->stage[EX_TWO].executed = 0;
	// if (!cpu->stage[DRF].stalled) {
	// 	cpu->stage[EX_ONE] = cpu->stage[DRF];
	// 	cpu->stage[EX_ONE].executed = 0;
	// }
	// else {
	// 	add_bubble_to_stage(cpu, EX_ONE, 0); // next cycle Bubble will be executed
	// 	cpu->stage[EX_ONE].executed = 0;
	// }
	// if (!cpu->stage[F].stalled) {
	// 	cpu->stage[DRF].rd = -99;
	// 	cpu->stage[DRF].rs1 = -99;
	// 	cpu->stage[DRF].rs2 = -99;
	// 	cpu->stage[DRF] = cpu->stage[F];
	// 	cpu->stage[DRF].executed = 0;
	// }
	// else if (!cpu->stage[DRF].stalled) {
	// 	add_bubble_to_stage(cpu, DRF, 0); // next cycle Bubble will be executed
	// 	cpu->stage[DRF].executed = 0;
	// }
	// if (ENABLE_PUSH_STAGE_PRINT) {
	// 	printf("\n--------------------------------\n");
	// 	printf("Clock Cycle #: %d Instructions Pushed\n", cpu->clock);
	// 	printf("%-15s: Executed: Instruction\n", "Stage");
	// 	printf("--------------------------------\n");
	// 	print_stage_content("Writeback", &cpu->stage[WB]);
	// 	print_stage_content("Memory Two", &cpu->stage[MEM_TWO]);
	// 	print_stage_content("Memory One", &cpu->stage[MEM_ONE]);
	// 	print_stage_content("Execute Two", &cpu->stage[EX_TWO]);
	// 	print_stage_content("Execute One", &cpu->stage[EX_ONE]);
	// 	print_stage_content("Decode/RF", &cpu->stage[DRF]);
	// 	print_stage_content("Fetch", &cpu->stage[F]);
	// }
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
