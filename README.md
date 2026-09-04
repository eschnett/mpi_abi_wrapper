# `mpi_abi_wrapper`: Provide the MPI ABI bindings on top of another MPI library

[![CI](https://github.com/eschnett/mpi_abi_wrapper/actions/workflows/ci.yaml/badge.svg)](https://github.com/eschnett/mpi_abi_wrapper/actions/workflows/ci.yaml)

This package provides C language bindings for the
[MPI](https://www.mpi-forum.org) ABI defined in the MPI standard 5.0.
This can be used as stop-gap measure for MPI implementations or
system-wide MPI installations which do not provide the new ABI.

The main advantage of the new MPI ABI is that applications can use MPI
libraries interchangeably. They can be built against one
implementation and can then use, at run time, any other MPI
implementation, provided that both implement the MPI ABI.

## Status

`mpi_abi_wrapper` implements the MPI functionality by forwarding all
calls to another MPI library. This other MPI library does not need to
implement the MPI ABI. `mpi_abi_wrapper` provides, in principle, the
full MPI 5.0 standard, but only if the actual MPI implementation does
so.

## Usage

`mpi_abi_wrapper` wraps an existing MPI installation in such a way
that it provide the new MPI ABI. To wrap multiple MPI installations,
as may be needed on an HPC system, each MPI installation needs to be
wrapped separately.

When installed, `mpi_abi_wrapper` provides a header file `mpi.h` (for
the MPI ABI) and library `libmpi_abi.so` (which implements the MPI
functionality, exposing the MPI ABI). This library looks and behaves
like a regular MPI library. All actual work is forwarded to the
exsiting MPI implementation.

## Examples

The [Nibi](https://docs.alliancecan.ca/wiki/Nibi) HPC system provides,
as its default MPI implementation, the [Open
MPI](https://www.open-mpi.org) library version 4.1.5, presumably
configured to make efficient use of this system's hardware. This
library implements the MPI 3.1 standard and does _not_ provide the MPI
ABI (which was only introduced with MPI 5.0).

We thus install `mpi_abi_wrapper` to wrap this MPI library:
```sh
mkdir -p $HOME/src && cd $HOME/src
git clone https://github.com/eschnett/mpi_abi_wrapper
cd mpi_abi_wrapper
cmake -Bbuild -DCMAKE_INSTALL_PREFIX=$HOME/openmpi-4.1.5-mpi-abi -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
```

When building an application, you can now choose
`$HOME/openmpi-4.1.5-mpi-abi` as your MPI library.

## Building and running against an installed prefix

The prefix looks and behaves like an MPI installation. Its `bin/` holds:

| | |
|---|---|
| `mpicc`, `mpicxx`, `mpic++` | compile and link against this prefix's `mpi.h` and `libmpi_abi` |
| `mpiexec`, `mpirun` | start a job, by forwarding to the wrapped MPI's own launcher |
| `mpi_abi_wrapper_info` | what this prefix resolves to; the first thing to ask when something looks wrong |

so an application builds and runs the ordinary way:

```sh
export PATH=$HOME/openmpi-4.1.5-mpi-abi/bin:$PATH
mpicc -o myprog myprog.c
mpiexec -n 4 ./myprog
```

There are three other ways to build against it, all equivalent:
`find_package(mpi_abi)` and a plain `find_package(MPI)` for CMake (the prefix
ships a `FindMPI` shim for the latter — point `CMAKE_MODULE_PATH` at
`<prefix>/lib/cmake/mpi_abi/Modules`), and `pkg-config --cflags --libs
mpi_abi`.

`mpiexec` here is a forwarder, not a launcher: starting a job is the wrapped
implementation's business, so this one resolves that MPI's own `mpiexec` and
hands it every argument untouched. It does not need to be told which MPI —
that was decided when the prefix was built — but two things are worth knowing:

- `MPI_ABI_WRAPPER_MPIEXEC` overrides which launcher it forwards to, and
  `MPI_ABI_WRAPPER_LIB` overrides which MPI a program actually runs against.
  Set both, and set them consistently: a program running against one MPI's
  library under another's launcher does not fail, it silently starts N
  one-rank jobs instead of one N-rank job.
- On one machine both MPICH's and Open MPI's launchers pass
  `MPI_ABI_WRAPPER_LIB` on to the ranks without being asked (measured — see
  `dev/launcher-env-forwarding/`). Across nodes they may not, so on a cluster
  state it explicitly with the launcher's own flag: `-x MPI_ABI_WRAPPER_LIB`
  for Open MPI, `-genv MPI_ABI_WRAPPER_LIB <value>` for MPICH.

**There is no `mpifort`.** `mpi_abi_wrapper` provides the C bindings only; the
Fortran side of the MPI ABI is the separate
[mpif](https://github.com/eschnett/mpif) project, which builds against a
prefix like this one.

## Testing

Tested agains MPICH 5.0.1, Open MPI 5.0.10, and MVAPICH 4.1.
