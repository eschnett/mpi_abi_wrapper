cmake_minimum_required(VERSION 3.20)
project(mpi_abi_wrapper-consume LANGUAGES C)

# The CMake-route half of S6's install-consumption check (NOTES.md #9): a
# standalone project, built against nothing but an installed mpi_abi_wrapper
# prefix, the way mpif's test-consume/ is standalone against an installed
# mpif. ci-scripts/check-install.sh configures this with
# -DCMAKE_PREFIX_PATH=<installed prefix> and nothing else, so a plain
# find_package(mpi_abi REQUIRED) is exactly what gets exercised.
find_package(mpi_abi REQUIRED)

enable_testing()
add_executable(hello hello.c)
target_link_libraries(hello PRIVATE mpi_abi::mpi_abi)
add_test(NAME hello COMMAND hello)
