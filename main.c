/*
 *  main.c
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "ls_iq.h"
#include "rob.h"



int main(int argc, char const* argv[]) {

  char command[20];
  char* cmd;
  int num_cycle = 0;
  char func[10];
  // argc = count of arguments, executable being 1st argument in argv[0]
  if ((argc == 4)||(argc == 2)) {
    fprintf(stderr, "APEX_INFO : Initializing CPU !!!\n");
    APEX_CPU* cpu = APEX_cpu_init(argv[1]);
    APEX_LSQ* ls_queue = init_ls_queue();
    APEX_IQ* issue_queue = init_issue_queue();
    APEX_ROB* rob = init_reorder_buffer();
    APEX_RENAME_TABLE* rename_table = init_rename_table();

    if ((!cpu)||(!issue_queue)||(!ls_queue)||(!rob)||(!rename_table)) {
      fprintf(stderr, "APEX_Error : Unable to initialize CPU\n");
      exit(1);
    }
    int ret = 0;
    if (argc == 4) {
      strcpy(func, argv[2]);
      num_cycle = atoi(argv[3]);
      if (((strcmp(func, "display") == 0)||(strcmp(func, "simulate")==0))&&(num_cycle>0)) {
        ret = APEX_cpu_run(cpu, num_cycle, ls_queue, issue_queue, rob, rename_table);
        if (ret == SUCCESS) {
          printf("Simulation Complete\n");
        }
        else {
          printf("Simulation Return Code %d\n",ret);
        }
        if (strcmp(func, "display") == 0) {
          // show everything
          print_cpu_content(cpu);
          print_ls_iq_content(ls_queue, issue_queue);
          print_rob_rename_content(rob, rename_table);
        }
      }
      else {
        fprintf(stderr, "Invalid parameters passed !!!\n");
        if (!num_cycle) {
          fprintf(stderr, "Number of Cycles cannot be 0\n");
        }
        fprintf(stderr, "APEX_Help : Usage %s <input_file> <func(eg: simulate Or display)> <num_cycle>\n", argv[0]);
        exit(1);
      }
    }
    else if (argc == 2) {
      while (ret==0) {
        fprintf(stderr, "Usage ?: <func(eg: simulate Or display)> <num_cycle>\n");
        fprintf(stderr, "Exit : exit<space><entre>\n");
        fgets(command, 20, stdin);
        cmd = strtok (command," ");
        strcpy(func, cmd);
        cmd = strtok (NULL," ");
        num_cycle = atoi(cmd);
        if (strcmp(func, "exit") == 0) {
          printf("Terminating Simulation\n");
          ret = 1;
          break;
        }
        if (((strcmp(func, "display") == 0)||(strcmp(func, "simulate")==0))&&(num_cycle>0)) {
          ret = APEX_cpu_run(cpu, num_cycle, ls_queue, issue_queue, rob, rename_table);
          if (ret == SUCCESS) {
            printf("Simulation Complete\n");
          }
          else {
            printf("Simulation Return Code %d\n",ret);
          }
          if (strcmp(func, "display") == 0) {
            // show everything
            print_cpu_content(cpu);
            print_ls_iq_content(ls_queue, issue_queue);
            print_rob_rename_content(rob, rename_table);
          }
        }
        else {
          fprintf(stderr, "Invalid parameters passed !!!\n");
          if (!num_cycle) {
            fprintf(stderr, "Number of Cycles cannot be 0\n");
          }
          fprintf(stderr, "APEX_Help : Usage %s <input_file> <func(eg: simulate Or display)> <num_cycle>\n", argv[0]);
        }
      }
    }
    printf("Press Any Key to Exit Simulation\n");
    getchar();
    deinit_issue_queue(issue_queue);
    deinit_ls_queue(ls_queue);
    deinit_rename_table(rename_table);
    deinit_reorder_buffer(rob);
    APEX_cpu_stop(cpu);
  }
  else {
    fprintf(stderr, "Invalid parameters passed !!!\n");
    fprintf(stderr, "APEX_Help : For One Time Execution !!!\n");
    fprintf(stderr, "Type: %s <input_file> <func(eg: simulate Or display)> <num_cycle>\n", argv[0]);
    fprintf(stderr, "APEX_Help : For N Time Execution !!!\n");
    fprintf(stderr, "Type: %s <input_file>\n", argv[0]);
    exit(1);
  }

  return 0;
}
