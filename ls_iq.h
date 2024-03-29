#ifndef _APEX_LS_IQ_H_
#define _APEX_LS_IQ_H_
/*
 *  ls_iq.h
 *  Contains APEX LSQ and IQ implementation
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
*/


#define IQ_SIZE 8

#define LSQ_SIZE 6


/* Format of an APEX Issue Queue mechanism  */
typedef struct IQ_FORMAT {
	int status;					// indicate if entry is free or allocated
	int inst_type;			// indicate instruction type
	int inst_ptr;				// indicate pc value of instruction
	int literal;				// hold literal value
	int rd;							// holds desc reg tag
	int rd_ready;			// indicate if src1 is ready
	int rd_value;				// holds desc reg value
	int rs1;						// holds src1 reg tag
	int rs1_ready;			// indicate if src1 is ready
	int rs1_value;			// holds src1 reg value
	int rs2;						// holds src2 reg tag
	int rs2_ready;			// indicate if src2 is ready
	int rs2_value;			// holds src2 reg value
	int stage_cycle;		// holds src2 reg value
	int lsq_index;			// to address lSQ entry in issue queue
} IQ_FORMAT;


/* Format of an APEX Load Strore Queue mechanism  */
typedef struct LSQ_FORMAT {
	int status;					// indicate if entry is free or allocated
	int load_store;			// indicate if entry is for load or store // use as inst_type
	int inst_ptr;				// indicate pc value of instruction
	int mem_valid;			// indicate if memory address is valid
	int mem_address;
	int rd;							// holds destination reg tag
	int rd_value;				// holds src1 reg value
	int data_ready;			// indicate if data is ready
	int rs1;						// holds src1 reg tag
	int rs1_value;			// holds src1 reg value
	int rs2;						// holds src1 reg tag
	int rs2_value;			// holds src1 reg value
	int literal;				// hold literal value
	int stage_cycle;
} LSQ_FORMAT;


typedef struct APEX_IQ {
	IQ_FORMAT iq_entries[IQ_SIZE];
}APEX_IQ;


typedef struct APEX_LSQ {
	LSQ_FORMAT lsq_entries[LSQ_SIZE];
}APEX_LSQ;

/* Format of an Load Store & Issue Queue entry/update mechanism  */
typedef struct LS_IQ_Entry {
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
	int mem_address;
	int lsq_index;
	int stage_cycle;
} LS_IQ_Entry;


APEX_IQ* init_issue_queue();
void deinit_issue_queue(APEX_IQ* issue_queue);

int can_add_entry_in_issue_queue(APEX_IQ* issue_queue);
int add_issue_queue_entry(APEX_IQ* issue_queue, LS_IQ_Entry ls_iq_entry, int* lsq_index);

int update_issue_queue_entry(APEX_IQ* issue_queue, LS_IQ_Entry ls_iq_entry);
int get_issue_queue_index_to_issue(APEX_IQ* issue_queue, int* issue_index);

APEX_LSQ* init_ls_queue();
void deinit_ls_queue(APEX_LSQ* ls_queue);

int can_add_entry_in_ls_queue(APEX_LSQ* ls_queue);
int add_ls_queue_entry(APEX_LSQ* ls_queue, LS_IQ_Entry ls_iq_entry, int* add_position);

int update_ls_queue_entry_mem_address(APEX_LSQ* ls_queue, LS_IQ_Entry ls_iq_entry);
int update_ls_queue_entry_reg(APEX_LSQ* ls_queue, LS_IQ_Entry ls_iq_entry);

int get_ls_queue_index_to_issue(APEX_LSQ* ls_queue, int* lsq_index);

void clear_issue_queue_entry(APEX_IQ* issue_queue);
void clear_ls_queue_entry(APEX_LSQ* ls_queue);

void print_ls_iq_content(APEX_LSQ* ls_queue, APEX_IQ* issue_queue);

#endif
