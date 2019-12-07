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

	APEX_IQ* issue_queue = malloc(sizeof(*issue_queue)*IQ_SIZE);

	if (!issue_queue) {
		return NULL;
	}

	memset(issue_queue, 0, sizeof(APEX_IQ) * IQ_SIZE);  // all issue entry set to 0

	return issue_queue;
}

void deinit_issue_queue(APEX_IQ* issue_queue) {
	free(issue_queue);
}


int add_issue_queue_entry(APEX_IQ* issue_queue, LS_IQ_update stage_entry) {
	// if instruction sucessfully added then only pass the instruction to function units
	int add_position = -1;
	if (!stage_entry.executed) {
		return FAILURE;
	}
	else {
		for (int i=0; i<IQ_SIZE; i++) {
			if (issue_queue[i].status == 0) {
				add_position = i;
				break;
			}
		}
		if (add_position<0) {
			return FAILURE;
		}
		else {
			issue_queue[add_position].status = 1;
			issue_queue[add_position].inst_type = stage_entry.inst_type;
			issue_queue[add_position].inst_ptr = stage_entry.pc;
			issue_queue[add_position].rd = stage_entry.rd;
			issue_queue[add_position].rs1 = stage_entry.rs1;
			issue_queue[add_position].rs1_value = stage_entry.rs1_value;
			issue_queue[add_position].rs1_ready = stage_entry.rs1_valid;
			issue_queue[add_position].rs2 = stage_entry.rs2;
			issue_queue[add_position].rs2_value = stage_entry.rs2_value;
			issue_queue[add_position].rs2_ready = stage_entry.rs2_valid;
			issue_queue[add_position].literal = stage_entry.imm;
		}
	}
	return SUCCESS;
}


int update_issue_queue_entry(APEX_IQ* issue_queue, LS_IQ_update stage_entry) {
	// this is updating values from func unit to any entry which is waiting in queue
	// array to hold respective reg update positions
	// int rs1_position[IQ_SIZE] = {-1}; // these can use to see where values got updated
	// int rs2_position[IQ_SIZE] = {-1};
	int rs1_pos_sum = 0;
	int rs2_pos_sum = 0;
	if (!stage_entry.executed) {
    return FAILURE;
  }
	else {
		// for now loop through entire issue_queue and check
		for (int i=0; i<IQ_SIZE; i++) {
			// check only alloted entries
			if (issue_queue[i].status == 1) {
				// first check rd ie flow and output dependencies
				if (stage_entry.rd == issue_queue[i].rs1) {
					issue_queue[i].rs1_value = stage_entry.rd_value;
					issue_queue[i].rs1_ready = 1;
					// rs1_position[i] = i;
					rs1_pos_sum += 1;
				}
				if (stage_entry.rd == issue_queue[i].rs2) {
					issue_queue[i].rs2_value = stage_entry.rd_value;
					issue_queue[i].rs2_ready = 1;
					// rs2_position[i] = i;
					rs2_pos_sum += 1;
				}
			}
		}
	}
	if ((rs1_pos_sum == 0)&&(rs2_pos_sum == 0)) {
		return FAILURE;
	}
	return SUCCESS;
}


int get_issue_queue_index_to_issue(APEX_IQ* issue_queue, int* issue_index) {
	// get all the index of issue_queue entries
	// in cpu loop through index and issue instructions to respective func units
	// int issue_index[IQ_SIZE] = {-1}; // these can use to see where values got updated
	int index_sum = 0;
	for (int i=0; i<IQ_SIZE; i++) {
		// check only alloted entries
		if (issue_queue[i].status == 1) {
			// first check rd ie flow and output dependencies
			if ((issue_queue[i].rs1_ready)&&(issue_queue[i].rs2_ready)) {
				issue_index[i] = i;
				index_sum += 1;
			}
		}
	}
	if (index_sum == 0) {
		return FAILURE;
	}
	return SUCCESS;
}


/*
 * ########################################## Load Store Queue ##########################################
*/


APEX_LSQ* init_ls_queue() {

	APEX_LSQ* ls_queue = malloc(sizeof(*ls_queue)*LSQ_SIZE);

	if (!ls_queue) {
		return NULL;
	}

	memset(ls_queue, 0, sizeof(APEX_LSQ) * LSQ_SIZE);  // all issue entry set to 0

	return ls_queue;
}

void deinit_ls_queue(APEX_LSQ* ls_queue) {
	free(ls_queue);
}


int add_ls_queue_entry(APEX_LSQ* ls_queue, APEX_IQ* issue_queue) {
	// if instruction sucessfully added then only pass the instruction to function units
	int add_position = -1;

	for (int i=0; i<LSQ_SIZE; i++) {
		if (issue_queue[i].status == 0) {
			add_position = i;
			break;
		}
	}
	if (add_position<0) {
		return FAILURE;
	}
	else {
		ls_queue[add_position].status = issue_queue->status;
		ls_queue[add_position].load_store = issue_queue->inst_type;
		ls_queue[add_position].rd = issue_queue->rd;
		ls_queue[add_position].rd_value = issue_queue->rd_value;
		ls_queue[add_position].rs1 = issue_queue->rs1;
		ls_queue[add_position].rs1_value = issue_queue->rs1_value;
		ls_queue[add_position].rs2 = issue_queue->rs2;
		ls_queue[add_position].rs2_value = issue_queue->rs2_value;
		ls_queue[add_position].mem_valid = 0;
		ls_queue[add_position].data_ready = 0;
	}
	return SUCCESS;
}


void print_ls_iq_content(APEX_LSQ* ls_queue, APEX_IQ* issue_queue) {
	// if require to print the instruction instead of type just call the func fron cpu to print inst
	if (ENABLE_REG_MEM_STATUS_PRINT) {
		printf("\n============ STATE OF ISSUE QUEUE ============\n");
		printf("Index, Status, Type, Rd-value, Rs1-value-ready, Rs2-value-ready, LSQ Index\n");
		for (int i=0;i<IQ_SIZE;i++) {
			printf("%02d\t|\t%d\t|\t%d\t|\tR%02d-%d\t|\tR%02d-%d-%d\t|\tR%02d-%d-%d\t|\t%02d\n",
							i, issue_queue[i].status, issue_queue[i].inst_type, issue_queue[i].rd, issue_queue[i].rd_value,
							issue_queue[i].rs1, issue_queue[i].rs1_value, issue_queue[i].rs1_ready,
							issue_queue[i].rs2, issue_queue[i].rs2_value, issue_queue[i].rs2_ready, issue_queue[i].lsq_index);
		}
		printf("\n============ STATE OF LOAD STORE QUEUE ============\n");
		printf("Index, Status, Type, Mem Valid, Data Ready, Rd-value, Rs1-value, Rs2-value\n");
		for (int i=0;i<LSQ_SIZE;i++) {
			if ((ls_queue[i].load_store==STR) || (ls_queue[i].load_store==LDR)) {
				printf("%02d\t|\t%d\t|\t%d\t|\t%d\t|\t%d\t|\tR%02d-%d\t|\tR%02d-%d\t|\tR%02d-%d\n",
		 						i, ls_queue[i].status, ls_queue[i].load_store, ls_queue[i].mem_valid, ls_queue[i].data_ready,
								ls_queue[i].rd, ls_queue[i].rd_value, ls_queue[i].rs1, ls_queue[i].rs1_value, ls_queue[i].rs2, ls_queue[i].rs2_value);
			}
			else {
				printf("%02d\t|\t%d\t|\t%d\t|\t%d\t|\t%d\t|\tR%02d-%d\t|\tR%02d-%d\t|\t#%02d\n",
		 						i, ls_queue[i].status, ls_queue[i].load_store, ls_queue[i].mem_valid, ls_queue[i].data_ready,
								ls_queue[i].rd, ls_queue[i].rd_value, ls_queue[i].rs1, ls_queue[i].rs1_value, ls_queue[i].literal);
			}
		}
	}
}
