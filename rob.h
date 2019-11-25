#ifndef _APEX_ROB_H_
#define _APEX_ROB_H_
/*
 *  rob.h
 *  Contains APEX reorder buffer implementation
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */


#define ROB_SIZE 12


/* Format of an APEX ROB mechanism  */
typedef struct APEX_ROB_ENTRY {
	int status;					// indicate if entry is free or allocated
	int inst_type;			// indicate instruction type
	int inst_ptr;				// holds instruction address
	int rd;							// holds destination reg tag
	int rd_value;			  // holds destination reg value
	int exception;			// indicate if there is exception
	int valid;					// indicate if instruction is ready to commit
} APEX_ROB_ENTRY;

typedef struct APEX_ROB {
	int commit_ptr;				  // pointer index for commit rob entry
	int issue_ptr;					// pointer index of last rob entry
	int buffer_length;			// rob buffer length
	APEX_ROB_ENTRY rob_entry[ROB_SIZE];					// array of ROB_SIZE rob entry struct. Note: use . in struct with variable names, use -> when its a pointer
} APEX_ROB;


int reorder_buffer(APEX_ROB* reorder_buffer);


 #endif
