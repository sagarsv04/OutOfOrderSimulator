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
typedef struct APEX_IQ {
	int status;					// indicate if entry is free or allocated
	int inst_type;			// indicate instruction type
	int inst_ptr;				// indicate pc value of instruction
	int literal;				// hold literal value
	int rd;							// holds desc reg tag
	int rs1;						// holds src1 reg tag
	int rs1_ready;			// indicate if src1 is ready
	int rs1_value;			// holds src1 reg value
	int rs2;						// holds src2 reg tag
	int rs2_ready;			// indicate if src2 is ready
	int rs2_value;			// holds src2 reg value
	int lsq_index;			// to address lSQ entry in issue queue
} APEX_IQ;

/* Format of an APEX Load Strore Queue mechanism  */
typedef struct APEX_LSQ {
	int status;					// indicate if entry is free or allocated
	int load_store;			// indicate if entry is for load or store
	int mem_valid;			// indicate if memory address is valid
	int rd;							// holds destination reg tag
	int rd_value;			// holds src1 reg value
	int data_ready;			// indicate if data is ready
	int rs1;						// holds src1 reg tag
	int rs1_value;			// holds src1 reg value
	int rs2;						// holds src1 reg tag
	int rs2_value;			// holds src1 reg value
} APEX_LSQ;






 #endif