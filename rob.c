/*
 *  rob.c
 *  Contains APEX reorder buffer implementation
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */

#include "rob.h"
#include "cpu.h"

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


int add_reorder_buffer_entry(APEX_ROB* rob, APEX_IQ* iq_entry) {
  // if instruction sucessfully added then only pass the instruction to function units
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
    rob->rob_entry[rob->issue_ptr].rd_value = -99;
    rob->rob_entry[rob->issue_ptr].exception = 0;
    rob->rob_entry[rob->issue_ptr].valid = 0;
    // increment buffer_length and issue_ptr
    buffer_length += 1;
    issue_ptr += 1;
  }
  return SUCCESS;
}
