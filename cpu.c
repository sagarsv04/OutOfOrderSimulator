/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "cpu.h"

/* Set this flag to 1 to enable debug messages */
#define ENABLE_DEBUG_MESSAGES 1

/* Set this flag to 1 to enable print of Regs, Flags, Memory */
#define ENABLE_REG_MEM_STATUS_PRINT 1
#define ENABLE_PUSH_STAGE_PRINT 0

/*
 * ########################################## Initialize CPU ##########################################
 */

APEX_CPU* APEX_cpu_init(const char* filename) {
	// This function creates and initializes APEX cpu.
	if (!filename) {
		return NULL;
	}
	// memory allocation of struct APEX_CPU to struct pointer cpu
	APEX_CPU* cpu = malloc(sizeof(*cpu));
	if (!cpu) {
		return NULL;
	}

	/* Initialize PC, Registers and all pipeline stages */
	cpu->pc = 4000;
	memset(cpu->regs, 0, sizeof(int) * REGISTER_FILE_SIZE);  // fill a block of memory with a particular value here value is 0 for 32 regs with size 4 Bytes
	memset(cpu->regs_invalid, 0, sizeof(int) * REGISTER_FILE_SIZE);  // all registers are valid at start, set to value 1
	memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES); // all values in stage struct of type CPU_Stage like pc, rs1, etc are set to 0
	memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE); // from 4000 to 4095 there will be garbage values in data_memory array
	memset(cpu->flags, 0, sizeof(int) * NUM_FLAG); // all flag values in cpu are set to 0

	memset(cpu->issue_queue, 0, sizeof(APEX_IQ) * IQ_SIZE); // all issue queue values in cpu are set to 0
	memset(cpu->load_store_queue, 0, sizeof(APEX_LSQ) * LSQ_SIZE); // all load store queue values in cpu are set to 0
	memset(cpu->reorder_buffer.rob_entry, 0, sizeof(APEX_ROB_ENTRY) * ROB_SIZE); // all rob values in cpu are set to 0

	/* Parse input file and create code memory */
	cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

	if (!cpu->code_memory) {
		free(cpu); // If code_memory is not created free the memory for cpu struct
		return NULL;
	}
	// Below code just prints the instructions and operands before execution
	if (ENABLE_DEBUG_MESSAGES) {
		fprintf(stderr,"APEX_CPU : Initialized APEX CPU, loaded %d instructions\n", cpu->code_memory_size);
		fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
		printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode", "rd", "rs1", "rs2", "imm");

		for (int i = 0; i < cpu->code_memory_size; ++i) {
			printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode, cpu->code_memory[i].rd, cpu->code_memory[i].rs1, cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
		}
	}

	/* Make all stages busy except Fetch stage, initally to start the pipeline */
	for (int i = 1; i < NUM_STAGES; ++i) {
		cpu->stage[i].empty = 1;
	}

	return cpu;
}

/*
 * ########################################## Stop CPU ##########################################
 */

void APEX_cpu_stop(APEX_CPU* cpu) {
	// This function de-allocates APEX cpu.
	free(cpu->code_memory);
	free(cpu);
}

int get_code_index(int pc) {
	// Converts the PC(4000 series) into array index for code memory
	// First instruction index is 0
	return (pc - 4000) / 4;
}

static void print_instruction(CPU_Stage* stage) {
	// This function prints operands of instructions in stages.
	switch (stage->inst_type) {

		case STORE: case LOAD: case ADDL: case SUBL:
			printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
			break;

		case STR: case LDR: case ADD: case SUB:case MUL: case DIV: case AND: case OR: case EXOR:
			printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
			break;

		case MOVC: case JUMP:
			printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
			break;

		case MOV:
			printf("%s,R%d,R%d ", stage->opcode, stage->rd, stage->rs1);
			break;

		case BZ: case BNZ:
			printf("%s,#%d ", stage->opcode, stage->imm);
			break;

		case HALT: case NOP:
			printf("%s ", stage->opcode);
			break;

		default:
			printf("<<- %s ->>", stage->opcode);
			break;
	}
}

static void print_stage_status(CPU_Stage* stage) {
	// This function prints status of stages.
	if (stage->empty) {
		printf(" ---> EMPTY ");
	}
	else if (stage->stalled) {
		printf(" ---> STALLED ");
	}
}

static void print_stage_content(char* name, CPU_Stage* stage) {
	// Print function which prints contents of stage
	printf("%-15s: %d: pc(%d) ", name, stage->executed, stage->pc);
	print_instruction(stage);
	print_stage_status(stage);
	printf("\n");
}

void print_cpu_content(APEX_CPU* cpu) {
	// Print function which prints contents of cpu memory
	if (ENABLE_REG_MEM_STATUS_PRINT) {
		printf("============ STATE OF CPU FLAGS ============\n");
		// print all Flags
		printf("Falgs::  ZeroFlag, CarryFlag, OverflowFlag, InterruptFlag\n");
		printf("Values:: %d,\t|\t%d,\t|\t%d,\t|\t%d\n", cpu->flags[ZF],cpu->flags[CF],cpu->flags[OF],cpu->flags[IF]);

		// print all regs along with valid bits
		printf("============ STATE OF ARCHITECTURAL REGISTER FILE ============\n");
		printf("NOTE :: 0 Means Valid & 1 Means Invalid\n");
		printf("Registers, Values, Invalid\n");
		for (int i=0;i<REGISTER_FILE_SIZE;i++) {
			printf("R%02d,\t|\t%02d,\t|\t%d\n", i, cpu->regs[i], cpu->regs_invalid[i]);
		}

		// print 100 memory location
		printf("============ STATE OF DATA MEMORY ============\n");
		printf("Mem Location, Values\n");
		for (int i=0;i<100;i++) {
			printf("M%02d,\t|\t%02d\n", i, cpu->data_memory[i]);
		}
		printf("\n");
	}
}

static int get_reg_values(APEX_CPU* cpu, CPU_Stage* stage, int src_reg_pos, int src_reg) {
	// Get Reg values function
	int value = 0;
	if (src_reg_pos == 0) {
		value = cpu->regs[src_reg];
	}
	else if (src_reg_pos == 1) {
		value = cpu->regs[src_reg];
	}
	else if (src_reg_pos == 2) {
		value = cpu->regs[src_reg];
	}
	else {
		;// Nothing
	}
	return value;
}

static int get_reg_status(APEX_CPU* cpu, int reg_number) {
	// Get Reg Status function
	int status = 1; // 1 is invalid
	if (reg_number > REGISTER_FILE_SIZE) {
		// Segmentation fault
		fprintf(stderr, "Segmentation fault for Register location :: %d\n", reg_number);
	}
	else {
		status = cpu->regs_invalid[reg_number];
	}
	return status;
}

static void set_reg_status(APEX_CPU* cpu, int reg_number, int status) {
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

static void add_bubble_to_stage(APEX_CPU* cpu, int stage_index, int flushed) {
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

/*
 * ########################################## forwarding Check ##########################################
 */
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

/*
 * ########################################## Fetch Stage ##########################################
 */
int fetch(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[F];
	stage->executed = 0;

	if (!stage->stalled) {
		/* Store current PC in fetch latch */
		stage->pc = cpu->pc;

		/* Index into code memory using this pc and copy all instruction fields into fetch latch */
		APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
		strcpy(stage->opcode, current_ins->opcode);
		stage->rd = current_ins->rd;
		stage->rs1 = current_ins->rs1;
		stage->rs2 = current_ins->rs2;
		stage->imm = current_ins->imm;
		stage->inst_type = current_ins->type;

		/* Copy data from Fetch latch to Decode latch*/
		stage->executed = 1;
		// cpu->stage[DRF] = cpu->stage[F]; // this is cool I should empty the fetch stage as well to avoid repetition ?
		// cpu->stage[DRF].executed = 0;
		if (stage->inst_type == 0) {
			// stop fetching Instructions, exit from writeback stage
			stage->stalled = 0;
			stage->empty = 1;
		}
		else {
			/* Update PC for next instruction */
			cpu->pc += 4;
			stage->empty = 0;
		}
	}
	else {
		//If Fetch has HALT and Decode has HALT fetch only one Inst
		if (cpu->stage[DRF].inst_type == HALT) {
			// just fetch the next instruction
			stage->pc = cpu->pc;
			APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
			strcpy(stage->opcode, current_ins->opcode);
			stage->rd = current_ins->rd;
			stage->rs1 = current_ins->rs1;
			stage->rs2 = current_ins->rs2;
			stage->imm = current_ins->imm;
			stage->inst_type = current_ins->type;
		}
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Fetch", stage);
	}

	return 0;
}

/*
 * ########################################## Decode Stage ##########################################
 */
int decode(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[DRF];
	stage->executed = 0;
	// decode stage only has power to stall itself and Fetch stage
	APEX_Forward forwarding = get_cpu_forwarding_status(cpu, stage);
	if ((!stage->stalled)||(forwarding.unstall)) {
		/* Read data from register file for store */
		switch(stage->inst_type) {

			case STORE:  // ************************************* STORE ************************************* //
				if (!get_reg_status(cpu, stage->rd) && !get_reg_status(cpu, stage->rs1)) {
					// read literal and register values
					stage->rd_value = get_reg_values(cpu, stage, 0, stage->rd);
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
				}
				else if (forwarding.status) {
					// take the value
					stage->rd_value = cpu->stage[forwarding.rd_from].rd_value;
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rd)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rd_value = get_reg_values(cpu, stage, 0, stage->rd);
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rd_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rd_value = cpu->stage[forwarding.rd_from].rd_value;
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
				// keep DF and Fetch Stage in stall if regs_invalid is set
				cpu->stage[DRF].stalled = 1;
				cpu->stage[F].stalled = 1;
				}
				break;

			case STR:  // ************************************* STR ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rd) && !get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rd_value = get_reg_values(cpu, stage, 0, stage->rd);
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1); // Here rd becomes src1 and src2, src3 are rs1, rs2
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rd_value = cpu->stage[forwarding.rd_from].rd_value;
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rd_from>=0) && !get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rd_value = cpu->stage[forwarding.rd_from].rd_value;
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rd) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rd_value = get_reg_values(cpu, stage, 0, stage->rd);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rd) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					stage->rd_value = get_reg_values(cpu, stage, 0, stage->rd);
					stage->rs1_value = get_reg_values(cpu, stage, 2, stage->rs1);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case LOAD:  // ************************************* LOAD ************************************* //
				// read literal and register values
				if (!get_reg_status(cpu, stage->rs1)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case LDR:  // ************************************* LDR ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case MOVC:  // ************************************* MOVC ************************************* //
				// read literal values
				stage->buffer = stage->imm; // keeping literal value in buffer to load in mem stage
				break;

			case MOV:  // ************************************* MOV ************************************* //
				// read register values
				if (!get_reg_status(cpu, stage->rs1)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case ADD:  // ************************************* ADD ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				 else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case ADDL:  // ************************************* ADDL ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->buffer = stage->imm; // keeping literal value in buffer to add in exe stage
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case SUB:   // ************************************* SUB ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case SUBL:  // ************************************* SUBL ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->buffer = stage->imm; // keeping literal value in buffer to sub in exe stage
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case MUL:  // ************************************* MUL ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case DIV:  // ************************************* DIV ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case AND:  // ************************************* AND ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case OR:  // ************************************* OR ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case EXOR:  // ************************************* EX-OR ************************************* //
				// read only values of last two registers
				if (!get_reg_status(cpu, stage->rs1) && !get_reg_status(cpu, stage->rs2)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs2_from>=0) && !get_reg_status(cpu, stage->rs1)) {
					// take the value
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->rs2_value = cpu->stage[forwarding.rs2_from].rd_value;
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else if ((forwarding.rs1_from>=0) && !get_reg_status(cpu, stage->rs2)) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->rs2_value = get_reg_values(cpu, stage, 2, stage->rs2);
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case BZ:  // ************************************* BZ ************************************* //
				// read literal values
				stage->buffer = stage->imm; // keeping literal value in buffer to jump in exe stage
				if ((forwarding.status)&&(!previous_arithmetic_check(cpu, WB))) {
					// if previous was arithmatic instruction than stall
					// keep DF and Fetch Stage in stall if regs_invalid is set
					stage->rd_value = cpu->stage[WB].rd_value;
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case BNZ:  // ************************************* BNZ ************************************* //
				// read literal values
				stage->buffer = stage->imm; // keeping literal value in buffer to jump in exe stage
				if ((forwarding.status)&&(!previous_arithmetic_check(cpu, WB))) {
					// if previous was arithmatic instruction than stall
					// keep DF and Fetch Stage in stall if regs_invalid is set
					stage->rd_value = cpu->stage[WB].rd_value;
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case JUMP:   // ************************************* JUMP ************************************* //
				// read literal and register values
				if (!get_reg_status(cpu, stage->rs1)) {
					stage->rs1_value = get_reg_values(cpu, stage, 1, stage->rs1);
					stage->buffer = stage->imm; // keeping literal value in buffer to cal memory to jump in exe stage
				}
				else if (forwarding.status) {
					// take the value
					stage->rs1_value = cpu->stage[forwarding.rs1_from].rd_value;
					stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
					// Un Stall DF and Fetch Stage
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
				}
				else {
					// keep DF and Fetch Stage in stall if regs_invalid is set
					cpu->stage[DRF].stalled = 1;
					cpu->stage[F].stalled = 1;
				}
				break;

			case HALT:  // ************************************* HALT ************************************* //
				// Halt causes a type of Intrupt where Fetch is stalled and cpu intrupt Bit is Set
				// Stop fetching new instruction but allow all the instruction to go from Decode Writeback
				cpu->stage[F].stalled = 1; // add NOP from fetch stage
				cpu->flags[IF] = 1; // Halt as Interrupt
				break;

			case NOP:  // ************************************* NOP ************************************* //
				break; // Nothing

			default:
				if (strcmp(stage->opcode, "") != 0) {
					fprintf(stderr, "Decode/RF Invalid Instruction Found :: %s\n", stage->opcode);
				}
				break;
		}

		stage->executed = 1;
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Decode/RF", stage);
	}

	return 0;
}

/*
 * ########################################## EX One Stage ##########################################
 */
int int_one_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[INT_ONE];
	stage->executed = 0;
	if (!stage->stalled) {

		/* Store */
		if (strcmp(stage->opcode, "STORE") == 0) {
			// create memory address using literal and register values
			stage->mem_address = stage->rs1_value + stage->buffer;
		}
		else if (strcmp(stage->opcode, "STR") == 0) {
			// create memory address using register values
			stage->mem_address = stage->rs1_value + stage->rs2_value;
		}
		else if (strcmp(stage->opcode, "LOAD") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "LDR") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		/* MOVC */
		else if (strcmp(stage->opcode, "MOVC") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "MOV") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "ADD") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "ADDL") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "SUB") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "SUBL") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "MUL") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "DIV") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "AND") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "OR") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "EX-OR") == 0) {
			set_reg_status(cpu, stage->rd, 1); // make desitination regs invalid so following instructions stall
		}
		else if (strcmp(stage->opcode, "BZ") == 0) {
			; // flush all the previous stages and start fetching instruction from mem_address in execute_two
		}
		else if (strcmp(stage->opcode, "BNZ") == 0) {
			; // flush all the previous stages and start fetching instruction from mem_address in execute_two
		}
		else if (strcmp(stage->opcode, "JUMP") == 0) {
			; // flush all the previous stages and start fetching instruction from mem_address in execute_two
		}
		else if (strcmp(stage->opcode, "HALT") == 0) {
			; // treat Halt as an interrupt stoped fetching instructions
		}
		else if (strcmp(stage->opcode, "NOP") == 0) {
			; // Do nothing its just a bubble
		}
		else {
			; // Do nothing
		}
		stage->executed = 1;
	}
	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Execute One", stage);
	}

	return 0;
}

/*
 * ########################################## EX Two Stage ##########################################
 */
int int_two_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[INT_TWO];
	stage->executed = 0;
	if (!stage->stalled) {

		/* Store */
		if (strcmp(stage->opcode, "STORE") == 0) {
			// create memory address using literal and register values
			stage->mem_address = stage->rs1_value + stage->buffer;
		}
		else if (strcmp(stage->opcode, "STR") == 0) {
			// create memory address using register values
			stage->mem_address = stage->rs1_value + stage->rs2_value;
		}
		else if (strcmp(stage->opcode, "LOAD") == 0) {
			// create memory address using literal and register values
			stage->mem_address = stage->rs1_value + stage->buffer;
		}
		else if (strcmp(stage->opcode, "LDR") == 0) {
			// create memory address using register values
			stage->mem_address = stage->rs1_value + stage->rs2_value;
		}
		/* MOVC */
		else if (strcmp(stage->opcode, "MOVC") == 0) {
			stage->rd_value = stage->buffer; // move buffer value to rd_value so it can be forwarded
		}
		else if (strcmp(stage->opcode, "MOV") == 0) {
			stage->rd_value = stage->rs1_value; // move rs1_value value to rd_value so it can be forwarded
		}
		else if (strcmp(stage->opcode, "ADD") == 0) {
			// add registers value and keep in rd_value for mem / writeback stage
			if ((stage->rs2_value > 0 && stage->rs1_value > INT_MAX - stage->rs2_value) ||
				(stage->rs2_value < 0 && stage->rs1_value < INT_MIN - stage->rs2_value)) {
				cpu->flags[OF] = 1; // there is an overflow
			}
			else {
				stage->rd_value = stage->rs1_value + stage->rs2_value;
				cpu->flags[OF] = 0; // there is no overflow
			}
		}
		else if (strcmp(stage->opcode, "ADDL") == 0) {
			// add literal and register value and keep in rd_value for mem / writeback stage
			if ((stage->buffer > 0 && stage->rs1_value > INT_MAX - stage->buffer) ||
				(stage->buffer < 0 && stage->rs1_value < INT_MIN - stage->buffer)) {
				cpu->flags[OF] = 1; // there is an overflow
			}
			else {
				stage->rd_value = stage->rs1_value + stage->buffer;
				cpu->flags[OF] = 0; // there is no overflow
			}
		}
		else if (strcmp(stage->opcode, "SUB") == 0) {
			// sub registers value and keep in rd_value for mem / writeback stage
			if (stage->rs2_value > stage->rs1_value) {
				stage->rd_value = stage->rs1_value - stage->rs2_value;
				cpu->flags[CF] = 1; // there is an carry
			}
			else {
				stage->rd_value = stage->rs1_value - stage->rs2_value;
				cpu->flags[CF] = 0; // there is no carry
			}
		}
		else if (strcmp(stage->opcode, "SUBL") == 0) {
			// sub literal and register value and keep in rd_value for mem / writeback stage
			if (stage->buffer > stage->rs1_value) {
				stage->rd_value = stage->rs1_value - stage->buffer;
				cpu->flags[CF] = 1; // there is an carry
			}
			else {
				stage->rd_value = stage->rs1_value - stage->buffer;
				cpu->flags[CF] = 0; // there is no carry
			}
		}
		else if (strcmp(stage->opcode, "MUL") == 0) {
			// mul registers value and keep in rd_value for mem / writeback stage
			stage->rd_value = stage->rs1_value * stage->rs2_value;
		}
		else if (strcmp(stage->opcode, "DIV") == 0) {
			// div registers value and keep in rd_value for mem / writeback stage
			if (stage->rs2_value != 0) {
				stage->rd_value = stage->rs1_value / stage->rs2_value;
			}
			else {
				fprintf(stderr, "Division By Zero Returning Value Zero\n");
				stage->rd_value = 0;
			}
		}
		else if (strcmp(stage->opcode, "AND") == 0) {
			// mul registers value and keep in rd_value for mem / writeback stage
			stage->rd_value = stage->rs1_value & stage->rs2_value;
		}
		else if (strcmp(stage->opcode, "OR") == 0) {
			// or registers value and keep in rd_value for mem / writeback stage
			stage->rd_value = stage->rs1_value | stage->rs2_value;
		}
		else if (strcmp(stage->opcode, "EX-OR") == 0) {
			// ex-or registers value and keep in rd_value for mem / writeback stage
			stage->rd_value = stage->rs1_value ^ stage->rs2_value;
		}
		else if (strcmp(stage->opcode, "BZ") == 0) {
			// load buffer value to mem_address
			stage->mem_address = stage->buffer;
			if (stage->rd_value==0) {
				// check address validity, pc-add % 4 should be 0
				if (((stage->pc + stage->mem_address)%4 == 0)&&!((stage->pc + stage->mem_address) < 4000)) {
					// reset status of rd in exe_one stage
					// No Need to do this as when EX_ONE will get executed it will have NOP and it wont inc reg valid count
					// set_reg_status(cpu, cpu->stage[EX_ONE].rd, -1); // make desitination regs valid so following instructions won't stall
					// flush previous instructions add NOP
					// add_bubble_to_stage(cpu, EX_ONE, 1); // next cycle Bubble will be executed
					// add_bubble_to_stage(cpu, DRF, 1); // next cycle Bubble will be executed
					// add_bubble_to_stage(cpu, F, 1); // next cycle Bubble will be executed
					// change pc value
					cpu->pc = stage->pc + stage->mem_address;
					// un stall Fetch and Decode stage if they are stalled
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
					// cpu->flags[ZF] = 0;
				}
				else {
					fprintf(stderr, "Invalid Branch Loction for %s\n", stage->opcode);
					fprintf(stderr, "Instruction %s Relative Address %d\n", stage->opcode, cpu->pc + stage->mem_address);
				}
			}
		}
		else if (strcmp(stage->opcode, "BNZ") == 0) {
			// load buffer value to mem_address
			stage->mem_address = stage->buffer;
			if (stage->rd_value!=0) {
				// check address validity, pc-add % 4 should be 0
				if (((stage->pc + stage->mem_address)%4 == 0)&&!((stage->pc + stage->mem_address) < 4000)) {
					// reset status of rd in exe_one stage
					// No Need to do this as when EX_ONE will get executed it will have NOP and it wont inc reg valid count
					// set_reg_status(cpu, cpu->stage[EX_ONE].rd, 0); // make desitination regs valid so following instructions won't stall
					// flush previous instructions add NOP
					// add_bubble_to_stage(cpu, EX_ONE, 1); // next cycle Bubble will be executed
					// add_bubble_to_stage(cpu, DRF, 1); // next cycle Bubble will be executed
					// add_bubble_to_stage(cpu, F, 1); // next cycle Bubble will be executed
					// change pc value
					cpu->pc = stage->pc + stage->mem_address;
					// un stall Fetch and Decode stage if they are stalled
					cpu->stage[DRF].stalled = 0;
					cpu->stage[F].stalled = 0;
					// cpu->flags[ZF] = 0;
				}
				else {
					fprintf(stderr, "Invalid Branch Loction for %s\n", stage->opcode);
					fprintf(stderr, "Instruction %s Relative Address %d\n", stage->opcode, cpu->pc + stage->mem_address);
				}
			}
		}
		else if (strcmp(stage->opcode, "JUMP") == 0) {
			// load buffer value to mem_address
			stage->mem_address = stage->rs1_value + stage->buffer;
			// check address validity, pc-add % 4 should be 0
			if (((stage->pc + stage->mem_address)%4 == 0)&&!((stage->pc + stage->mem_address) < 4000)) {
				// reset status of rd in exe_one stage
				// No Need to do this as when EX_ONE will get executed it will have NOP and it wont inc reg valid count
				// set_reg_status(cpu, cpu->stage[EX_ONE].rd, -1); // make desitination regs valid so following instructions won't stall
				// flush previous instructions add NOP
				// add_bubble_to_stage(cpu, EX_ONE, 1); // next cycle Bubble will be executed
				// add_bubble_to_stage(cpu, DRF, 1); // next cycle Bubble will be executed
				// add_bubble_to_stage(cpu, F, 1); // next cycle Bubble will be executed
				// change pc value
				cpu->pc = stage->mem_address;
				// un stall Fetch and Decode stage if they are stalled
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
			else {
				fprintf(stderr, "Invalid Branch Loction for %s\n", stage->opcode);
				fprintf(stderr, "Instruction %s Relative Address %d\n", stage->opcode, cpu->pc + stage->mem_address);
			}
		}
		else if (strcmp(stage->opcode, "HALT") == 0) {
			; // treat Halt as an interrupt stoped fetching instructions
		}
		else if (strcmp(stage->opcode, "NOP") == 0) {
			; // Do nothing its just a bubble
		}
		else {
			; // Do nothing
		}
		stage->executed = 1;
	}
	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Execute Two", stage);
	}

	return 0;
}

/*
 * ########################################## Mem One Stage ##########################################
 */
int memory_one_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[MEM_ONE];
	stage->executed = 0;
	if (!stage->stalled) {

		/* Store */
		if (strcmp(stage->opcode, "STORE") == 0) {
			// use memory address and write value in data_memory
			if (stage->mem_address > DATA_MEMORY_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for writing memory location :: %d\n", stage->mem_address);
			}
			else {
				cpu->data_memory[stage->mem_address] = stage->rd_value;
			}
		}
		else if (strcmp(stage->opcode, "STR") == 0) {
			// use memory address and write value in data_memory
			if (stage->mem_address > DATA_MEMORY_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for writing memory location :: %d\n", stage->mem_address);
			}
			else {
				cpu->data_memory[stage->mem_address] = stage->rd_value;
			}
		}
		else if (strcmp(stage->opcode, "LOAD") == 0) {
			// use memory address and write value in data_memory
			if (stage->mem_address > DATA_MEMORY_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing memory location :: %d\n", stage->mem_address);
			}
			else {
				stage->rd_value = cpu->data_memory[stage->mem_address];
			}
		}
		else if (strcmp(stage->opcode, "LDR") == 0) {
			// use memory address and write value in data_memory
			if (stage->mem_address > DATA_MEMORY_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing memory location :: %d\n", stage->mem_address);
			}
			else {
				stage->rd_value = cpu->data_memory[stage->mem_address];
			}
		}
		/* MOVC */
		else if (strcmp(stage->opcode, "MOVC") == 0) {
			// can flags be used to make better decision
			; // Nothing for now do operation in writeback stage
		}
		else if (strcmp(stage->opcode, "MOV") == 0) {
			// can flags be used to make better decision
			; // Nothing for now do operation in writeback stage
		}
		else if (strcmp(stage->opcode, "ADD") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "ADDL") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "SUB") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "SUBL") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "MUL") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "DIV") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "AND") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "OR") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "EX-OR") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "BZ") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "BNZ") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "JUMP") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "HALT") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "NOP") == 0) {
			; // Nothing for now
		}
		else {
			; // Nothing
		}
		stage->executed = 1;
	}
	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Memory One", stage);
	}

	return 0;
}

/*
 * ########################################## Mem Two Stage ##########################################
 */
int memory_two_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[MEM_TWO];
	stage->executed = 0;
	if (!stage->stalled) {

		/* Store */
		if (strcmp(stage->opcode, "STORE") == 0) {
			// use memory address and write value in data_memory
			if (stage->mem_address > DATA_MEMORY_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for writing memory location :: %d\n", stage->mem_address);
			}
			else {
				cpu->data_memory[stage->mem_address] = stage->rd_value;
			}
		}
		else if (strcmp(stage->opcode, "STR") == 0) {
			// use memory address and write value in data_memory
			if (stage->mem_address > DATA_MEMORY_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for writing memory location :: %d\n", stage->mem_address);
			}
			else {
				cpu->data_memory[stage->mem_address] = stage->rd_value;
			}
		}
		else if (strcmp(stage->opcode, "LOAD") == 0) {
			// use memory address and write value in data_memory
			if (stage->mem_address > DATA_MEMORY_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing memory location :: %d\n", stage->mem_address);
			}
			else {
				stage->rd_value = cpu->data_memory[stage->mem_address];
			}
		}
		else if (strcmp(stage->opcode, "LDR") == 0) {
			// use memory address and write value in data_memory
			if (stage->mem_address > DATA_MEMORY_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing memory location :: %d\n", stage->mem_address);
			}
			else {
				stage->rd_value = cpu->data_memory[stage->mem_address];
			}
		}
		/* MOVC */
		else if (strcmp(stage->opcode, "MOVC") == 0) {
			; // stage->rd_value = stage->buffer; // move buffer value to rd_value so it can be forwarded
		}
		else if (strcmp(stage->opcode, "MOV") == 0) {
			; // stage->rd_value = stage->rs1_value; // move rs1_value value to rd_value so it can be forwarded
		}
		else if (strcmp(stage->opcode, "ADD") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "ADDL") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "SUB") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "SUBL") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "MUL") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "DIV") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "AND") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "OR") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "EX-OR") == 0) {
			// can flags be used to make better decision
			; // Nothing for now holding rd_value from exe stage
		}
		else if (strcmp(stage->opcode, "BZ") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "BNZ") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "JUMP") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "HALT") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "NOP") == 0) {
			; // Nothing for now
		}
		else {
			; // Nothing
		}
		stage->executed = 1;
	}
	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Memory Two", stage);
	}

	return 0;
}

/*
 * ########################################## Writeback Stage ##########################################
 */
int writeback(APEX_CPU* cpu) {

	int ret = 0;

	CPU_Stage* stage = &cpu->stage[WB];
	stage->executed = 0;
	if (!stage->stalled) {

		/* Store */
		if (strcmp(stage->opcode, "STORE") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "STR") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "LOAD") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "LDR") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		/* MOVC */
		else if (strcmp(stage->opcode, "MOVC") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "MOV") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "ADD") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				if (stage->rd_value == 0) {
					cpu->flags[ZF] = 1; // computation resulted value zero
				}
				else {
					cpu->flags[ZF] = 0; // computation did not resulted value zero
				}
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "ADDL") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				if (stage->rd_value == 0) {
					cpu->flags[ZF] = 1; // computation resulted value zero
				}
				else {
					cpu->flags[ZF] = 0; // computation did not resulted value zero
				}
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "SUB") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				if (stage->rd_value == 0) {
					cpu->flags[ZF] = 1; // computation resulted value zero
				}
				else {
					cpu->flags[ZF] = 0; // computation did not resulted value zero
				}
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "SUBL") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				if (stage->rd_value == 0) {
					cpu->flags[ZF] = 1; // computation resulted value zero
				}
				else {
					cpu->flags[ZF] = 0; // computation did not resulted value zero
				}
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "MUL") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				if (stage->rd_value == 0) {
					cpu->flags[ZF] = 1; // computation resulted value zero
				}
				else {
					cpu->flags[ZF] = 0; // computation did not resulted value zero
				}
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "DIV") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				if (stage->rs1_value % stage->rs2_value != 0) {
					cpu->flags[ZF] = 1; // remainder / operation result is zero
				}
				else {
					cpu->flags[ZF] = 0; // remainder / operation result is not zero
				}
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also un-stall instruction which were dependent on rd reg
				// values are valid un-stall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "AND") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "OR") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "EX-OR") == 0) {
			// use rd address and write value in register
			if (stage->rd > REGISTER_FILE_SIZE) {
				// Segmentation fault
				fprintf(stderr, "Segmentation fault for accessing register location :: %d\n", stage->rd);
			}
			else {
				cpu->regs[stage->rd] = stage->rd_value;
				set_reg_status(cpu, stage->rd, -1); // make desitination regs valid so following instructions won't stall
				// also unstall instruction which were dependent on rd reg
				// values are valid unstall DF and Fetch Stage
				cpu->stage[DRF].stalled = 0;
				cpu->stage[F].stalled = 0;
			}
		}
		else if (strcmp(stage->opcode, "BZ") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "BNZ") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "JUMP") == 0) {
			; // Nothing for now
		}
		else if (strcmp(stage->opcode, "HALT") == 0) {
			ret = HALTED; // return exit code halt to stop simulation
		}
		else if (strcmp(stage->opcode, "NOP") == 0) {
			; // Nothing for now
		}
		else {
			if (strcmp(stage->opcode, "") == 0) {
				ret = EMPTY; // return exit code empty to stop simulation
			}
		}

		stage->executed = 1;
		cpu->ins_completed++;

	}
	// But If Fetch has Something and Decode Has NOP Do Not Un Stall Fetch
	// Intrupt Flag is set
	if ((cpu->flags[IF])&&(strcmp(cpu->stage[DRF].opcode, "NOP") == 0)){
		cpu->stage[F].stalled = 1;
	}
	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Writeback", stage);
	}

	return ret;
}

int issue_queue(APEX_CPU* cpu){
	;
}

int ls_queue(APEX_CPU* cpu) {
	;
}

int cpu_execute(APEX_CPU* cpu) {
	;
}

int rob_commit(APEX_CPU* cpu) {
	;
}




// static void push_stages(APEX_CPU* cpu) {
//
// 	cpu->stage[WB] = cpu->stage[MEM_TWO];
// 	cpu->stage[WB].executed = 0;
// 	cpu->stage[MEM_TWO] = cpu->stage[MEM_ONE];
// 	cpu->stage[MEM_TWO].executed = 0;
// 	cpu->stage[MEM_ONE] = cpu->stage[EX_TWO];
// 	cpu->stage[MEM_ONE].executed = 0;
// 	cpu->stage[EX_TWO] = cpu->stage[EX_ONE];
// 	cpu->stage[EX_TWO].executed = 0;
// 	if (!cpu->stage[DRF].stalled) {
// 		cpu->stage[EX_ONE] = cpu->stage[DRF];
// 		cpu->stage[EX_ONE].executed = 0;
// 	}
// 	else {
// 		add_bubble_to_stage(cpu, EX_ONE, 0); // next cycle Bubble will be executed
// 		cpu->stage[EX_ONE].executed = 0;
// 	}
// 	if (!cpu->stage[F].stalled) {
// 		cpu->stage[DRF].rd = -99;
// 		cpu->stage[DRF].rs1 = -99;
// 		cpu->stage[DRF].rs2 = -99;
// 		cpu->stage[DRF] = cpu->stage[F];
// 		cpu->stage[DRF].executed = 0;
// 	}
// 	else if (!cpu->stage[DRF].stalled) {
// 		add_bubble_to_stage(cpu, DRF, 0); // next cycle Bubble will be executed
// 		cpu->stage[DRF].executed = 0;
// 	}
// 	if (ENABLE_PUSH_STAGE_PRINT) {
// 		printf("\n--------------------------------\n");
// 		printf("Clock Cycle #: %d Instructions Pushed\n", cpu->clock);
// 		printf("%-15s: Executed: Instruction\n", "Stage");
// 		printf("--------------------------------\n");
// 		print_stage_content("Writeback", &cpu->stage[WB]);
// 		print_stage_content("Memory Two", &cpu->stage[MEM_TWO]);
// 		print_stage_content("Memory One", &cpu->stage[MEM_ONE]);
// 		print_stage_content("Execute Two", &cpu->stage[EX_TWO]);
// 		print_stage_content("Execute One", &cpu->stage[EX_ONE]);
// 		print_stage_content("Decode/RF", &cpu->stage[DRF]);
// 		print_stage_content("Fetch", &cpu->stage[F]);
// 	}
// }
/*
 * ########################################## CPU Run ##########################################
 */
int APEX_cpu_run(APEX_CPU* cpu, int num_cycle) {

	int ret = 0;

	while (ret==0) {

		/* Requested number of cycle committed, so pause and exit */
		if ((num_cycle>0)&&(cpu->clock == num_cycle)) {
			printf("Requested %d Cycle Completed\n", num_cycle);
			break;
		}
		else {
			cpu->clock++; // places here so we can see prints aligned with executions

			if (ENABLE_DEBUG_MESSAGES) {
				printf("\n--------------------------------\n");
				printf("Clock Cycle #: %d\n", cpu->clock);
				printf("%-15s: Executed: Instruction\n", "Stage");
				printf("--------------------------------\n");
			}

			int stage_ret = 0;
			stage_ret = fetch(cpu); // fetch inst from code memory
			stage_ret = decode(cpu); // decode will have rename and dispatch func call inside
			stage_ret = issue_queue(cpu); // issue will have issue func call inside
			// issue will issue load/store to LSQ and rob, then lsq will send inst to mem
			// issue will issue other inst to FU and rob
			stage_ret = ls_queue(cpu); // lsq will dispatch inst to mem
			stage_ret = cpu_execute(cpu); // cpu execute will hav diff FU calls
			stage_ret = rob_commit(cpu); // rob to commit inst executed in FU

			// why we are executing from behind ??
			stage_ret = writeback(cpu);
			// if ((stage_ret == HALT) || (stage_ret == EMPTY)) {
			// 	if (ENABLE_DEBUG_MESSAGES) {
			// 		print_stage_content("Memory Two", &cpu->stage[MEM_TWO]);
			// 		print_stage_content("Memory One", &cpu->stage[MEM_ONE]);
			// 		print_stage_content("Execute Two", &cpu->stage[EX_TWO]);
			// 		print_stage_content("Execute One", &cpu->stage[EX_ONE]);
			// 		print_stage_content("Decode/RF", &cpu->stage[DRF]);
			// 		print_stage_content("Fetch", &cpu->stage[F]);
			// 	}
			// 	if (stage_ret == HALTED) {
			// 		fprintf(stderr, "Simulation Stoped ....\n");
			// 		printf("Instruction HALT Encountered\n");
			// 	}
			// 	else if (stage_ret == EMPTY) {
			// 		fprintf(stderr, "Simulation Stoped ....\n");
			// 		printf("No More Instructions Encountered\n");
			// 	}
			// 	ret = stage_ret;
			// 	break; // break when halt is encountered or empty instruction goes to writeback
			// }
			// stage_ret = memory_two(cpu);
			// stage_ret = memory_one(cpu);
			// stage_ret = execute_two(cpu);
			// stage_ret = execute_one(cpu);
			// stage_ret = decode(cpu);
			// stage_ret = fetch(cpu);
			// if ((stage_ret!=HALT)&&(stage_ret!=SUCCESS)) {
			// 	ret = stage_ret;
			// }
			// push_stages(cpu);
		}
	}

	return ret;
}
