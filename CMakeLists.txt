cmake_minimum_required(VERSION 3.15)
project(apex_sim C)

set(CMAKE_C_STANDARD 99)

add_executable(apex_sim main.c cpu.c rob.c ls_iq.c forwarding.c file_parser.c)
