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
#include "forwarding.h"


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
	for (int i = 0; i < NUM_STAGES; i++) {
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


/*
 * ########################################## Print Stage ##########################################
*/
static void print_instruction(CPU_Stage* stage) {
	// This function prints operands of instructions in stages.
	switch (stage->inst_type) {

		case STORE: case LOAD: case ADDL: case SUBL:
			printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->buffer);
			break;

		case STR: case LDR: case ADD: case SUB:case MUL: case DIV: case AND: case OR: case EXOR:
			printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
			break;

		case MOVC: case JUMP:
			printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->buffer);
			break;

		case MOV:
			printf("%s,R%d,R%d ", stage->opcode, stage->rd, stage->rs1);
			break;

		case BZ: case BNZ:
			printf("%s,#%d ", stage->opcode, stage->buffer);
			break;

		case HALT: case NOP:
			printf("%s ", stage->opcode);
			break;

		default:
			if (!stage->empty) {
				printf("<<- %s ->>", stage->opcode);
			}
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
		printf("\n============ STATE OF CPU FLAGS ============\n");
		// print all Flags
		printf("Falgs::  ZeroFlag, CarryFlag, OverflowFlag, InterruptFlag\n");
		printf("Values:: %d\t|\t%d\t|\t%d\t|\t%d\n", cpu->flags[ZF],cpu->flags[CF],cpu->flags[OF],cpu->flags[IF]);

		// print all regs along with valid bits
		printf("\n============ STATE OF ARCHITECTURAL REGISTER FILE ============\n");
		printf("NOTE :: 0 Means Valid & 1 Means Invalid\n");
		printf("Registers, Values, Invalid\n");
		for (int i=0;i<REGISTER_FILE_SIZE;i++) {
			printf("R%02d\t|\t%02d\t|\t%d\n", i, cpu->regs[i], cpu->regs_invalid[i]);
		}

		// print 100 memory location
		printf("\n============ STATE OF DATA MEMORY ============\n");
		printf("Mem Location, Values\n");
		for (int i=0;i<100;i++) {
			printf("M%02d\t|\t%02d\n", i, cpu->data_memory[i]);
		}
		printf("\n");
	}
}


/*
 * ########################################## Regs Status Stage ##########################################
*/
void set_arch_reg_status(APEX_CPU* cpu, APEX_RENAME* rename_table, int reg_number, int status) {
	// Set Reg Status function
	// NOTE: instead of set inc or dec regs_invalid
	if (reg_number > REGISTER_FILE_SIZE) {
		// Segmentation fault
		fprintf(stderr, "Segmentation fault for Register location :: %d\n", reg_number);
	}
	else {
		int src_reg = -1;
		src_reg = get_phy_reg_renamed_tag(reg_number, rename_table);
		if (src_reg<0) {
			;// the phy reg is not tagged with arch rsgs
		}
		else {
			set_reg_status(cpu, src_reg, status);
		}
	}
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
		stage->rd_valid = 0;
		stage->rs1 = current_ins->rs1;
		stage->rs1_valid = 0;
		stage->rs2 = current_ins->rs2;
		stage->rs2_valid = 0;
		stage->imm = current_ins->imm;
		stage->buffer = current_ins->imm;
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
			stage->rd_valid = 0;
			stage->rs1 = current_ins->rs1;
			stage->rs1_valid = 0;
			stage->rs2 = current_ins->rs2;
			stage->rs2_valid = 0;
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
int decode(APEX_CPU* cpu, APEX_RENAME* rename_table) {

	CPU_Stage* stage = &cpu->stage[DRF];
	stage->executed = 0;
	int ret = -1;
	// decode should stall if IQ is full
	if (!stage->stalled) {
		/* Read data from register file for store */
		switch(stage->inst_type) {

			case STORE:  // ************************************* STORE ************************************* //

				stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if desc regs are renamed
				if (check_if_reg_renamed(stage->rd, rename_table)==SUCCESS) {
					// get the desc reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rd), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rd);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rd)==INVALID) {
						stage->rd_value = get_reg_values(cpu, stage->rd);
						stage->rd_valid = VALID;
					}
				}
				break;

			case STR:  // ************************************* STR ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs2)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if desc regs are renamed
				if (check_if_reg_renamed(stage->rd, rename_table)==SUCCESS) {
					// get the desc reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rd), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rd);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rd)==INVALID) {
						stage->rd_value = get_reg_values(cpu, stage->rd);
						stage->rd_valid = VALID;
					}
				}

				break;

			case LOAD:  // ************************************* LOAD ************************************* //
				// read literal and register values
				stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rd);
					}
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case LDR:  // ************************************* LDR ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs2)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rd);
					}
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case MOVC:  // ************************************* MOVC ************************************* //
				// read literal values
				stage->buffer = stage->imm; // keeping literal value in buffer to load in mem stage
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case MOV:  // ************************************* MOV ************************************* //
				// read register values
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case ADD:  // ************************************* ADD ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs2)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case ADDL:  // ************************************* ADDL ************************************* //
				// read only values of last two registers
				stage->buffer = stage->imm; // keeping literal value in buffer to add in exe stage
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rd);
					}
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case SUB:   // ************************************* SUB ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs2)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case SUBL:  // ************************************* SUBL ************************************* //
				// read only values of last two registers
				stage->buffer = stage->imm; // keeping literal value in buffer to add in exe stage
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case MUL:  // ************************************* MUL ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs2)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case DIV:  // ************************************* DIV ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case AND:  // ************************************* AND ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs2)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case OR:  // ************************************* OR ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs2)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case EXOR:  // ************************************* EX-OR ************************************* //
				// read only values of last two registers
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs1_value = get_reg_values(cpu, stage->rs1);
						stage->rs1_valid = VALID;
					}
				}
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs2, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs2), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs2);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
				}
				// check if renaming can be done
				if (can_rename_reg_tag(rename_table)==SUCCESS) {
					// change the desc regs tag
					ret = rename_desc_reg(&(stage->rd), rename_table);
				}
				else {
					printf("Cannot Rename Register R%d of pc(%d) :: %s\n", stage->rd, stage->pc, stage->opcode);
				}
				break;

			case BZ:  // ************************************* BZ ************************************* //
				// read literal values
				stage->buffer = stage->imm; // keeping literal value in buffer to jump in exe stage
				break;

			case BNZ:  // ************************************* BNZ ************************************* //
				// read literal values
				stage->buffer = stage->imm; // keeping literal value in buffer to jump in exe stage
				break;

			case JUMP:   // ************************************* JUMP ************************************* //
				// read literal and register values
				stage->buffer = stage->imm; // keeping literal value in buffer to calculate mem add in exe stage
				// check if src regs are renamed
				if (check_if_reg_renamed(stage->rs1, rename_table)==SUCCESS) {
					// get the src reg renamed tag
					ret = get_reg_renamed_tag(&(stage->rs1), rename_table);
					if (ret!=SUCCESS) {
						printf("Failed to get renamed tag for R%d\n", stage->rs1);
					}
				}
				else {
					// check arch reg if valid read from them
					// here 0 is valid and any other number is invalid
					if(get_reg_status(cpu, stage->rs1)==INVALID) {
						stage->rs2_value = get_reg_values(cpu, stage->rs2);
						stage->rs2_valid = VALID;
					}
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
 * ########################################## Int FU One Stage ##########################################
*/
int int_one_stage(APEX_CPU* cpu, APEX_RENAME* rename_table) {

	CPU_Stage* stage = &cpu->stage[INT_ONE];
	stage->executed = 0;
	if ((!stage->stalled)&&(!stage->empty)) {
		/* Read data from register file for store */
		switch(stage->inst_type) {

			case STORE: case STR:  // ************************************* STORE or STR ************************************* //
				break;
			// ************************************* LOAD to EX-OR ************************************* //
			case LOAD: case LDR: case MOVC: case MOV: case ADD: case ADDL: case SUB: case SUBL: case DIV: case AND: case OR: case EXOR:
				// make desitination regs invalid count increment by one following instructions stall
				set_arch_reg_status(cpu, rename_table, stage->rd, 1);
				stage->rd_valid = INVALID;
				break;

			case JUMP:  // ************************************* JUMP ************************************* //
				break;

			case HALT:  // ************************************* HALT ************************************* //
				break;

			case NOP:  // ************************************* NOP ************************************* //
				break;

			default:
				break;
		}
		stage->executed = 1;
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Int FU One", stage);
	}

	return 0;
}

/*
 * ########################################## Int FU Two Stage ##########################################
*/
int int_two_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[INT_TWO];
	stage->executed = 0;
	if ((!stage->stalled)&&(!stage->empty)) {
		/* Read data from register file for store */
		switch(stage->inst_type) {

			case STORE: case LOAD:  // ************************************* STORE or LOAD ************************************* //
				// create memory address using literal and register values
				stage->mem_address = stage->rs1_value + stage->buffer;
				break;

			case STR: case LDR: // ************************************* STR or LDR ************************************* //
				// create memory address using two source register values
				stage->mem_address = stage->rs1_value + stage->rs2_value;
				break;

			case MOVC:	// ************************************* MOVC ************************************* //
				// move buffer value to rd_value so it can be forwarded
				stage->rd_value = stage->buffer;
				stage->rd_valid = VALID;
				break;

			case MOV:	// ************************************* MOV ************************************* //
				// move rs1_value value to rd_value so it can be forwarded
				stage->rd_value = stage->rs1_value;
				stage->rd_valid = VALID;
				break;

			case ADD:	// ************************************* ADD ************************************* //
				// add registers value and keep in rd_value for mem / writeback stage
				if ((stage->rs2_value > 0 && stage->rs1_value > INT_MAX - stage->rs2_value) ||
					(stage->rs2_value < 0 && stage->rs1_value < INT_MIN - stage->rs2_value)) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Overflow Occurred\n");
					}
					stage->rd_valid = VALID;
					cpu->flags[OF] = 1; // there is an overflow
				}
				else {
					stage->rd_value = stage->rs1_value + stage->rs2_value;
					stage->rd_valid = VALID;
					cpu->flags[OF] = 0; // there is no overflow
					if (stage->rd_value == 0) {
						cpu->flags[ZF] = 1; // computation resulted value zero
					}
					else {
						cpu->flags[ZF] = 0; // computation did not resulted value zero
					}
				}
				break;

			case ADDL:	// ************************************* ADDL ************************************* //
				// add literal and register value and keep in rd_value for mem / writeback stage
				if ((stage->buffer > 0 && stage->rs1_value > INT_MAX - stage->buffer) ||
					(stage->buffer < 0 && stage->rs1_value < INT_MIN - stage->buffer)) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Overflow Occurred\n");
					}
					stage->rd_valid = VALID;
					cpu->flags[OF] = 1; // there is an overflow
				}
				else {
					stage->rd_value = stage->rs1_value + stage->buffer;
					stage->rd_valid = VALID;
					cpu->flags[OF] = 0; // there is no overflow
					if (stage->rd_value == 0) {
						cpu->flags[ZF] = 1; // computation resulted value zero
					}
					else {
						cpu->flags[ZF] = 0; // computation did not resulted value zero
					}
				}
				break;

			case SUB:	// ************************************* SUB ************************************* //
				// sub registers value and keep in rd_value for mem / writeback stage
				if (stage->rs2_value > stage->rs1_value) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Carry Occurred\n");
					}
					stage->rd_value = stage->rs1_value - stage->rs2_value;
					stage->rd_valid = VALID;
					cpu->flags[CF] = 1; // there is an carry
				}
				else {
					stage->rd_value = stage->rs1_value - stage->rs2_value;
					stage->rd_valid = VALID;
					cpu->flags[CF] = 0; // there is no carry
					if (stage->rd_value == 0) {
						cpu->flags[ZF] = 1; // computation resulted value zero
					}
					else {
						cpu->flags[ZF] = 0; // computation did not resulted value zero
					}
				}
				break;

			case SUBL:	// ************************************* SUBL ************************************* //
				// sub literal and register value and keep in rd_value for mem / writeback stage
				if (stage->buffer > stage->rs1_value) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Carry Occurred\n");
					}
					stage->rd_value = stage->rs1_value - stage->buffer;
					stage->rd_valid = VALID;
					cpu->flags[CF] = 1; // there is an carry
				}
				else {
					stage->rd_value = stage->rs1_value - stage->buffer;
					stage->rd_valid = VALID;
					cpu->flags[CF] = 0; // there is no carry
					if (stage->rd_value == 0) {
						cpu->flags[ZF] = 1; // computation resulted value zero
					}
					else {
						cpu->flags[ZF] = 0; // computation did not resulted value zero
					}
				}
				break;

			case DIV:	// ************************************* DIV ************************************* //
				// div registers value and keep in rd_value for mem / writeback stage
				if (stage->rs2_value != 0) {
					stage->rd_value = stage->rs1_value / stage->rs2_value;
					stage->rd_valid = VALID;
					if (stage->rd_value == 0) {
						cpu->flags[ZF] = 1; // computation resulted value zero
					}
					else {
						cpu->flags[ZF] = 0; // computation did not resulted value zero
					}
				}
				else {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Division By Zero Returning Value Zero\n");
					}
					stage->rd_value = 0;
					stage->rd_valid = VALID;
				}
				break;

			case AND:	// ************************************* AND ************************************* //
				// logical AND registers value and keep in rd_value for mem / writeback stage
				stage->rd_value = stage->rs1_value & stage->rs2_value;
				stage->rd_valid = VALID;
				break;

			case OR:	// ************************************* OR ************************************* //
				// logical OR registers value and keep in rd_value for mem / writeback stage
				stage->rd_value = stage->rs1_value | stage->rs2_value;
				stage->rd_valid = VALID;
				break;

			case EXOR:	// ************************************* EX-OR ************************************* //
				// logical OR registers value and keep in rd_value for mem / writeback stage
				stage->rd_value = stage->rs1_value ^ stage->rs2_value;
				stage->rd_valid = VALID;
				break;

			case JUMP:  // ************************************* JUMP ************************************* //
				break;

			case HALT:  // ************************************* HALT ************************************* //
				break;

			case NOP:  // ************************************* NOP ************************************* //
				break;

			default:
				break;
		}
		stage->executed = 1;
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Int FU Two", stage);
	}

	return 0;
}


/*
 * ########################################## Mul FU One Stage ##########################################
*/
int mul_one_stage(APEX_CPU* cpu, APEX_RENAME* rename_table) {

	CPU_Stage* stage = &cpu->stage[MUL_ONE];
	stage->executed = 0;
	if ((!stage->stalled)&&(!stage->empty)) {
		/* Read data from register file for store */
		switch(stage->inst_type) {

			case MUL:  // ************************************* MUL ************************************* //
				// mul registers value and keep in rd_value for mem / writeback stage
				// if possible check y it requires 3 cycle
				set_arch_reg_status(cpu, rename_table, stage->rd, 1);
				stage->rd_valid = INVALID;
				break;

			default:
				break;
		}
		stage->executed = 1;
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Mul FU One", stage);
	}

	return 0;
}


/*
 * ########################################## Mul FU Two Stage ##########################################
*/
int mul_two_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[MUL_TWO];
	stage->executed = 0;
	if ((!stage->stalled)&&(!stage->empty)) {
		/* Read data from register file for store */
		switch(stage->inst_type) {

			case MUL:  // ************************************* MUL ************************************* //
				// mul registers value and keep in rd_value for mem / writeback stage
				// if possible check y it requires 3 cycle
				stage->rd_value = stage->rs1_value * stage->rs2_value;
				stage->rd_valid = INVALID;
				break;

			default:
				break;
		}
		stage->executed = 1;
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Mul FU Two", stage);
	}

	return 0;
}


/*
 * ########################################## Mul FU Three Stage ##########################################
*/
int mul_three_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[MUL_THREE];
	stage->executed = 0;
	if ((!stage->stalled)&&(!stage->empty)) {
		/* Read data from register file for store */
		switch(stage->inst_type) {

			case MUL:  // ************************************* MUL ************************************* //
				// mul registers value and keep in rd_value for mem / writeback stage
				// if possible check y it requires 3 cycle
				stage->rd_value = stage->rs1_value * stage->rs2_value;
				stage->rd_valid = VALID;
				if (stage->rd_value == 0) {
					cpu->flags[ZF] = 1; // computation resulted value zero
				}
				else {
					cpu->flags[ZF] = 0; // computation did not resulted value zero
				}
				break;

			default:
				break;
		}
		stage->executed = 1;
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Mul FU Three", stage);
	}

	return 0;
}


/*
 * ########################################## Branch FU Stage ##########################################
*/
int branch_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[BRANCH];
	stage->executed = 0;
	if ((!stage->stalled)&&(!stage->empty)) {
		/* Read data from register file for store */
		int new_pc;
		switch(stage->inst_type) {

			case BZ:  // ************************************* BZ ************************************* //
				// load buffer value to mem_address
				stage->mem_address = stage->buffer;
				new_pc = stage->pc + stage->mem_address;
				if ((new_pc < 4000)||(new_pc > ((cpu->code_memory_size*4)+4000))) {
					fprintf(stderr, "Instruction %s Invalid Relative Address %d\n", stage->opcode, new_pc);
				}
				else {
					if (cpu->flags[ZF] == VALID) {
						clear_stage_entry(cpu, DRF);
						clear_stage_entry(cpu, F);
						stage->rd_value = INVALID;
						stage->rd_valid = VALID;
						cpu->pc = new_pc;
						// stall F so it wont fetch in same cycle
						cpu->stage[F].stalled = VALID;
					}
					else {
						stage->rd_value = VALID;
						stage->rd_valid = INVALID;
					}
				}
				break;

			case BNZ:  // ************************************* BNZ ************************************* //
				// load buffer value to mem_address
				stage->mem_address = stage->buffer;
				new_pc = stage->pc + stage->mem_address;
				if ((new_pc < 4000)||(new_pc > ((cpu->code_memory_size*4)+4000))) {
					fprintf(stderr, "Instruction %s Invalid Relative Address %d\n", stage->opcode, new_pc);
				}
				else {
					if (cpu->flags[ZF] == INVALID) {
						stage->rd_value = VALID;
						stage->rd_valid = VALID;
						clear_stage_entry(cpu, DRF);
						clear_stage_entry(cpu, F);
						cpu->pc = new_pc;
						// stall F so it wont fetch in same cycle
						cpu->stage[F].stalled = VALID;
					}
					else {
						stage->rd_value = INVALID;
						stage->rd_valid = INVALID;
					}
				}
				break;

			case JUMP:  // ************************************* BNZ ************************************* //
				// load buffer value to mem_address
				stage->mem_address = stage->rs1_value + stage->buffer;
				new_pc = stage->mem_address;
				if ((new_pc < 4000)||(new_pc > ((cpu->code_memory_size*4)+4000))) {
					fprintf(stderr, "Instruction %s Invalid Address %d\n", stage->opcode, new_pc);
				}
				else {
					// so rob can commite
					stage->rd_value = INVALID;
					stage->rd_valid = VALID;
					// just change the pc and flush the F and DRF
					cpu->pc = new_pc;
					clear_stage_entry(cpu, DRF);
					clear_stage_entry(cpu, F);
				}
				break;

			default:
				break;
		}
		stage->executed = 1;
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Branch FU", stage);
	}

	return 0;
}


/*
 * ########################################## Mem FU Stage ##########################################
*/
int mem_stage(APEX_CPU* cpu) {

	CPU_Stage* stage = &cpu->stage[MEM];
	stage->executed = 0;
	if ((!stage->stalled)&&(!stage->empty)) {
		/* Read data from register file for store */
		if (stage->stage_cycle >= 3) {

			switch(stage->inst_type) {

				case STORE: case STR:  // ************************************* STORE or STR ************************************* //
				// use memory address and write value in data_memory
				if (stage->mem_address > DATA_MEMORY_SIZE) {
					// Segmentation fault
					fprintf(stderr, "Segmentation fault for writing memory location :: %d\n", stage->mem_address);
				}
				else {
					// wait for 3 cycles
					if (stage->rd_valid == VALID) {
						cpu->data_memory[stage->mem_address] = stage->rd_value;
						stage->executed = 1;
					}
					else {
						stage->executed = 0;
					}
				}
				break;

				case LOAD: case LDR:  // ************************************* LOAD or LDR ************************************* //
				// use memory address and write value in desc reg
				if (stage->mem_address > DATA_MEMORY_SIZE) {
					// Segmentation fault
					fprintf(stderr, "Segmentation fault for accessing memory location :: %d\n", stage->mem_address);
				}
				else {
					// wait for 3 cycles
					stage->rd_value = cpu->data_memory[stage->mem_address];
					stage->rd_valid = VALID;
					stage->executed = 1;
				}
				break;

				default:
				break;
			}
		}
		else {
			stage->stage_cycle += 1;
		}
	}

	if (ENABLE_DEBUG_MESSAGES) {
		print_stage_content("Mem FU", stage);
		printf("Mem FU Cycle :: %d\n",stage->stage_cycle);
	}

	return 0;
}


/*
 * ########################################## Writeback Stage ##########################################
*/
int writeback_stage(APEX_CPU* cpu, APEX_LSQ* ls_queue, APEX_IQ* issue_queue, APEX_ROB* rob, APEX_RENAME* rename_table) {

	// take MUL_THREE, INT_TWO Stage and update the ROB entry so in next cycle
	// instruction can be commited

	int cpu_stages[CPU_OUT_STAGES] = {INT_TWO, MUL_THREE, BRANCH, MEM};

	for (int i=0; i<CPU_OUT_STAGES; i++) {

		CPU_Stage* stage = &cpu->stage[cpu_stages[i]];

		if ((stage->executed)&&(!stage->empty)) {
			int ret = -1;
			LS_IQ_Entry ls_iq_entry = {
				.inst_type = stage->inst_type,
				.executed = stage->executed,
				.pc = stage->pc,
				.rd = stage->rd,
				.rd_value = stage->rd_value,
				.rd_valid = stage->rd_valid,
				.rs1 = stage->rs1,
				.rs1_value = stage->rs1_value,
				.rs1_valid = stage->rs1_valid,
				.rs2 = stage->rs2,
				.rs2_value = stage->rs2_value,
				.rs2_valid = stage->rs2_valid,
				.buffer = stage->buffer,
				.mem_address = stage->mem_address,
				.lsq_index = stage->lsq_index,
				.stage_cycle = stage->stage_cycle};

			ROB_Entry rob_entry = {
				.inst_type = stage->inst_type,
				.executed = stage->executed,
				.pc = stage->pc,
				.rd = stage->rd,
				.rd_value = stage->rd_value,
				.rd_valid = stage->rd_valid,
				.rs1 = stage->rs1,
				.rs1_value = stage->rs1_value,
				.rs1_valid = stage->rs1_valid,
				.rs2 = stage->rs2,
				.rs2_value = stage->rs2_value,
				.rs2_valid = stage->rs2_valid,
				.buffer = stage->buffer,
				.exception = stage->rd_valid,
				.stage_cycle = stage->stage_cycle};

			if ((cpu_stages[i]==INT_TWO)&&((stage->inst_type==STORE)||(stage->inst_type==STR)||(stage->inst_type==LOAD)||(stage->inst_type==LDR))) {
				ret = update_ls_queue_entry_mem_address(ls_queue, ls_iq_entry);
				if (ret!=SUCCESS) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Failed to Update LSQ Entry (%d) for pc(%d):: %.5s\n", ret, stage->pc, stage->opcode);
					}
				}
				continue;
			}
			else if (cpu_stages[i]==BRANCH) {
				ret = update_reorder_buffer_entry_data(rob, rob_entry);
				if (ret==ERROR) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Writeback Failed to Update Rob Entry (%d) for pc(%d):: %.5s\n", ret, stage->pc, stage->opcode);
					}
				}
				if (ret==FAILURE) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Writeback Nothing to Update in Rob Entry (%d) for pc(%d):: %.5s\n", ret, stage->pc, stage->opcode);
					}
				}
				continue;
			}
			else {
				ret = update_reorder_buffer_entry_data(rob, rob_entry);
				if (ret==ERROR) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Writeback Failed to Update Rob Entry (%d) for pc(%d):: %.5s\n", ret, stage->pc, stage->opcode);
					}
				}
				if (ret==FAILURE) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Writeback Nothing to Update in Rob Entry (%d) for pc(%d):: %.5s\n", ret, stage->pc, stage->opcode);
					}
				}

				ret = update_issue_queue_entry(issue_queue, ls_iq_entry);
				if (ret==FAILURE) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Writeback Failed to Update IQ Entry (%d) for pc(%d):: %.5s\n", ret, stage->pc, stage->opcode);
					}
				}

				ret = update_ls_queue_entry_reg(ls_queue, ls_iq_entry);
				if (ret==FAILURE) {
					if (ENABLE_DEBUG_MESSAGES_L2) {
						fprintf(stderr, "Writeback Nothing to Update in LSQ Entry (%d) for pc(%d):: %.5s\n", ret, stage->pc, stage->opcode);
					}
				}
				// Also update DRF regs so next they can be dispatched
				switch (cpu->stage[DRF].inst_type) {
					// check single src reg instructions
					case STORE: case LOAD: case MOV: case ADDL: case SUBL: case JUMP:
						if ((cpu->stage[DRF].rs1==stage->rd)&&(cpu->stage[DRF].rs1_valid==INVALID)) {
							cpu->stage[DRF].rs1_value = stage->rd_value;
							cpu->stage[DRF].rs1_valid = stage->rd_valid;
						}
						break;
					// check two src reg instructions
					case STR: case LDR: case ADD: case SUB: case MUL: case DIV: case AND: case OR: case EXOR:
						if ((cpu->stage[DRF].rs1==stage->rd)&&(cpu->stage[DRF].rs1_valid==INVALID)) {
							cpu->stage[DRF].rs1_value = stage->rd_value;
							cpu->stage[DRF].rs1_valid = stage->rd_valid;
						}
						if ((cpu->stage[DRF].rs2==stage->rd)&&(cpu->stage[DRF].rs2_valid==INVALID)) {
							cpu->stage[DRF].rs2_value = stage->rd_value;
							cpu->stage[DRF].rs2_valid = stage->rd_valid;
						}
						break;
					// confirm if for Store we need to read all three src reg
					default:
						break;
				}
			}
		}
		else {
			if (ENABLE_DEBUG_MESSAGES_L2) {
				fprintf(stderr, "Writeback for Stage %d Not Ready to Process\n", cpu_stages[i]);
			}
		}
	}

	return 0;
}


/*
 * ########################################## Dispatch Stage ##########################################
*/
int dispatch_instruction(APEX_CPU* cpu, APEX_LSQ* ls_queue, APEX_IQ* issue_queue, APEX_ROB* rob, APEX_RENAME* rename_table){
	// check if ISQ entry is free and ROB entry is free then dispatch the instruction
	CPU_Stage* stage = &cpu->stage[DRF];
	if (stage->executed) {
		int ret = 0;
		int lsq_index = -1;

		LS_IQ_Entry ls_iq_entry = {
			.inst_type = stage->inst_type,
			.executed = stage->executed,
			.pc = stage->pc,
			.rd = stage->rd,
			.rd_value = stage->rd_value,
			.rd_valid = stage->rd_valid,
			.rs1 = stage->rs1,
			.rs1_value = stage->rs1_value,
			.rs1_valid = stage->rs1_valid,
			.rs2 = stage->rs2,
			.rs2_value = stage->rs2_value,
			.rs2_valid = stage->rs2_valid,
			.buffer = stage->buffer,
			.stage_cycle = INVALID}; // so that which issue is called it stalls this just added inst for at least 1 cycyle

		ROB_Entry rob_entry = {
			.inst_type = stage->inst_type,
			.executed = stage->executed,
			.pc = stage->pc,
			.rd = stage->rd,
			.rd_value = stage->rd_value,
			.rd_valid = stage->rd_valid,
			.rs1 = stage->rs1,
			.rs1_value = stage->rs1_value,
			.rs1_valid = stage->rs1_valid,
			.rs2 = stage->rs2,
			.rs2_value = stage->rs2_value,
			.rs2_valid = stage->rs2_valid,
			.exception = INVALID,
			.buffer = stage->buffer,
			.stage_cycle = INVALID};

		switch (stage->inst_type) {
			case STORE: case STR: case LOAD: case LDR:
				// add entry to LSQ and ROB
				// check if LSQ entry is available and rob entry is available
				if ((can_add_entry_in_issue_queue(issue_queue)==SUCCESS)&&(can_add_entry_in_ls_queue(ls_queue)==SUCCESS)&&(can_add_entry_in_reorder_buffer(rob)==SUCCESS)) {
					ret = add_ls_queue_entry(ls_queue, ls_iq_entry, &lsq_index);
					if(ret==SUCCESS) {
						ret = add_issue_queue_entry(issue_queue, ls_iq_entry, &lsq_index);
						ret = add_reorder_buffer_entry(rob, rob_entry);
					}
					else {
						if (ENABLE_DEBUG_MESSAGES_L2) {
							fprintf(stderr, "LSQ IQ ROB Eentry Failed for Inst Type :: %d\n", stage->inst_type);
						}
					}
				}
				else{
					// stall DRF and Fetch stage
					cpu->stage[DRF].stalled = VALID;
					cpu->stage[F].stalled = VALID;
				}
				break;

			case MOVC ... JUMP:
				// add entry to ISQ and ROB
				// check if IQ entry is available and rob entry is available
				if ((can_add_entry_in_issue_queue(issue_queue)==SUCCESS)&&(can_add_entry_in_reorder_buffer(rob)==SUCCESS)) {
					ret = add_issue_queue_entry(issue_queue, ls_iq_entry, &lsq_index);
					ret = add_reorder_buffer_entry(rob, rob_entry);
					if(ret!=SUCCESS) {
						if (ENABLE_DEBUG_MESSAGES_L2) {
							fprintf(stderr, "IQ ROB Eentry Failed for Inst Type :: %d\n", stage->inst_type);
						}
					}
				}
				else{
					// stall DRF and Fetch stage
					cpu->stage[DRF].stalled = VALID;
					cpu->stage[F].stalled = VALID;
				}
				break;

			case HALT:
				// add entry to ROB
				if (can_add_entry_in_reorder_buffer(rob)==SUCCESS) {
					ret = add_reorder_buffer_entry(rob, rob_entry);
					if(ret!=SUCCESS) {
						if (ENABLE_DEBUG_MESSAGES_L2) {
							fprintf(stderr, "ROB Eentry Failed for Inst Type :: %d\n", stage->inst_type);
						}
					}
				}
				break;

			default:
				break;
		}
	}
	else {
		if (!stage->empty){
			if (ENABLE_DEBUG_MESSAGES_L2) {
				fprintf(stderr, "Dispatch Failed DRF has non executed instruction :: pc(%d) %s\n", stage->pc, stage->opcode);
			}
		}
	}

	return 0;
}


/*
 * ########################################## Issue Stage ##########################################
*/
int issue_instruction(APEX_CPU* cpu, APEX_IQ* issue_queue, APEX_LSQ* ls_queue) {
	// check if respective FU is free and All Regs Value are Valid then issue the instruction
	// get issue_index of all the instruction
	int ret = 0;
	int issue_index[IQ_SIZE] = {[0 ... IQ_SIZE-1] = -1};
	ret = get_issue_queue_index_to_issue(issue_queue, issue_index);
	if (ret==SUCCESS) {
		char* inst_type_str = (char*) malloc(10);
		for (int i=0; i<IQ_SIZE; i++) {
			if (issue_index[i]>-1) {
				int stage_num = -1;

				switch (issue_queue->iq_entries[issue_index[i]].inst_type) {

					case STORE: case STR: case LOAD: case LDR: case MOVC: case MOV: case ADD: case ADDL: case SUB: case SUBL: case DIV: case AND: case OR: case EXOR:
						stage_num = INT_ONE;
						break;

					case MUL:
						stage_num = MUL_ONE;
						break;

					case BZ: case BNZ:
						stage_num = BRANCH;
						break;

					default:
						break;
				}
				if (stage_num>DRF) {
					CPU_Stage* stage = &cpu->stage[stage_num];
					if ((stage->executed)||(stage->empty)) {
						strcpy(inst_type_str, "");
						stage->executed = INVALID;
						stage->empty = INVALID;
						stage->inst_type = issue_queue->iq_entries[issue_index[i]].inst_type;
						get_inst_name(stage->inst_type, inst_type_str);
						strcpy(stage->opcode, inst_type_str);
						stage->pc = issue_queue->iq_entries[issue_index[i]].inst_ptr;
						stage->rd = issue_queue->iq_entries[issue_index[i]].rd;
						stage->rd_value = issue_queue->iq_entries[issue_index[i]].rd_value;
						if ((stage->inst_type==STORE)||(stage->inst_type==STR)) {
							stage->rd_valid = issue_queue->iq_entries[issue_index[i]].rd_ready;;
						}
						else {
							stage->rd_valid = INVALID;
						}
						stage->rs1 = issue_queue->iq_entries[issue_index[i]].rs1;
						stage->rs1_value = issue_queue->iq_entries[issue_index[i]].rs1_value;
						stage->rs1_valid = issue_queue->iq_entries[issue_index[i]].rs1_ready;
						stage->rs2 = issue_queue->iq_entries[issue_index[i]].rs2;
						stage->rs2_value = issue_queue->iq_entries[issue_index[i]].rs2_value;
						stage->rs2_valid = issue_queue->iq_entries[issue_index[i]].rs2_ready;
						stage->buffer = issue_queue->iq_entries[issue_index[i]].literal;
						stage->lsq_index = issue_queue->iq_entries[issue_index[i]].lsq_index;
						// remove the entry from issue_queue or mark it as invalid
						issue_queue->iq_entries[issue_index[i]].status = INVALID;
						issue_queue->iq_entries[issue_index[i]].inst_type = INVALID;
						issue_queue->iq_entries[issue_index[i]].inst_ptr = INVALID;
						issue_queue->iq_entries[issue_index[i]].rd = INVALID;
						issue_queue->iq_entries[issue_index[i]].rd_value = INVALID;
						issue_queue->iq_entries[issue_index[i]].rd_ready = INVALID;
						issue_queue->iq_entries[issue_index[i]].rs1 = INVALID;
						issue_queue->iq_entries[issue_index[i]].rs1_value = INVALID;
						issue_queue->iq_entries[issue_index[i]].rs1_ready = INVALID;
						issue_queue->iq_entries[issue_index[i]].rs2 = INVALID;
						issue_queue->iq_entries[issue_index[i]].rs2_value = INVALID;
						issue_queue->iq_entries[issue_index[i]].rs2_ready = INVALID;
						issue_queue->iq_entries[issue_index[i]].literal = INVALID;
						issue_queue->iq_entries[issue_index[i]].lsq_index = INVALID;
						issue_queue->iq_entries[issue_index[i]].stage_cycle = INVALID;
					}
				}
				if (ENABLE_DEBUG_MESSAGES_L2) {
					fprintf(stderr, "Inst issueed to Unit :: %d\n", stage_num);
				}
			}
		}
		free(inst_type_str);
	}
	else {
		if (ENABLE_DEBUG_MESSAGES_L2) {
			fprintf(stderr, "No Inst to issue to Unit\n");
		}
	}

	int lsq_index = -1;
	ret = get_ls_queue_index_to_issue(ls_queue, &lsq_index);
	if (ret==SUCCESS) {
		char* inst_type_str = (char*) malloc(10);
		CPU_Stage* stage = &cpu->stage[MEM];
		if ((stage->executed)||(stage->empty)) {
			strcpy(inst_type_str, "");
			stage->executed = INVALID;
			stage->empty = INVALID;
			stage->inst_type = ls_queue->lsq_entries[lsq_index].load_store;
			get_inst_name(stage->inst_type, inst_type_str);
			strcpy(stage->opcode, inst_type_str);
			stage->pc = ls_queue->lsq_entries[lsq_index].inst_ptr;
			stage->rd = ls_queue->lsq_entries[lsq_index].rd;
			stage->rd_value = ls_queue->lsq_entries[lsq_index].rd_value;
			stage->rd_valid = ls_queue->lsq_entries[lsq_index].data_ready;
			stage->rs1 = ls_queue->lsq_entries[lsq_index].rs1;
			stage->rs1_value = ls_queue->lsq_entries[lsq_index].rs1_value;
			stage->rs1_valid = VALID;
			stage->rs2 = ls_queue->lsq_entries[lsq_index].rs2;
			stage->rs2_value = ls_queue->lsq_entries[lsq_index].rs2_value;
			stage->rs2_valid = VALID;
			stage->buffer = ls_queue->lsq_entries[lsq_index].literal;
			stage->mem_address = ls_queue->lsq_entries[lsq_index].mem_address;

			// remove the entry from issue_queue or mark it as invalid
			ls_queue->lsq_entries[lsq_index].status = INVALID;
			ls_queue->lsq_entries[lsq_index].load_store = INVALID;
			ls_queue->lsq_entries[lsq_index].inst_ptr = INVALID;
			ls_queue->lsq_entries[lsq_index].mem_valid = INVALID;
			ls_queue->lsq_entries[lsq_index].rd = INVALID;
			ls_queue->lsq_entries[lsq_index].rd_value = INVALID;
			ls_queue->lsq_entries[lsq_index].data_ready = INVALID;
			ls_queue->lsq_entries[lsq_index].literal = INVALID;
			ls_queue->lsq_entries[lsq_index].rs1 = INVALID;
			ls_queue->lsq_entries[lsq_index].rs1_value = INVALID;
			ls_queue->lsq_entries[lsq_index].rs2 = INVALID;
			ls_queue->lsq_entries[lsq_index].rs2_value = INVALID;
			ls_queue->lsq_entries[lsq_index].stage_cycle = INVALID;
		}
		else {
			if (ENABLE_DEBUG_MESSAGES_L2) {
				fprintf(stderr, "Cannot Issue Inst To Mem Stage\n");
			}
		}
	}

	return 0;
}


/*
 * ########################################## Execute Stage ##########################################
*/
int execute_instruction(APEX_CPU* cpu, APEX_LSQ* ls_queue, APEX_IQ* issue_queue, APEX_ROB* rob, APEX_RENAME* rename_table) {
	// check if respective FU has any instructions and execute them
	// call each unit one by one
	// branch will be called last idk y ?
	int_one_stage(cpu, rename_table);
	int_two_stage(cpu);
	mul_one_stage(cpu, rename_table);
	mul_two_stage(cpu);
	mul_three_stage(cpu);
	branch_stage(cpu);
	mem_stage(cpu);

	writeback_stage(cpu, ls_queue, issue_queue, rob, rename_table);

	return 0;
}


/*
 * ########################################## Branch Misprediction Stage ##########################################
*/
void branch_misprediction(APEX_CPU* cpu, ROB_Entry* rob_entry, APEX_ROB* rob, APEX_LSQ* ls_queue, APEX_IQ* issue_queue, APEX_RENAME* rename_table) {

	clear_rename_table(rename_table);
	clear_reorder_buffer(rob);

	clear_issue_queue_entry(issue_queue);
	clear_ls_queue_entry(ls_queue);

	// clear all the cpu stages
	// change pc and flush F DRF
	clear_stage_entry(cpu, DRF);
	clear_stage_entry(cpu, F);
	// stall F so it wont fetch in same cycle
	cpu->stage[F].stalled = VALID;

}

/*
 * ########################################## Commit Stage ##########################################
*/
int commit_instruction(APEX_CPU* cpu, APEX_LSQ* ls_queue, APEX_IQ* issue_queue, APEX_ROB* rob, APEX_RENAME* rename_table) {
	// check if rob entry is valid and data is valid then commit instruction and free rob entry
	int ret = -1;

	ROB_Entry* rob_entry = malloc(sizeof(*rob_entry));
	// entry removed from rob
	ret = commit_reorder_buffer_entry(rob, rob_entry);

	if (ret==SUCCESS) {
		if ((rob_entry->inst_type==STORE)||(rob_entry->inst_type==STR)) {
			; // no need to free regs or pass rd value
		}
		else if (rob_entry->inst_type==JUMP) {
			; // no need to free regs or pass rd value
		}
		else if (rob_entry->inst_type==BZ) {
			// no need to free regs or pass rd value
			if ((cpu->flags[ZF]==VALID)&&(rob_entry->exception)) {
				// branch misspredicted
				// revert the changes
				branch_misprediction(cpu, rob_entry, rob, ls_queue, issue_queue, rename_table);
				// pc of branch inst plus 4 will be nex pc
				cpu->pc = rob_entry->pc + 4;
			}
		}
		else if (rob_entry->inst_type==BNZ) {
			// no need to free regs or pass rd value
			if ((cpu->flags[ZF]==INVALID)&&(rob_entry->exception)) {
				// branch misspredicted
				// revert the changes
				branch_misprediction(cpu, rob_entry, rob, ls_queue, issue_queue, rename_table);
				// pc of branch inst plus 4 will be nex pc
				cpu->pc = rob_entry->pc + 4;
			}
		}
		else if (rob_entry->inst_type==HALT) {
			// exit the simulation
			return HALT;
		}
		else {
			// also update IQ here as well
			LS_IQ_Entry ls_iq_entry = {
				.inst_type = rob_entry->inst_type,
				.executed = rob_entry->executed,
				.pc = rob_entry->pc,
				.rd = rob_entry->rd,
				.rd_value = rob_entry->rd_value,
				.rd_valid = rob_entry->rd_valid,
				.rs1 = rob_entry->rs1,
				.rs1_value = rob_entry->rs1_value,
				.rs1_valid = rob_entry->rs1_valid,
				.rs2 = rob_entry->rs2,
				.rs2_value = rob_entry->rs2_value,
				.rs2_valid = rob_entry->rs2_valid,
				.buffer = rob_entry->buffer,
				.stage_cycle = rob_entry->stage_cycle};

			ret = update_issue_queue_entry(issue_queue, ls_iq_entry);
			if (ret==FAILURE) {
				if (ENABLE_DEBUG_MESSAGES_L2) {
					fprintf(stderr, "Commit Failed to Update IQ Entry (%d) for pc(%d)\n", ret, ls_iq_entry.pc);
				}
			}

			ret = update_ls_queue_entry_reg(ls_queue, ls_iq_entry);
			if (ret==FAILURE) {
				if (ENABLE_DEBUG_MESSAGES_L2) {
					fprintf(stderr, "Commit Nothing to Update in LSQ Entry (%d) for pc(%d)\n", ret, ls_iq_entry.pc);
				}
			}

			// put the value in DRF
			switch (cpu->stage[DRF].inst_type) {
				// check single src reg instructions
				case STORE: case LOAD: case MOV: case ADDL: case SUBL: case JUMP:
				if ((cpu->stage[DRF].rs1==rob_entry->rd)&&(cpu->stage[DRF].rs1_valid==INVALID)) {
					cpu->stage[DRF].rs1_value = rob_entry->rd_value;
					cpu->stage[DRF].rs1_valid = rob_entry->rd_valid;
				}
				break;
				// check two src reg instructions
				case STR: case LDR: case ADD: case SUB: case MUL: case DIV: case AND: case OR: case EXOR:
				if ((cpu->stage[DRF].rs1==rob_entry->rd)&&(cpu->stage[DRF].rs1_valid==INVALID)) {
					cpu->stage[DRF].rs1_value = rob_entry->rd_value;
					cpu->stage[DRF].rs1_valid = rob_entry->rd_valid;
				}
				if ((cpu->stage[DRF].rs2==rob_entry->rd)&&(cpu->stage[DRF].rs2_valid==INVALID)) {
						cpu->stage[DRF].rs2_value = rob_entry->rd_value;
						cpu->stage[DRF].rs2_valid = rob_entry->rd_valid;
				}
				break;
				// confirm if for Store we need to read all three src reg
				default:
				break;
			}
		// change decs reg in cpu
		// undo the renaming
		// put the value in arch reg and increment valid valid count
		int src_reg = -1;
		src_reg = get_phy_reg_renamed_tag(rob_entry->rd, rename_table);
		if (src_reg<0) {
			if (ENABLE_DEBUG_MESSAGES_L2) {
				fprintf(stderr, "Commit Failed to Rename :: P%d for pc(%d)\n", rob_entry->rd, rob_entry->pc);
			}
		}
		else {
			// free the physical regs
			cpu->regs[src_reg] = rob_entry->rd_value;
			rename_table->reg_rename[rob_entry->rd].tag_valid = INVALID;
			set_reg_status(cpu, src_reg, -1);
			}
		}
	}
	else {
		printf("Failed to Commit Rob Entry\n");
	}

	return 0;
}

/*
 * ########################################## CPU Run ##########################################
*/

int APEX_cpu_run(APEX_CPU* cpu, int num_cycle, APEX_LSQ* ls_queue, APEX_IQ* issue_queue, APEX_ROB* rob, APEX_RENAME* rename_table) {

	int ret = 0;

	while (ret==0) {

		/* Requested number of cycle committed, so pause and exit */
		if ((num_cycle>0)&&(cpu->clock == num_cycle)) {
			printf("\n--------------------------------\n");
			printf("Requested %d Cycle Completed", num_cycle);
			printf("\n--------------------------------\n");
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
			// commit inst
			stage_ret = commit_instruction(cpu, ls_queue, issue_queue, rob, rename_table);
			if (stage_ret==HALT) {
				ret = stage_ret;
			}
			// adding inst to FU
			stage_ret = issue_instruction(cpu, issue_queue, ls_queue);
			// executing inst
			stage_ret = execute_instruction(cpu, ls_queue, issue_queue, rob, rename_table);
			// adding inst to IQ, LSQ, ROB
			stage_ret = dispatch_instruction(cpu, ls_queue, issue_queue, rob, rename_table);
			// push only before IQ Stages
			push_func_unit_stages(cpu, INVALID);
			// only renaming happens
			stage_ret = decode(cpu, rename_table);
			// after renaming is done just fetch the values from CPU OUTPUT Stages
			// and update the DRF stage so while dispatching dependencies are handled
			// fetch inst from code memory
			stage_ret = fetch(cpu);
			// dispatch func will have rename call inside
			print_ls_iq_content(ls_queue, issue_queue);
			print_rob_and_rename_content(rob, rename_table);

			// push only after IQ Stages
			push_func_unit_stages(cpu, VALID);

			// if ((stage_ret!=HALT)&&(stage_ret!=SUCCESS)) {
			// 	ret = stage_ret;
			// }
		}
	}

	return ret;
}
