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

## Testing

Tested agains MPICH 5.0.1, Open MPI 5.0.10, and MVAPICH 4.1.
