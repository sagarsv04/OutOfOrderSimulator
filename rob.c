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
// #include "cpu.h"

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

int add_reorder_buffer_entry(APEX_ROB* rob, APEX_IQ* iq_entry) {
	// if instruction sucessfully added then only pass the instruction to function units
	// entry gets added by DRF at the same time dispatching instruction to issue_queue
	// using iq_entry to add entry so first add to issue_queue and use the same entry to add to rob simultaniously
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
		rob->rob_entry[rob->issue_ptr].status = iq_entry->status;
		rob->rob_entry[rob->issue_ptr].inst_type = iq_entry->inst_type;
		rob->rob_entry[rob->issue_ptr].inst_ptr = iq_entry->inst_ptr;
		rob->rob_entry[rob->issue_ptr].rd = iq_entry->rd;
		rob->rob_entry[rob->issue_ptr].rd_value = -9999;
		rob->rob_entry[rob->issue_ptr].exception = 0;
		rob->rob_entry[rob->issue_ptr].valid = 0;
		// increment buffer_length and issue_ptr
		rob->buffer_length += 1;
		rob->issue_ptr += 1;
	}
	return SUCCESS;
}


int update_reorder_buffer_entry_data(APEX_ROB* rob, CPU_Stage* stage) {
	// check the stage data and update the rob data value
	int update_position = -1;
	if (!stage->executed) {
		return FAILURE;
	}
	else {
		// for now loop through entire rob and check
		// a better way would be checking between commit_ptr and issue_ptr
		for (int i=0; i<ROB_SIZE; i++) {
			// check only alloted entries
			if (rob->rob_entry[i].status == 1) {
				// check pc to match instructions
				if (rob->rob_entry[i].inst_ptr==stage->pc) {
					update_position = i;
					break;
				}
			}
		}
		if (update_position<0) {
			return FAILURE;
		}
		else {
			if (rob->rob_entry[update_position].rd == stage->rd) {
				rob->rob_entry[update_position].rd_value = stage->rd_value;
				rob->rob_entry[update_position].valid = 1;
			}
			else {
				return ERROR; // found the pc value inst but failed to match desc reg
			}
		}
	}
	return SUCCESS;
}


int commit_reorder_buffer_entry(APEX_ROB* rob, APEX_CPU* cpu) {

	if (rob->commit_ptr == ROB_SIZE) {
		rob->commit_ptr = 0; // go back to zero index
	}
	if (!rob->rob_entry[rob->commit_ptr].valid) {
		return FAILURE;
	}
	else {
		// inst ready to commit
		// change decs reg in cpu
		cpu->regs[rob->rob_entry[rob->commit_ptr].rd] = rob->rob_entry[rob->commit_ptr].rd_value;
		// decrement reg invalid count in cpu
		cpu->regs_invalid[rob->rob_entry[rob->commit_ptr].rd] -= 1;
		// free the rob entry
		rob->rob_entry[rob->commit_ptr].status = 0;
		rob->rob_entry[rob->commit_ptr].inst_type = -9999;
		rob->rob_entry[rob->commit_ptr].inst_ptr = -9999;
		rob->rob_entry[rob->commit_ptr].rd = -9999;
		rob->rob_entry[rob->commit_ptr].rd_value = -9999;
		rob->rob_entry[rob->commit_ptr].exception = 0;
		rob->rob_entry[rob->commit_ptr].valid = 0;
		// decrement buffer_length and increment commit_ptr
		rob->buffer_length -= 1;
		rob->commit_ptr += 1;
	}
	return SUCCESS;
}
