# Overall goal

Implement the MPI C bindings according to the new MPI ABI, based on an
existing MPI implementation. Each MPI call is to be forwarded to this
MPI implementation (called "the MPI library" below), converting
arguments forth and back according to the MPI ABI and existing MPI
implementation's choices.

Produce a library called `mpi_abi` and an include file `mpi.h`.
`mpi.h` is to be taken from https://github.com/mpi-forum/mpi-abi-stubs
(with a patch, as described below).

## Design

Implement `mpi_abi` in C.

`mpi_abi` does not link directly against the MPI library. Instead, it
uses `dlopen` and `dlsym` at initialization time to open the MPI
library. This avoids name conflicts, because both `mpi_abi` and the
MPI library will provide functions called e.g. `MPI_Init`.

In addition to the functions defined by the MPI ABI, wrap also some
Fortran-related functions. These functions are defined in
`doc/mpi.h.patch`. This patch corrects also a small error in `mpi.h`
for some `Psend`/`Precv` functions.

MPI handles, MPI status objects, and many other constants need to be
converted before and/or after forwarding calls to the MPI library.
This may require allocating temporaries.

See the directory `examples` for examples for wrapping functions,
converting arguments etc.

### MPI handles

The MPI library might use `#define` to map predefined handles to
internal names, e.g. via `#define MPI_COMM_WORLD internal_comm_world`.
To access predefined handles, define another shared library
`mpiwrapper` which links directly against the MPI library. This
library contains symbols called e.g. `mpiwrapper_MPI_COMM_WOLRD` with
names we control. `mpiwrapper` is the shared library that we actually
open via `dlopen` and which pulls in the MPI implementation.

These are 64-bit integers in the MPI ABI and may be either 32-bit or
64-bit integers or pointers in the MPI library. The 64-bit ABI
representation should be defined essentially as `(uint64_t)handle`,
with possibly additional casts to make this legal in C.

There are many predefined handles with fixed values in the MPI ABI.
These need to be converted to the MPI library's values which may not
be constant. To ensure that we know the linker's symbol names for the handles, create 

### MPI status

`MPI_Status` is a 32-byte object in the ABI with certain named `int`
fields (`MPI_SOURCE`, `MPI_TAG`, and `MPI_ERROR`) and additional
private fields. The same type may be smaller in the MPI
implementation. Convert status objects from the MPI library to the ABI
in the following way:

- first copy the three named fields
- then copy the remaining bits, using the ABI's private fields
- the ABI status may be only partially defined

Converting from the ABI to the MPI implementation is the reverse
procedure.

### Sentinel values (special pointers)

Some pointer values have special meaning, e.g. `MPI_BOTTOM` or
`MPI_IN_PLACE`. These have fixed values in the ABI and may be
non-constant in the MPI implementation. (They might be defined as
`extern void *`, which makes them non-constant at build time but
constant at link/run time.)

### Other constants (e.g. error codes)

These have fixed values in both the ABI and the MPI implementation,
but their values differ.

### Callbacks

Callbacks need to be wrapped too. Most MPI API's have an "extra args"
argument that we can use to store the pointer of the MPI ABI
compatible callback which the user passes and that we need to call.
User-defined operators `MPI_Op_create` are an exception, and there we
need to use a finite list of trampolines.

## Testing

Test against the latest releases of MPICH and OpenMPI. MPICH comes
with a test suite that can be called externally; mpif (see below)
already does this for the Fortran bindings.

## Other considerations

The MPI standard defines many functions. Use a Python code generator
built on the python library https://github.com/mpi-forum/pympistandard
which defines the MPI API in a machine readable way.

I had a previous high-level conversation with Claude about this
design. See `APPROACH.md` for the result.

MPI applications can be multi-threaded. The code needs to be
thread-safe. Prefer atomic operations since they do not require a
support library.

This code will be built on many systems with different operating
systems and architectures. Try to be portable, but compiler-specific
mechanisms are probably fine. Support Linux and MacOS at least,
FreeBSD and Windows (mingw) if possible.

On MacOS, it is okay to assume that the MPI library uses a two-level
namespace. Check this at configure time if you need to make this
assumption.

Support cross-compiling. You cannot run executables at configure time.



## References

The file `doc/mpi50-report.pdf` contains the MPI 5.0 standard which
defines both the API and the ABI.

The project [mpif](https://github.com/eschnett/mpif) provides MPI
Fortran bindings based on the MPI ABI. There are similarities (both
generate code for the MPI standard). mpif is also a consumer of this
project. It is okay to request changes/updates/corrections from the
mpif project.
