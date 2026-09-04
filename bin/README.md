# `bin/` — the programs an installed prefix holds

Templates, not programs: CMake configures each `*.in` at *this project's*
configure time and installs the result into `<prefix>/bin`. `CODE.md` §9 has
the consumption routes these serve; `NOTES.md` §9 and decisions 19 and 27 have
why each exists.

| installed as | from | |
|---|---|---|
| `mpicc` | `mpicc.in` | compile and link against this prefix's `mpi.h` and `libmpi_abi`, rpath set so the result starts unaided |
| `mpicxx`, `mpic++` | `mpicxx.in` | the same for C++, when the build had a C++ compiler. Two names because `AX_MPI` and several build systems probe `mpic++` first |
| `mpiexec`, `mpirun` | `mpiexec.in` | forward to the wrapped MPI's own launcher |
| `mpi_abi_wrapper_info` | `src/tools/mpi_abi_wrapper_info.c` | what this prefix was configured to do, and what it resolves to now |

**There is no `mpifort`.** This project has no Fortran bindings; they are the
sibling `mpif` project's (`ci-scripts/test-mpif.sh` runs mpif's own suite over
a prefix built here). A stub that errored would be worse than the absence,
because a build system that probes for the file would conclude Fortran works.

**There is no `mpiCC`.** On a case-insensitive filesystem — macOS's default —
`mpiCC` and `mpicc` are one directory entry, so installing both means one
clobbers the other. MPICH ships `mpic++` and no `mpiCC` for the same reason.
`ci-scripts/check-install.sh`'s inventory leg carries the oracle: `mpicc
-showme:command` and `mpicxx -showme:command` must not name the same compiler.

**There is no launcher under `-DMPI_ABI_BUILD_WRAPPER=OFF`**, which builds
`libmpi_abi` alone with no MPI present. There is nothing for a launcher to
forward to, and an `mpiexec` that always errored would let a probe conclude the
prefix can launch. The compiler wrappers and the info tool are installed there.

## Environment variables these read

Each is an override of a value baked in at configure time, and
`mpi_abi_wrapper_info` prints both halves plus which one is in force.

| | |
|---|---|
| `MPI_ABI_WRAPPER_MPIEXEC` | which launcher `bin/mpiexec` and `bin/mpirun` exec; overrides the path found beside `MPI_C_COMPILER` at configure time |
| `MPI_ABI_CC`, `MPI_ABI_CFLAGS`, `MPI_ABI_LIBS` | `bin/mpicc`'s compiler and flag inventory |
| `MPI_ABI_CXX`, `MPI_ABI_CXXFLAGS` | `bin/mpicxx`'s |

`src/mpi_abi/README.md` has the variables `libmpi_abi` itself reads —
`MPI_ABI_WRAPPER_LIB` above all — which are a separate set: they are read by
the *program* these wrappers produce, not by the wrappers.

## Why `mpiexec` here forwards rather than launches

The MPI ABI is a library ABI, so the program that starts a job belongs to the
implementation. That is why this prefix had no `mpiexec` at all until decision
27, and the argument was sound as far as it went: a launcher baked in at build
time would name one MPI, while decision 5 lets one `libmpi_abi` be pointed at
any wrapper at run time.

What it missed is that decision 5 had already solved exactly this problem for
`libmpiwrapper` — name the thing, resolve it at run time, environment variable
first and a configure-time absolute path second — and that a prefix calling
itself an MPI while having no `mpiexec` is half an installation to every tool
that treats an MPI as a prefix. So `bin/mpiexec` uses decision 5's mechanism
for decision 5's reason, and stays correct under re-pointing.

Two things it deliberately does not do:

- **No argument parsing.** Every launcher option this script does not
  understand is one that keeps working — `--`, `-genv A B`, a `:` multi-app
  command line, `--version`, `--help`. Only a literal `-showme:launcher` or
  `-showme:version` as the first argument is intercepted, matched exactly
  rather than by a glob, since a launcher may have a `-show…` flag of its own.
  `ci-scripts/suite/mpiexec-filter` is the thing that *does* parse, and its
  header says why it has to.
- **No environment rewriting.** `dev/launcher-env-forwarding/` measured it:
  both launcher families forward a plain variable to single-node ranks unasked,
  and hydra rejects `-x` outright, so the feature would have broken the
  implementation that needed no help. Across nodes the answer may differ —
  state the variable with the launcher's own flag there.
