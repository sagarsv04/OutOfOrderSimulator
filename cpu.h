#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_
/**
 *  cpu.h
 *  Contains various CPU and Pipeline Data structures
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */


#define RUNNING_IN_WINDOWS 0

#define DATA_MEMORY_SIZE 4096
#define REGISTER_FILE_SIZE 32

enum {
	F,
	DRF,
	INT_ONE,
	INT_TWO,
	MUL_ONE,
	MUL_TWO,
	MUL_THREE,
	BRANCH,
	MEM_ONE,
	MEM_TWO,
	MEM_THREE,
	WB,
	NUM_STAGES
};

enum {
	SUCCESS,
	FAILURE,
	HALTED,
	ERROR,
	EMPTY,
	NUM_EXIT
};

/* Index of Flags */
enum {
	ZF, // Zero Flag index
	CF, // Carry Flag index
	OF, // Overflow Flag index
	IF, // Interrupt Flag index
	NUM_FLAG
};

/* Instructions Type */
enum {
	STORE = 1,
	STR,
	LOAD,
	LDR,
	MOVC,
	MOV,
	ADD,
	ADDL,
	SUB,
	SUBL,
	MUL,
	DIV,
	AND,
	OR,
	EXOR,
	BZ,
	BNZ,
	JUMP,
	HALT,
	NOP
};


/* Format of an APEX instruction  */
typedef struct APEX_Instruction {
	char opcode[128];	// Operation Code
	int rd;           // Destination Register Address
	int rs1;          // Source-1 Register Address
	int rs2;          // Source-2 Register Address
	int imm;          // Literal Value
	int type;         // Inst Type
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage {
	int pc;           // Program Counter
	char opcode[128]; // Operation Code
	int rd;           // Destination Register Address
	int rs1;          // Source-1 Register Address
	int rs2;          // Source-2 Register Address
	int imm;          // Literal Value
	int rd_value;     // Destination Register Value
	int rs1_value;    // Source-1 Register Value
	int rs1_valid;    // Source-1 Register Value Valid
	int rs2_value;    // Source-2 Register Value
	int rs2_valid;    // Source-2 Register Value Valid
	int buffer;       // Latch to hold some value (currently used to hold literal value from decode)
	int mem_address;  // Computed Memory Address
	int inst_type;		// instruction type
	int stalled;      // Flag to indicate, stage is stalled
	int executed;     // Flag to indicate, stage has executed or not
	int empty;        // Flag to indicate, stage is empty
} CPU_Stage;

/* Model of APEX CPU */
typedef struct APEX_CPU {

	int clock;		// clock cycles elasped
	int pc;		// current program counter
	int regs[REGISTER_FILE_SIZE];		// integer register file
	int regs_invalid[REGISTER_FILE_SIZE];		// integer register valid file
	CPU_Stage stage[NUM_STAGES];		// array of CPU_Stage struct. Note: use . in struct with variable names, use -> when its a pointer
	APEX_Instruction* code_memory;		// struct pointer where instructions are stored
	int flags[NUM_FLAG];
	int code_memory_size;
	int data_memory[DATA_MEMORY_SIZE];		// data cache memory
	int ins_completed;		// instruction completed count
} APEX_CPU;


APEX_Instruction* create_code_memory(const char* filename, int* size);

APEX_CPU* APEX_cpu_init(const char* filename);

int previous_arithmetic_check(APEX_CPU* cpu, int func_unit);

int simulate(APEX_CPU* cpu, int num_cycle);

int display(APEX_CPU* cpu, int num_cycle);

void print_cpu_content(APEX_CPU* cpu);

int APEX_cpu_run(APEX_CPU* cpu, int num_cycle);

void APEX_cpu_stop(APEX_CPU* cpu);

// ##################### Out-of-Order ##################### //

int fetch(APEX_CPU* cpu);

int decode(APEX_CPU* cpu);

int issue_queue(APEX_CPU* cpu);

int ls_queue(APEX_CPU* cpu);

int cpu_execute(APEX_CPU* cpu); // cpu execute will hav diff FU calls

int rob_commit(APEX_CPU* cpu);

// ##################### Sub calls ##################### //

int int_one_stage(APEX_CPU* cpu);

int int_two_stage(APEX_CPU* cpu);

int mul_one_stage(APEX_CPU* cpu);

int mul_two_stage(APEX_CPU* cpu);

int mul_three_stage(APEX_CPU* cpu);

int branch_stage(APEX_CPU* cpu);

int memory_one_stage(APEX_CPU* cpu);

int memory_two_stage(APEX_CPU* cpu);

int memory_three_stage(APEX_CPU* cpu);

int writeback_stage(APEX_CPU* cpu);

#endif
