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

void get_inst_name(int inst_type, char* inst_type_str);
void clear_stage_entry(APEX_CPU* cpu, int stage_index);
void add_bubble_to_stage(APEX_CPU* cpu, int stage_index);
void push_func_unit_stages(APEX_CPU* cpu, int after_iq);

int get_reg_values(APEX_CPU* cpu, int src_reg);
int get_reg_status(APEX_CPU* cpu, int reg_number);
void set_reg_status(APEX_CPU* cpu, int reg_number, int status);
int previous_arithmetic_check(APEX_CPU* cpu, int func_unit);

APEX_Forward get_cpu_forwarding_status(APEX_CPU* cpu, CPU_Stage* stage);


#endif
