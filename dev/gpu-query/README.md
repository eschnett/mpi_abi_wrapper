# `dev/gpu-query/`

The measurements behind `NOTES.md` #7 decision 28 — the five non-standard
GPU-support queries — and behind `HISTORY.md` #1.26, the optional-header
pre-check. Two scripts, both read-only; neither is run by the build or by
`ctest`.

```sh
python3 dev/gpu-query/absent.py      # what each configured build tree's probe found
python3 dev/gpu-query/timeprobe.py   # what the mpi-ext.h pre-check costs
```

`absent.py` reads the `mpiwrapper_impl_config.h` of `build/mpich`,
`build/openmpi` and `build/ompi-main-abi` and prints, per row, how many
`MPIWRAPPER_HAVE_*` guards were asked, how many were answered, and the names
that were not. It needs those three build trees to exist under the repository
root and edits nothing. `timeprobe.py` names this laptop's compiler and its
native Open MPI prefix, which is what makes it re-runnable here rather than
portable; `CODE.md` §8 has how each tree is configured.

## What they said, 2026-09-06, on the development laptop

`absent.py`, over 732 guards:

| row | available | absent | `mpi-ext.h` |
|---|---|---|---|
| conda MPICH 4.3.1 | 722 | 10 | no |
| native Open MPI 5.0.6 | 488 | 244 | yes |
| Open MPI main `--enable-standard-abi` | 717 | 15 | no |

MPICH's ten are the five sized Fortran logicals, `MPI_ERR_ABI`,
`MPIX_TYPECLASS_LOGICAL`, `FORTRAN` (a CMake-set guard the probe cannot answer
and does not need to — `CODE.md` §4) and the two `rocm` spellings, which it
calls `hip`. It lacks no standard entry point. Open MPI 5.0.6's 244 are the
`_c` forms, `MPI_T`'s events, the MPI-4.1 buffer forms and eight of the GPU
family. The ABI-implementing row's fifteen are `MPIX_TYPECLASS_LOGICAL`,
`FORTRAN` and all thirteen names of the GPU family — nothing of the standard.

**That last row is the one worth keeping.** Its `libmpi_abi` *declares* the
standard's 688 and defines 683, which is `NOTES.md` §3's hazard, and the GPU
queries would have been the same hazard again had the probe seen them:

```
$ build/mpi/ompi-main-prefix/bin/mpicc_abi -show
gcc -std=gnu23 -I.../ompi-main-prefix/include/standard_abi -L.../lib -lmpi_abi

$ find build/mpi/ompi-main-prefix -name mpi-ext.h
build/mpi/ompi-main-prefix/include/mpi-ext.h        # not include/standard_abi

$ nm -gU build/mpi/ompi-main-prefix/lib/libmpi_abi.dylib | grep -c ' T '
1341
$ nm -gU build/mpi/ompi-main-prefix/lib/libmpi_abi.dylib | grep -ci mpix
0
```

So the header is off that build's include path, the probe reports it absent,
and the wrapper names none of the ten symbols that library declares nowhere and
defines nowhere. Had the header been reachable, all ten would have been link
failures of the whole wrapper rather than run-time answers.

`timeprobe.py`, against native Open MPI 5.0.6:

```
pre-check (mpi-ext.h present=True): 34 ms
whole probe, 1350 spellings: 316 ms, headers=['mpi-ext.h']
```

One `-fsyntax-only` of a two-line translation unit, about a ninth of the
probe's own cost, once per build tree. `HISTORY.md` #1.26 is why that is paid
rather than answered for free with `__has_include`.
