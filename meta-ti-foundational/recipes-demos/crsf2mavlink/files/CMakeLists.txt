cmake_minimum_required(VERSION 3.16)
project(crsf2mavlink C)

add_executable(crsf2mavlink crsf2mavlink.c)
target_compile_options(crsf2mavlink PRIVATE -Wall -Wextra -O2)

install(TARGETS crsf2mavlink DESTINATION sbin)
