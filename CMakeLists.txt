cmake_minimum_required(VERSION 3.20)
project(mpi_abi_wrapper C)

# S0: repository skeleton and header generation (STAGES.md). Only the ABI
# headers exist yet -- src/mpi_abi, src/mpiwrapper and the dev/ generator
# core are later stages.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(MPIABI_GEN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/gen/include)
set(MPIABI_GENERATE_HEADERS_SCRIPT ${CMAKE_SOURCE_DIR}/dev/generate_headers.py)

# Generated output is committed (NOTES.md #9): `regenerate` writes it, and is
# never part of the default build. CI instead runs `check-headers` (below),
# which fails on any drift between the committed files and a fresh
# generation.
add_custom_target(regenerate
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_GENERATE_HEADERS_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Regenerating gen/include/mpi.h, gen/include/mpiabi.h, dev/entrypoints.txt"
)

enable_testing()

add_test(NAME headers-up-to-date
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_GENERATE_HEADERS_SCRIPT} --check
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# S0 exit check: both headers compile standalone in a TU of their own, and
# (compile_both_headers.c) together in one TU with no collision -- the
# configuration libmpiwrapper will use from S1 onward.
add_library(check_mpi_h_standalone OBJECT test/compile_mpi_h.c)
target_include_directories(check_mpi_h_standalone PRIVATE ${MPIABI_GEN_INCLUDE_DIR})

add_library(check_mpiabi_h_standalone OBJECT test/compile_mpiabi_h.c)
target_include_directories(check_mpiabi_h_standalone PRIVATE ${MPIABI_GEN_INCLUDE_DIR})

add_library(check_both_headers OBJECT test/compile_both_headers.c)
target_include_directories(check_both_headers PRIVATE ${MPIABI_GEN_INCLUDE_DIR})

foreach(tgt check_mpi_h_standalone check_mpiabi_h_standalone check_both_headers)
  target_compile_options(${tgt} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endforeach()
