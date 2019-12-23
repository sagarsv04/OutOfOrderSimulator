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

	memset(rob->rob_entry, 0, sizeof(APEX_ROB_ENTRY)*ROB_SIZE);  // all rob entry set to 0
	rob->commit_ptr = 0; // rob commit pointer set to 0
	rob->issue_ptr = 0; // rob issue pointer set to 0
	rob->buffer_length  = 0; // rob buffer length set to 0

	return rob;
}

APEX_RENAME* init_rename_table() {

	APEX_RENAME* rename_table = malloc(sizeof(*rename_table));
	if (!rename_table) {
		return NULL;
	}

	memset(rename_table->reg_rename, 0, sizeof(APEX_RENAME_TABLE)*RENAME_TABLE_SIZE);  // all rename table entry set to 0
	memset(rename_table->rat_rename, 0, sizeof(APEX_ARF_TABLE)*RENAME_TABLE_SIZE);  // all rename table entry set to 0
	rename_table->last_rename_pos = 0;

	return rename_table;
}

void deinit_reorder_buffer(APEX_ROB* rob) {
	free(rob);
}

void deinit_rename_table(APEX_RENAME* rename_table) {
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
		rob->rob_entry[rob->issue_ptr].rd_value = -1;
		rob->rob_entry[rob->issue_ptr].exception = 0;
		rob->rob_entry[rob->issue_ptr].valid = 0;
		if (rob_entry.inst_type==HALT) {
			rob->rob_entry[rob->issue_ptr].valid = VALID;
		}
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
			if (rob->rob_entry[i].status == VALID) {
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
				rob->rob_entry[update_position].valid = rob_entry.rd_valid;
				if (rob_entry.inst_type==BRANCH) {
					rob->rob_entry[update_position].valid = rob_entry.rd_valid;
					rob->rob_entry[update_position].exception = rob_entry.rd_valid;
				}
			}
			else {
				return ERROR; // found the pc value inst but failed to match desc reg
			}
		}
	}
	return SUCCESS;
}


int commit_reorder_buffer_entry(APEX_ROB* rob, ROB_Entry* rob_entry) {

	if (rob->commit_ptr == ROB_SIZE) {
		rob->commit_ptr = 0; // go back to zero index
	}
	if (!rob->rob_entry[rob->commit_ptr].valid) {
		return FAILURE;
	}
	else {
		// inst ready to commit
		rob_entry->inst_type = rob->rob_entry[rob->commit_ptr].inst_type;
		rob_entry->executed = rob->rob_entry[rob->commit_ptr].status;
		rob_entry->pc = rob->rob_entry[rob->commit_ptr].inst_ptr;
		rob_entry->rd = rob->rob_entry[rob->commit_ptr].rd;
		rob_entry->rd_value = rob->rob_entry[rob->commit_ptr].rd_value;
		rob_entry->rd_valid = rob->rob_entry[rob->commit_ptr].valid;
		rob_entry->exception = rob->rob_entry[rob->commit_ptr].exception;
		rob_entry->rs1 = INVALID;
		rob_entry->rs1_value = INVALID;
		rob_entry->rs1_valid = INVALID;
		rob_entry->rs2 = INVALID;
		rob_entry->rs2_value = INVALID;
		rob_entry->rs2_valid = INVALID;
		rob_entry->buffer = INVALID;
		rob_entry->stage_cycle = INVALID;

		// free the rob entry
		rob->rob_entry[rob->commit_ptr].status = INVALID;
		rob->rob_entry[rob->commit_ptr].inst_type = INVALID;
		rob->rob_entry[rob->commit_ptr].inst_ptr = INVALID;
		rob->rob_entry[rob->commit_ptr].rd = INVALID;
		rob->rob_entry[rob->commit_ptr].rd_value = INVALID;
		rob->rob_entry[rob->commit_ptr].exception = 0;
		rob->rob_entry[rob->commit_ptr].valid = INVALID;
		// decrement buffer_length and increment commit_ptr
		rob->buffer_length -= 1;
		rob->commit_ptr += 1;
	}
	return SUCCESS;
}


int can_rename_reg_tag(APEX_RENAME* rename_table) {

	int rename_position = -1;
	int search_start = 0;

	if (rename_table->last_rename_pos<(RENAME_TABLE_SIZE-1)) {
		search_start = rename_table->last_rename_pos;
	}

	for (int i=search_start; i<RENAME_TABLE_SIZE; i++) {
		if (rename_table->reg_rename[i].tag_valid==INVALID) {
			rename_position = i;
			break;
		}
	}
	if (rename_position<0) {
		return FAILURE;
	}
	else {
		return SUCCESS;
	}
}


int check_if_reg_renamed(int arch_reg, APEX_RENAME* rename_table) {
	// this tells if the arch regs are already renamed
	// check in reverse order
	int rename_position = -1;

	for (int i=RENAME_TABLE_SIZE-1; i>=0; i--) {
		if (rename_table->reg_rename[i].tag_valid==VALID) {
			if (rename_table->reg_rename[i].rename_tag == arch_reg) {
				rename_position = i;
				break;
			}
		}
	}
	if (rename_position<0) {
		return FAILURE;
	}
	else {
		return SUCCESS;
	}
}


int get_reg_renamed_tag(int* src_reg, APEX_RENAME* rename_table) {
	// this gives already renamed arch regs
	// check in reverse order
	int rename_position = -1;

	for (int i=RENAME_TABLE_SIZE-1; i>=0; i--) {
		if (rename_table->reg_rename[i].tag_valid==VALID) {
			if (rename_table->reg_rename[i].rename_tag == *src_reg) {
				rename_position = i;
				break;
			}
		}
	}
	if (rename_position<0) {
		return FAILURE;
	}
	else {
		*src_reg = rename_position;
	}
	return SUCCESS;
}


int get_phy_reg_renamed_tag(int phy_reg, APEX_RENAME* rename_table) {
	// any_reg is P
	// check in reverse order
	int arch_reg = -1;

	if (rename_table->reg_rename[phy_reg].tag_valid==VALID) {
		arch_reg = rename_table->reg_rename[phy_reg].rename_tag;
	}

	return arch_reg;
}


int rename_desc_reg(int* desc_reg, APEX_RENAME* rename_table) {
	// this actually renames the regs
	int rename_position = -1;
	int search_start = 0;

	if (rename_table->last_rename_pos<(RENAME_TABLE_SIZE-1)) {
		search_start = rename_table->last_rename_pos;
	}

	for (int i=search_start; i<RENAME_TABLE_SIZE; i++) {
		if (rename_table->reg_rename[i].tag_valid==INVALID) {
			rename_position = i;
			break;
		}
	}
	if (rename_position<0) {
		return FAILURE;
	}
	else {
		rename_table->reg_rename[rename_position].rename_tag = *desc_reg;
		*desc_reg = rename_position;
		rename_table->reg_rename[rename_position].tag_valid = VALID;
		rename_table->last_rename_pos = rename_position;
	}

	return SUCCESS;
}


void clear_rename_table(APEX_RENAME* rename_table) {

	// clear all rename
	for(int i=0; i<RENAME_TABLE_SIZE; i++) {
		rename_table->reg_rename[i].tag_valid = INVALID;
		rename_table->reg_rename[i].rename_tag = INVALID;
	}
	rename_table->last_rename_pos = INVALID;
}


void clear_reorder_buffer(APEX_ROB* rob) {

	// clear all rob entries
	for(int i=0; i<ROB_SIZE; i++) {
		rob->rob_entry[i].status = INVALID;
		rob->rob_entry[i].inst_type = INVALID;
		rob->rob_entry[i].inst_ptr = INVALID;
		rob->rob_entry[i].rd = INVALID;
		rob->rob_entry[i].rd_value = INVALID;
		rob->rob_entry[i].exception = INVALID;
		rob->rob_entry[i].valid = INVALID;
	}
	rob->commit_ptr = INVALID;
	rob->issue_ptr = INVALID;
	rob->buffer_length = INVALID;
}


void print_rob_and_rename_content(APEX_ROB* rob, APEX_RENAME* rename_table) {

	if (ENABLE_REG_MEM_STATUS_PRINT) {
		char* inst_type_str = (char*) malloc(10);
		printf("\n============ STATE OF REORDER BUFFER ============\n");
		printf("ROB Buffer Length: %d, Commit Pointer: %d, Issue Pointer: %d\n", rob->buffer_length, rob->commit_ptr, rob->issue_ptr);
		printf("Index, "
						"Status, "
						"Type, "
						"OpCode, "
						"Rd-value, "
						"Exception, "
						"Valid\n");
		for (int i=0;i<ROB_SIZE;i++) {
			strcpy(inst_type_str, "");
			get_inst_name(rob->rob_entry[i].inst_type, inst_type_str);
			printf("%02d\t|"
							"\t%d\t|"
							"\t%d\t|"
							"\t%.5s\t|"
							"\tR%02d-%d\t|"
							"\t%d\t|"
							"\t%d\n",
							i,
							rob->rob_entry[i].status,
							rob->rob_entry[i].inst_type,
							inst_type_str,
							rob->rob_entry[i].rd, rob->rob_entry[i].rd_value,
							rob->rob_entry[i].exception,
							rob->rob_entry[i].valid);
		}
		printf("\n============ STATE OF RENAME ADDR TABLE ============\n");
		printf("Index, "
						"Tag, "
						"Valid\n");
		for (int i=0;i<RENAME_TABLE_SIZE;i++) {
			printf("P%02d\t|"
							"\tR%02d\t|"
							"\t%d\n",
							i,
							rename_table->reg_rename[i].rename_tag,
							rename_table->reg_rename[i].tag_valid);
		}
	free(inst_type_str);
	}
}
