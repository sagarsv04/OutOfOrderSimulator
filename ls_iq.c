/*
 *  ls_iq.h
 *  Contains APEX LSQ and IQ implementation
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ls_iq.h"
#include "cpu.h"
#include "forwarding.h"

/*
 * ########################################## Issue Queue ##########################################
*/

// issue_queue(cpu);

/*  Issue:
 *  Get ROB entry, Point RAT to rob ROB entry
*/

/*  Dispatch:
 *  If ready ? : send inst to execute
 *  Get ROB entry, Point RAT to rob ROB entry
*/

APEX_IQ* init_issue_queue() {

	APEX_IQ* issue_queue = malloc(sizeof(*issue_queue));

	if (!issue_queue) {
		return NULL;
	}

	memset(issue_queue->iq_entries, 0, sizeof(IQ_FORMAT)*IQ_SIZE);  // all issue entry set to 0

	return issue_queue;
}

void deinit_issue_queue(APEX_IQ* issue_queue) {
	free(issue_queue);
}

int can_add_entry_in_issue_queue(APEX_IQ* issue_queue) {
	int add_position = -1;
	for (int i=0; i<IQ_SIZE; i++) {
		if (issue_queue->iq_entries[i].status == INVALID) {
			add_position = i;
			break;
		}
	}
	if (add_position<0) {
		return FAILURE;
	}
	else {
		return SUCCESS;
	}
}

int add_issue_queue_entry(APEX_IQ* issue_queue, LS_IQ_Entry ls_iq_entry, int* lsq_index) {
	// if instruction sucessfully added then only pass the instruction to function units
	int add_position = -1;
	if (!ls_iq_entry.executed) {
		return FAILURE;
	}
	else {
		for (int i=0; i<IQ_SIZE; i++) {
			if (issue_queue->iq_entries[i].status == INVALID) {
				add_position = i;
				break;
			}
		}
		if (add_position<0) {
			return FAILURE;
		}
		else {
			issue_queue->iq_entries[add_position].status = VALID;
			issue_queue->iq_entries[add_position].inst_type = ls_iq_entry.inst_type;
			issue_queue->iq_entries[add_position].inst_ptr = ls_iq_entry.pc;
			issue_queue->iq_entries[add_position].rd = ls_iq_entry.rd;
			issue_queue->iq_entries[add_position].rd_value = ls_iq_entry.rd_value;
			issue_queue->iq_entries[add_position].rd_ready = ls_iq_entry.rd_valid;
			issue_queue->iq_entries[add_position].rs1 = ls_iq_entry.rs1;
			issue_queue->iq_entries[add_position].rs1_value = ls_iq_entry.rs1_value;
			issue_queue->iq_entries[add_position].rs1_ready = ls_iq_entry.rs1_valid;
			issue_queue->iq_entries[add_position].rs2 = ls_iq_entry.rs2;
			issue_queue->iq_entries[add_position].rs2_value = ls_iq_entry.rs2_value;
			issue_queue->iq_entries[add_position].rs2_ready = ls_iq_entry.rs2_valid;
			issue_queue->iq_entries[add_position].literal = ls_iq_entry.buffer;
			issue_queue->iq_entries[add_position].stage_cycle = INVALID;
			issue_queue->iq_entries[add_position].lsq_index = *lsq_index;
		}
	}
	return SUCCESS;
}


int update_issue_queue_entry(APEX_IQ* issue_queue, LS_IQ_Entry ls_iq_entry) {
	// this is updating values from func unit to any entry which is waiting in queue
	// array to hold respective reg update positions
	// int rs1_position[IQ_SIZE] = {-1}; // these can use to see where values got updated
	// int rs2_position[IQ_SIZE] = {-1};
	int update_pos_sum = 0;
	if (!ls_iq_entry.executed) {
		return FAILURE;
	}
	else {
		// for now loop through entire issue_queue and check
		for (int i=0; i<IQ_SIZE; i++) {
			// check only alloted entries
			if (issue_queue->iq_entries[i].status == VALID) {
				// first check rd ie flow and output dependencies
				if ((ls_iq_entry.rd == issue_queue->iq_entries[i].rs1)&&(issue_queue->iq_entries[i].rs1_ready==INVALID)) {
					issue_queue->iq_entries[i].rs1_value = ls_iq_entry.rd_value;
					issue_queue->iq_entries[i].rs1_ready = VALID;
					// rs1_position[i] = i;
					update_pos_sum += 1;
				}
				if ((ls_iq_entry.rd == issue_queue->iq_entries[i].rs2)&&(issue_queue->iq_entries[i].rs1_ready==INVALID)) {
					issue_queue->iq_entries[i].rs2_value = ls_iq_entry.rd_value;
					issue_queue->iq_entries[i].rs2_ready = VALID;
					// rs2_position[i] = i;
					update_pos_sum += 1;
				}
				// this for updating STORE STR Rd
				if ((issue_queue->iq_entries[i].inst_type==STORE)||(issue_queue->iq_entries[i].inst_type==STR)) {
					if ((ls_iq_entry.rd == issue_queue->iq_entries[i].rd)&&(issue_queue->iq_entries[i].rd_ready==INVALID)) {
						issue_queue->iq_entries[i].rd_value = ls_iq_entry.rd_value;
						issue_queue->iq_entries[i].rd_ready = VALID;
						// rs2_position[i] = i;
						update_pos_sum += 1;
					}
				}
			}
		}
	}
	if (update_pos_sum == 0) {
		if (ENABLE_DEBUG_MESSAGES_L2) {
			fprintf(stderr, "No Update in IQ for Inst %d at pc(%d)\n", ls_iq_entry.inst_type, ls_iq_entry.pc);
		}
	}
	return SUCCESS;
}


int get_issue_queue_index_to_issue(APEX_IQ* issue_queue, int* issue_index) {
	// get all the index of issue_queue entries
	// in cpu loop through index and issue instructions to respective func units
	// int issue_index[IQ_SIZE] = {-1}; // these can use to see where values got updated
	int index_sum = 0;
	int stage_cycle_array[IQ_SIZE] = {[0 ... IQ_SIZE-1] = -1};
	for (int i=0; i<IQ_SIZE; i++) {
		// check only alloted entries
		if (issue_queue->iq_entries[i].status == VALID) {
			// check if instructions have been in IQ for atleast one cycle
			// if (issue_queue->iq_entries[i].stage_cycle>0) {  // no need if we are executing in reverse
			switch (issue_queue->iq_entries[i].inst_type) {

				// check no src reg instructions
				case MOVC: case BZ: case BNZ:
					issue_index[i] = i;
					index_sum += 1;
					break;

				// check single src reg instructions
				case LOAD: case MOV: case ADDL: case SUBL: case JUMP:
					if (issue_queue->iq_entries[i].rs1_ready) {
						issue_index[i] = i;
						index_sum += 1;
					}
					break;

				// check single src/desc reg instructions
				case STORE:
					if ((issue_queue->iq_entries[i].rs1_ready)&&(issue_queue->iq_entries[i].rd_ready)) {
						issue_index[i] = i;
						index_sum += 1;
					}
					break;

				// check two src reg instructions
				case LDR: case ADD: case SUB: case MUL: case DIV: case AND: case OR: case EXOR:
					if ((issue_queue->iq_entries[i].rs1_ready)&&(issue_queue->iq_entries[i].rs2_ready)) {
						issue_index[i] = i;
						index_sum += 1;
					}
					break;

				// check double src and desc reg instructions
				case STR:
					if ((issue_queue->iq_entries[i].rs1_ready)&&(issue_queue->iq_entries[i].rs2_ready)&&(issue_queue->iq_entries[i].rd_ready)) {
						issue_index[i] = i;
						index_sum += 1;
					}
					break;
				// confirm if for Store we need to check all three src reg before issuing
				// or the src reg (desc rd) is checked in LSQ before its issueed from LSQ to Mem stage
				default:
					break;
			}
			// even if inst is invalid inc stage cycle to know how long inst has been in IQ
			issue_queue->iq_entries[i].stage_cycle += 1;
			stage_cycle_array[i] = issue_queue->iq_entries[i].stage_cycle;
		}
	}
	if (index_sum == 0) {
		return FAILURE;
	}
	else {
		int i,j,temp;
		for(i=0; i<IQ_SIZE; i++)
		{
			for(j=i+1; j<IQ_SIZE; j++)
			{
				if(stage_cycle_array[i] < stage_cycle_array[j])
				{
					temp = issue_index[i];
					issue_index[i] = issue_index[j];
					issue_index[j] = temp;
				}
			}
		}
	}

	return SUCCESS;
}


/*
 * ########################################## Load Store Queue ##########################################
*/


APEX_LSQ* init_ls_queue() {

	APEX_LSQ* ls_queue = malloc(sizeof(*ls_queue));

	if (!ls_queue) {
		return NULL;
	}

	memset(ls_queue->lsq_entries, 0, sizeof(LSQ_FORMAT)*LSQ_SIZE);  // all issue entry set to 0

	return ls_queue;
}

void deinit_ls_queue(APEX_LSQ* ls_queue) {
	free(ls_queue);
}


int can_add_entry_in_ls_queue(APEX_LSQ* ls_queue) {
	int add_position = -1;
	for (int i=0; i<IQ_SIZE; i++) {
		if (ls_queue->lsq_entries[i].status == INVALID) {
			add_position = i;
			break;
		}
	}
	if (add_position<0) {
		return FAILURE;
	}
	else {
		return SUCCESS;
	}
}


int add_ls_queue_entry(APEX_LSQ* ls_queue, LS_IQ_Entry ls_iq_entry, int* lsq_index) {
	// if instruction sucessfully added then only pass the instruction to function units
	int add_position = -1;

	for (int i=0; i<LSQ_SIZE; i++) {
		if (ls_queue->lsq_entries[i].status == INVALID) {
			add_position = i;
			break;
		}
	}
	if (add_position<0) {
		return FAILURE;
	}
	else {
		*lsq_index = add_position;
		ls_queue->lsq_entries[add_position].status = VALID;
		ls_queue->lsq_entries[add_position].load_store = ls_iq_entry.inst_type;
		ls_queue->lsq_entries[add_position].inst_ptr = ls_iq_entry.pc;
		ls_queue->lsq_entries[add_position].rd = ls_iq_entry.rd;
		ls_queue->lsq_entries[add_position].rd_value = ls_iq_entry.rd_value;
		ls_queue->lsq_entries[add_position].rs1 = ls_iq_entry.rs1;
		ls_queue->lsq_entries[add_position].rs1_value = ls_iq_entry.rs1_value;
		ls_queue->lsq_entries[add_position].rs2 = ls_iq_entry.rs2;
		ls_queue->lsq_entries[add_position].rs2_value = ls_iq_entry.rs2_value;
		ls_queue->lsq_entries[add_position].literal = ls_iq_entry.buffer;
		ls_queue->lsq_entries[add_position].mem_valid = INVALID;
		if ((ls_iq_entry.inst_type==STORE)||(ls_iq_entry.inst_type==STR)) {
			ls_queue->lsq_entries[add_position].data_ready = ls_iq_entry.rd_valid;
		}
		else {
			ls_queue->lsq_entries[add_position].data_ready = INVALID;
		}
		ls_queue->lsq_entries[add_position].stage_cycle = INVALID;
	}
	return SUCCESS;
}


int update_ls_queue_entry_mem_address(APEX_LSQ* ls_queue, LS_IQ_Entry ls_iq_entry) {

	if (ls_iq_entry.lsq_index<0) {
		return FAILURE;
	}
	else {
		// same inst data can be copied
		if (ls_queue->lsq_entries[ls_iq_entry.lsq_index].inst_ptr == ls_iq_entry.pc) {
			ls_queue->lsq_entries[ls_iq_entry.lsq_index].mem_address = ls_iq_entry.mem_address;
			ls_queue->lsq_entries[ls_iq_entry.lsq_index].mem_valid = VALID;
			if (ls_queue->lsq_entries[ls_iq_entry.lsq_index].rd==ls_iq_entry.rd) {
				ls_queue->lsq_entries[ls_iq_entry.lsq_index].rd_value = ls_iq_entry.rd_value;
				ls_queue->lsq_entries[ls_iq_entry.lsq_index].data_ready = VALID;
			}
		}
	}

	return SUCCESS;
}


int update_ls_queue_entry_reg(APEX_LSQ* ls_queue, LS_IQ_Entry ls_iq_entry) {

	int update_pos = -1;

	for (int i=0; i<LSQ_SIZE; i++) {
		if (ls_queue->lsq_entries[i].status == VALID) {
			if ((ls_queue->lsq_entries[i].load_store==STORE)||(ls_queue->lsq_entries[i].load_store==STR)) {
				if (ls_queue->lsq_entries[i].data_ready==INVALID) {
					if (ls_queue->lsq_entries[i].rd==ls_iq_entry.rd) {
						ls_queue->lsq_entries[i].rd_value = ls_iq_entry.rd_value;
						ls_queue->lsq_entries[i].data_ready = VALID;
						update_pos = i;
					}
				}
			}
		}
	}

	if (update_pos<0) {
		return FAILURE;
	}
	return SUCCESS;
}


int get_ls_queue_index_to_issue(APEX_LSQ* ls_queue, int* lsq_index) {

	int index = -1;
	int prev_index = -1;
	int cycle = 0;
	int prev_cycle = 0;

	for (int i=0; i<LSQ_SIZE; i++) {

		if (ls_queue->lsq_entries[i].status == VALID) {
			if ((ls_queue->lsq_entries[i].load_store==STORE)||(ls_queue->lsq_entries[i].load_store==STR)) {
				if ((ls_queue->lsq_entries[i].mem_valid)&&(ls_queue->lsq_entries[i].data_ready)) {
					index = i;
					cycle = ls_queue->lsq_entries[i].stage_cycle;
				}
			}
			else {
				if (ls_queue->lsq_entries[i].mem_valid) {
					index = i;
					cycle = ls_queue->lsq_entries[i].stage_cycle;
				}
			}

		if (cycle > prev_cycle) {
			prev_cycle = cycle;
			prev_index = index;
		}
		ls_queue->lsq_entries[i].stage_cycle += 1;
		}
	}

	if (prev_index<0) {
		return FAILURE;
	}
	else {
		*lsq_index = prev_index;
	}
	return SUCCESS;
}



void print_ls_iq_content(APEX_LSQ* ls_queue, APEX_IQ* issue_queue) {
	// if require to print the instruction instead of type just call the func fron cpu to print inst
	if (ENABLE_REG_MEM_STATUS_PRINT) {
		char* inst_type_str = (char*) malloc(10);
		printf("\n============ STATE OF ISSUE QUEUE ============\n");
		printf("Index, "
						"Status, "
						"Type, "
						"OpCode, "
						"Rd-value, "
						"Rs1-value-ready, "
						"Rs2-value-ready, "
						"literal, "
						"LSQ Index\n");
		for (int i=0;i<IQ_SIZE;i++) {
			strcpy(inst_type_str, "");
			get_inst_name(issue_queue->iq_entries[i].inst_type, inst_type_str);
			printf("%02d\t|"
							"\t%d\t|"
							"\t%d\t|"
							"\t%.5s\t|"
							"\tR%02d-%d-%d\t|"
							"\tR%02d-%d-%d\t|"
							"\tR%02d-%d-%d\t|"
							"\t#%02d\t|"
							"\t%02d\n",
							i,
							issue_queue->iq_entries[i].status,
							issue_queue->iq_entries[i].inst_type,
							inst_type_str,
							issue_queue->iq_entries[i].rd, issue_queue->iq_entries[i].rd_value, issue_queue->iq_entries[i].rd_ready,
							issue_queue->iq_entries[i].rs1, issue_queue->iq_entries[i].rs1_value, issue_queue->iq_entries[i].rs1_ready,
							issue_queue->iq_entries[i].rs2, issue_queue->iq_entries[i].rs2_value, issue_queue->iq_entries[i].rs2_ready,
							issue_queue->iq_entries[i].literal,
							issue_queue->iq_entries[i].lsq_index);
		}
		printf("\n============ STATE OF LOAD STORE QUEUE ============\n");
		printf("Index, "
						"Status, "
						"Type, "
						"OpCode, "
						"Mem Valid, "
						"Data Ready, "
						"Rd-value, "
						"Rs1-value, "
						"Rs2-value, "
						"literal\n");
		for (int i=0;i<LSQ_SIZE;i++) {
			strcpy(inst_type_str, "");
			get_inst_name(ls_queue->lsq_entries[i].load_store, inst_type_str);
			printf("%02d\t|"
							"\t%d\t|"
							"\t%d\t|"
							"\t%.5s\t|"
							"\t%d\t|"
							"\t%d\t|"
							"\tR%02d-%d\t|"
							"\tR%02d-%d\t|"
							"\tR%02d-%d\t|"
							"\t#%02d\n",
							i,
							ls_queue->lsq_entries[i].status,
							ls_queue->lsq_entries[i].load_store,
							inst_type_str,
							ls_queue->lsq_entries[i].mem_valid,
							ls_queue->lsq_entries[i].data_ready,
							ls_queue->lsq_entries[i].rd, ls_queue->lsq_entries[i].rd_value,
							ls_queue->lsq_entries[i].rs1, ls_queue->lsq_entries[i].rs1_value,
							ls_queue->lsq_entries[i].rs2, ls_queue->lsq_entries[i].rs2_value,
							ls_queue->lsq_entries[i].literal);

		}
		free(inst_type_str);
	}
}
