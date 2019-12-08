/*
 *  rob.c
 *  Contains APEX reorder buffer implementation
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rob.h"
#include "cpu.h"
#include "forwarding.h"

/*
 * ########################################## Reorder Buffer Stage ##########################################
*/

APEX_ROB* init_reorder_buffer() {

	APEX_ROB* rob = malloc(sizeof(*rob));
	if (!rob) {
		return NULL;
	}

	memset(rob->rob_entry, 0, sizeof(APEX_ROB_ENTRY) * ROB_SIZE);  // all rob entry set to 0
	rob->commit_ptr = 0; // rob commit pointer set to 0
	rob->issue_ptr = 0; // rob issue pointer set to 0
	rob->buffer_length  = 0; // rob buffer length set to 0

	return rob;
}

APEX_RENAME_TABLE* init_rename_table() {

	APEX_RENAME_TABLE* rename_table = malloc(sizeof(*rename_table) * RENAME_TABLE_SIZE);
	if (!rename_table) {
		return NULL;
	}

	memset(rename_table, 0, sizeof(APEX_RENAME_TABLE) * RENAME_TABLE_SIZE);  // all rename table entry set to 0

	return rename_table;
}

void deinit_reorder_buffer(APEX_ROB* rob) {
	free(rob);
}

void deinit_rename_table(APEX_RENAME_TABLE* rename_table) {
	free(rename_table);
}


int can_add_entry_in_reorder_buffer(APEX_ROB* rob) {
	if (rob->issue_ptr == ROB_SIZE) {
		rob->issue_ptr = 0; // go back to zero index
	}
	if (rob->buffer_length == ROB_SIZE) {
		return FAILURE;
	}
	else if (rob->rob_entry[rob->issue_ptr].status) { // checking if enrty if free
		return FAILURE;
	}
	else {
		return SUCCESS;
	}
}


int add_reorder_buffer_entry(APEX_ROB* rob, ROB_Entry rob_entry) {
	// if instruction sucessfully added then only pass the instruction to function units
	// entry gets added by DRF at the same time dispatching instruction to issue_queue
	// using rob_entry to add entry so first add to issue_queue and use the same entry to add to rob simultaniously
	if (rob->issue_ptr == ROB_SIZE) {
		rob->issue_ptr = 0; // go back to zero index
	}
	if (rob->buffer_length == ROB_SIZE) {
		return FAILURE;
	}
	else if (rob->rob_entry[rob->issue_ptr].status) { // checking if enrty if free
		return FAILURE;
	}
	else {
		rob->rob_entry[rob->issue_ptr].status = VALID;
		rob->rob_entry[rob->issue_ptr].inst_type = rob_entry.inst_type;
		rob->rob_entry[rob->issue_ptr].inst_ptr = rob_entry.pc;
		rob->rob_entry[rob->issue_ptr].rd = rob_entry.rd;
		rob->rob_entry[rob->issue_ptr].rd_value = -9999;
		rob->rob_entry[rob->issue_ptr].exception = 0;
		rob->rob_entry[rob->issue_ptr].valid = 0;
		// increment buffer_length and issue_ptr
		rob->buffer_length += 1;
		rob->issue_ptr += 1;
	}
	return SUCCESS;
}


int update_reorder_buffer_entry_data(APEX_ROB* rob, ROB_Entry rob_entry) {
	// check the stage data and update the rob data value
	int update_position = -1;
	if (!rob_entry.executed) {
		return FAILURE;
	}
	else {
		// for now loop through entire rob and check
		// a better way would be checking between commit_ptr and issue_ptr
		for (int i=0; i<ROB_SIZE; i++) {
			// check only alloted entries
			if (rob->rob_entry[i].status == 1) {
				// check pc to match instructions
				if (rob->rob_entry[i].inst_ptr==rob_entry.pc) {
					update_position = i;
					break;
				}
			}
		}
		if (update_position<0) {
			return FAILURE;
		}
		else {
			if (rob->rob_entry[update_position].rd == rob_entry.rd) {
				rob->rob_entry[update_position].rd_value = rob_entry.rd_value;
				rob->rob_entry[update_position].valid = 1;
			}
			else {
				return ERROR; // found the pc value inst but failed to match desc reg
			}
		}
	}
	return SUCCESS;
}


int commit_reorder_buffer_entry(APEX_ROB* rob, int* cpu_reg, int* cpu_reg_valid) {

	if (rob->commit_ptr == ROB_SIZE) {
		rob->commit_ptr = 0; // go back to zero index
	}
	if (!rob->rob_entry[rob->commit_ptr].valid) {
		return FAILURE;
	}
	else {
		// inst ready to commit
		// change decs reg in cpu
		cpu_reg[rob->rob_entry[rob->commit_ptr].rd] = rob->rob_entry[rob->commit_ptr].rd_value;
		// decrement reg invalid count in cpu
		cpu_reg_valid[rob->rob_entry[rob->commit_ptr].rd] -= 1;
		// free the rob entry
		rob->rob_entry[rob->commit_ptr].status = INVALID;
		rob->rob_entry[rob->commit_ptr].inst_type = -9999;
		rob->rob_entry[rob->commit_ptr].inst_ptr = -9999;
		rob->rob_entry[rob->commit_ptr].rd = -9999;
		rob->rob_entry[rob->commit_ptr].rd_value = -9999;
		rob->rob_entry[rob->commit_ptr].exception = 0;
		rob->rob_entry[rob->commit_ptr].valid = INVALID;
		// decrement buffer_length and increment commit_ptr
		rob->buffer_length -= 1;
		rob->commit_ptr += 1;
	}
	return SUCCESS;
}


void print_rob_rename_content(APEX_ROB* rob, APEX_RENAME_TABLE* rename_table) {

	if (ENABLE_REG_MEM_STATUS_PRINT) {
		char* inst_type_str = (char*) malloc(10);
		printf("\n============ STATE OF REORDER BUFFER ============\n");
		printf("ROB Buffer Length: %d, Commit Pointer: %d, Issue Pointer: %d\n", rob->buffer_length, rob->commit_ptr, rob->issue_ptr);
		printf("Index, Status, Type, OpCode, Rd-value, Exception, Valid\n");
		for (int i=0;i<ROB_SIZE;i++) {
			strcpy(inst_type_str, "");
			get_inst_name(rob->rob_entry[i].inst_type, inst_type_str);
			printf("%02d\t|\t%d\t|\t%d\t|\t%s\t|\tR%02d-%d\t|\t%d\t|\t%d\n",
		 					i, rob->rob_entry[i].status, rob->rob_entry[i].inst_type, inst_type_str, rob->rob_entry[i].rd, rob->rob_entry[i].rd_value,
							rob->rob_entry[i].exception, rob->rob_entry[i].valid);
		}
		printf("\n============ STATE OF RENAME ADDR TABLE ============\n");
		printf("Index, Tag, Valid\n");
		for (int i=0;i<RENAME_TABLE_SIZE;i++) {
			printf("%02d\t|\t%02d\t|\t%d\n", i, rename_table[i].rob_tag, rename_table[i].tag_valid);
		}
	free(inst_type_str);
	}
}
