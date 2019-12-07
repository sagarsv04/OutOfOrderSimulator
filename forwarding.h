#ifndef _APEX_FORWARDING_H_
#define _APEX_FORWARDING_H_
/*
 *  forwarding.h
 *  Contains APEX Forwarding implementation
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
*/


#include "cpu.h"

/* Format of an APEX Forward mechanism  */
typedef struct APEX_Forward {
	int status;
	int rd_from;
	int rs1_from;     // CPU stage from which rs2 can be forwarded
	int rs2_from;
	int unstall;
} APEX_Forward;

char* get_inst_name(int inst_type);
void clear_stage_entry(APEX_CPU* cpu, int stage_index);
void add_bubble_to_stage(APEX_CPU* cpu, int stage_index, int flushed);
void push_stages(APEX_CPU* cpu);
APEX_Forward get_cpu_forwarding_status(APEX_CPU* cpu, CPU_Stage* stage);


#endif
