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



void get_inst_name(int inst_type, char* inst_type_str) {

	switch (inst_type) {

		case STORE:
			strcpy(inst_type_str, "STORE");
			break;
		case STR:
			strcpy(inst_type_str, "STR");
			break;
		case LOAD:
			strcpy(inst_type_str, "LOAD");
			break;
		case LDR:
			strcpy(inst_type_str, "LDR");
			break;
		case MOVC:
			strcpy(inst_type_str, "MOVC");
			break;
		case MOV:
			strcpy(inst_type_str, "MOV");
			break;
		case ADD:
			strcpy(inst_type_str, "ADD");
			break;
		case ADDL:
			strcpy(inst_type_str, "ADDL");
			break;
		case SUB:
			strcpy(inst_type_str, "SUB");
			break;
		case SUBL:
			strcpy(inst_type_str, "SUBL");
			break;
		case MUL:
			strcpy(inst_type_str, "MUL");
			break;
		case DIV:
			strcpy(inst_type_str, "DIV");
			break;
		case AND:
			strcpy(inst_type_str, "AND");
			break;
		case OR:
			strcpy(inst_type_str, "OR");
			break;
		case EXOR:
			strcpy(inst_type_str, "EX-OR");
			break;
		case BZ:
			strcpy(inst_type_str, "BZ");
			break;
		case BNZ:
			strcpy(inst_type_str, "BNZ");
			break;
		case JUMP:
			strcpy(inst_type_str, "JUMP");
			break;
		case HALT:
			strcpy(inst_type_str, "HALT");
			break;
		case NOP:
			strcpy(inst_type_str, "NOP");
			break;
		default:
			strcpy(inst_type_str, "INVALID");
			break;
	}
}


void clear_stage_entry(APEX_CPU* cpu, int stage_index){
	// this is to clear any previous entries in stage and avoid conflicts between diff instructions
	cpu->stage[stage_index].rd = INVALID;
	cpu->stage[stage_index].rd_valid = INVALID;;
	cpu->stage[stage_index].rs1 = INVALID;
	cpu->stage[stage_index].rs1_valid = 0;
	cpu->stage[stage_index].rs2 = INVALID;
	cpu->stage[stage_index].rs2_valid = 0;
	cpu->stage[stage_index].inst_type = INVALID;
	cpu->stage[stage_index].pc = -1;
	cpu->stage[stage_index].empty = 1;
	cpu->stage[stage_index].stage_cycle = INVALID;
	strcpy(cpu->stage[stage_index].opcode, "");
}


void add_bubble_to_stage(APEX_CPU* cpu, int stage_index) {
	// Add bubble to cpu stage
	clear_stage_entry(cpu, stage_index);
	strcpy(cpu->stage[stage_index].opcode, "NOP"); // add a Bubble
	cpu->stage[stage_index].inst_type = NOP;
	cpu->code_memory_size = cpu->code_memory_size + 1;
	cpu->stage[stage_index].empty = 0;
}


void push_func_unit_stages(APEX_CPU* cpu, int after_iq){

	if (after_iq) {
		cpu->stage[MUL_THREE] = cpu->stage[MUL_TWO];
		cpu->stage[MUL_TWO] = cpu->stage[MUL_ONE];
		// and empty the MUL_ONE stage
		clear_stage_entry(cpu, MUL_ONE);

		cpu->stage[INT_TWO] = cpu->stage[INT_ONE];
		// and empty the MUL_ONE stage
		clear_stage_entry(cpu, INT_ONE);

		if ((cpu->stage[MEM].executed)&&(cpu->stage[MEM].stage_cycle>=3)) {
			// and empty the MEM stage
			clear_stage_entry(cpu, MEM);
		}

	}
	else {
		if (!cpu->stage[F].stalled) {
			clear_stage_entry(cpu, DRF);
			cpu->stage[DRF] = cpu->stage[F];
			cpu->stage[DRF].executed = 0;
		}
		else {
			add_bubble_to_stage(cpu, DRF);
		}
	}
}


int get_reg_values(APEX_CPU* cpu, int src_reg) {
	// Get Reg values function
	int value = -1;
	value = cpu->regs[src_reg];
	return value;
}


int get_reg_status(APEX_CPU* cpu, int reg_number) {
	// Get Reg Status function
	int status = 1; // 1 is invalid and 0 is valid
	if (reg_number > REGISTER_FILE_SIZE) {
		// Segmentation fault
		fprintf(stderr, "Segmentation fault for Register location :: %d\n", reg_number);
	}
	else {
		status = cpu->regs_invalid[reg_number];
	}
	return status;
}


void set_reg_status(APEX_CPU* cpu, int reg_number, int status) {
	// Set Reg Status function
	// NOTE: insted of set inc or dec regs_invalid
	if (reg_number > REGISTER_FILE_SIZE) {
		// Segmentation fault
		fprintf(stderr, "Segmentation fault for Register location :: %d\n", reg_number);
	}
	else {
		cpu->regs_invalid[reg_number] = cpu->regs_invalid[reg_number] + status;
	}
}


int previous_arithmetic_check(APEX_CPU* cpu, int func_unit) {

	int status = 0;
	int a = 0;
	for (int i=a;i<func_unit; i++) {
		if (strcmp(cpu->stage[i].opcode, "NOP") != 0) {
			a = i;
			break;
		}
	}

	if (a!=0){
		if ((strcmp(cpu->stage[a].opcode, "ADD") == 0) ||
			(strcmp(cpu->stage[a].opcode, "ADDL") == 0) ||
			(strcmp(cpu->stage[a].opcode, "SUB") == 0) ||
			(strcmp(cpu->stage[a].opcode, "SUBL") == 0) ||
			(strcmp(cpu->stage[a].opcode, "MUL") == 0) || (strcmp(cpu->stage[a].opcode, "DIV") == 0)) {

			status = 1;
		}
	}

	return status;
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
