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


typedef struct APEX_ARF_TABLE {
	// array index is used to tell if its P0 or Pn
	// this tells us from which rob entry we will get our Pn Value
	int tag_valid;				// tells if rob tag is valid or not
	int rob_tag;					// holds the index of rob entry ie issue_ptr
}APEX_ARF_TABLE;


typedef struct APEX_RENAME_TABLE {
	// array index is used to tell if its P0 or Pn
	// this tells us which Pn will holds our desc reg value
	int tag_valid;					// tells if rename entry gor desc reg is valid
	int rename_tag;					// holds the index of desc reg like R0 or Rn
} APEX_RENAME_TABLE;


typedef struct APEX_ROB {
	int commit_ptr;				  // pointer index for commit rob entry
	int issue_ptr;					// pointer index of last rob entry
	int buffer_length;			// rob buffer length
	APEX_ROB_ENTRY rob_entry[ROB_SIZE];					// array of ROB_SIZE rob entry struct. Note: use . in struct with variable names, use -> when its a pointer
} APEX_ROB;


typedef struct APEX_RENAME {
	APEX_RENAME_TABLE reg_rename[RENAME_TABLE_SIZE];
	APEX_ARF_TABLE arf_rename[RENAME_TABLE_SIZE];
} APEX_RENAME;


/* Format of an ROB entry/update mechanism  */
typedef struct ROB_Entry {
	int inst_type;
	int executed;
	int pc;
	int rd;
	int rd_value;
	int rd_valid;
	int rs1;
	int rs1_value;
	int rs1_valid;
	int rs2;
	int rs2_value;
	int rs2_valid;
	int buffer;
	int stage_cycle;
} ROB_Entry;


APEX_ROB* init_reorder_buffer();
APEX_RENAME* init_rename_table();
void deinit_reorder_buffer(APEX_ROB* rob);
void deinit_rename_table(APEX_RENAME* rename_table);

int can_add_entry_in_reorder_buffer(APEX_ROB* rob);
int add_reorder_buffer_entry(APEX_ROB* rob, ROB_Entry rob_entry);

int can_rename_reg_tag(APEX_RENAME* rename_table);
int rename_desc_reg(int* desc_reg, APEX_RENAME* rename_table);

int check_if_reg_renamed(int any_reg, APEX_RENAME* rename_table);
int get_reg_renamed_tag(int* src_reg, APEX_RENAME* rename_table);

int update_reorder_buffer_entry_data(APEX_ROB* rob, ROB_Entry rob_entry);
int commit_reorder_buffer_entry(APEX_ROB* rob, int* cpu_reg, int* cpu_reg_valid);

void print_rob_and_rename_content(APEX_ROB* rob, APEX_RENAME* rename_table);

 #endif
