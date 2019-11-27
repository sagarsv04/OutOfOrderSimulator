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

#include "cpu.h"
#include "ls_iq.h"


#define ROB_SIZE 12
#define RENAME_TABLE_SIZE 24


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

typedef struct APEX_RENAME_TABLE {
	int tag_valid;				// tells if rename table points to valid rob entry
	int rob_tag;					// holds the index of rob entry ie issue_ptr
} APEX_RENAME_TABLE;

typedef struct APEX_ROB {
	int commit_ptr;				  // pointer index for commit rob entry
	int issue_ptr;					// pointer index of last rob entry
	int buffer_length;			// rob buffer length
	APEX_ROB_ENTRY rob_entry[ROB_SIZE];					// array of ROB_SIZE rob entry struct. Note: use . in struct with variable names, use -> when its a pointer
} APEX_ROB;


APEX_ROB* init_reorder_buffer();
APEX_RENAME_TABLE* init_rename_table();
void deinit_reorder_buffer(APEX_ROB* rob);
void deinit_rename_table(APEX_RENAME_TABLE* rename_table);

int add_reorder_buffer_entry(APEX_ROB* rob, APEX_IQ* iq_entry);
int update_reorder_buffer_entry_data(APEX_ROB* rob, CPU_Stage* stage);
int commit_reorder_buffer_entry(APEX_ROB* rob, APEX_CPU* cpu);

void print_rob_rename_content(APEX_ROB* rob, APEX_RENAME_TABLE* rename_table);

 #endif
