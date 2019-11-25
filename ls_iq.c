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


int add_issue_queue_entry(APEX_IQ* issue_queue, CPU_Stage* stage) {
	// if instruction sucessfully added then only pass the instruction to function units
	int add_position = -1;

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
		issue_queue[add_position].inst_type = stage->inst_type;
		issue_queue[add_position].inst_ptr = stage->pc;
		issue_queue[add_position].rd = stage->rd;
		issue_queue[add_position].rs1 = stage->rs1;
		issue_queue[add_position].rs1_value = stage->rs1_value;
		issue_queue[add_position].rs2 = stage->rs2;
		issue_queue[add_position].rs2_value = stage->rs2_value;
		issue_queue[add_position].literal = stage->imm;
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
