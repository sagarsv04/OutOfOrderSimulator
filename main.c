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



int main(int argc, char const* argv[]) {

  char command[20];
  char* cmd;
  int num_cycle = 0;
  char func[10];
  // argc = count of arguments, executable being 1st argument in argv[0]
  if (argc == 4) {
    fprintf(stderr, "APEX_INFO : Initializing One Time Execution !!!\n");
    strcpy(func, argv[2]);
    num_cycle = atoi(argv[3]);
    if (((strcmp(func, "display") == 0)||(strcmp(func, "simulate")==0))&&(num_cycle>0)) {
      APEX_CPU* cpu = APEX_cpu_init(argv[1]);
      if (!cpu) {
        fprintf(stderr, "APEX_Error : Unable to initialize CPU\n");
        exit(1);
      }
      int ret = 0;
      if (strcmp(func, "display") == 0) {
        // show everything
        ret = APEX_cpu_run(cpu, num_cycle);
        if (ret == SUCCESS) {
          printf("(apex) >> Simulation Complete");
        }
        else {
          printf("Simulation Return Code %d\n",ret);
        }
        print_cpu_content(cpu);
        APEX_cpu_stop(cpu);
        printf("Press Any Key to Exit Simulation\n");
        getchar();
      }
      else {
        // show only stages
        ret = APEX_cpu_run(cpu, num_cycle);
        if (ret == SUCCESS) {
          printf("(apex) >> Simulation Complete");
        }
        else {
          printf("Simulation Return Code %d\n",ret);
        }
        APEX_cpu_stop(cpu);
        printf("Press Any Key to Exit Simulation\n");
        getchar();
      }
    }
    else {
      fprintf(stderr, "Invalid parameters passed !!!\n");
      if (!num_cycle) {
        fprintf(stderr, "Number of Cycles cannot be 0\n", argv[0]);
      }
      fprintf(stderr, "APEX_Help : Usage %s <input_file> <func(eg: simulate Or display)> <num_cycle>\n", argv[0]);
      exit(1);
    }
  }
  else if (argc == 2) {
    fprintf(stderr, "APEX_INFO : Initializing N Time Execution !!!\n");
    APEX_CPU* cpu = APEX_cpu_init(argv[1]);
    if (!cpu) {
      fprintf(stderr, "APEX_Error : Unable to initialize CPU\n");
      exit(1);
    }
    else {
      int ret = 0;
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
          if (strcmp(func, "display") == 0) {
            // show everything
            ret = APEX_cpu_run(cpu, num_cycle);
            if (ret == SUCCESS) {
              printf("(apex) >> Simulation Complete");
            }
            else {
              printf("Simulation Return Code %d\n",ret);
            }
            print_cpu_content(cpu);
          }
          else {
            // show only stages
            ret = APEX_cpu_run(cpu, num_cycle);
            if (ret == SUCCESS) {
              printf("(apex) >> Simulation Complete");
            }
            else {
              printf("Simulation Return Code %d\n",ret);
            }
          }
        }
      }
      APEX_cpu_stop(cpu);
      printf("Press Any Key to Exit Simulation\n");
      getchar();
    }
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
