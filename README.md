# Out of Order Simulator

An instruction set simulator coded in C language, which mimics the behavior of a microprocessor by "reading" instructions and maintaining internal variables which represent the processor's registers.

Project 2 - APEX Out of Order Simulator
============

A simple implementation of Out of Order APEX Pipeline

Author :
============
Sagar Vishwakarma (svishwa2@binghamton.edu)

State University of New York, Binghamton

Notes:
============

1)	This code is a simple implementation of Out of Order Stage APEX Pipeline.
		Fetch -> Decode -> Issue Queue / Reorder Buffer / Load Store Queue -> Function Units -> Writeback

		You can read, modify and build upon given codebase to add other features as
		required in project description. You are also free to write your own
		implementation from scratch.

2)	All the stages have latency of one cycle. There are multiple functional unit in
		which perform all the arithmetic and logic operations.

3)	Logic to check data dependencies and renamed registers has included.

File-Info
============

1)	Makefile				- You can edit as needed
2)	file_parser.c 	- Contains Functions to parse input file.
3)	cpu.c						- Contains Implementation of APEX cpu.
4)	ls_iq.c					- Contains operations of Issue Queue and Load Store Queue.
5)	rob.c						- Contains operations of Reorder Buffer and Register Renaming.
6)	forwarding.c		- Contains operations of Stage Clearing, adding bubble(NOP) and forwaring instructions to stages.


How to compile and run
============

-	go to terminal, cd into project directory and type 'make' to compile project
-	Run using ./apex_sim <input_file> <func> <num_cycle>
		eg: ./apex_sim input.asm simulate 50

OR
==

- Run using ./apex_sim <input_file> (To execute in single stepping)
		eg: ./apex_sim input.asm
