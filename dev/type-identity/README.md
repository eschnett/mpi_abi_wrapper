# Do the ABI's integer types and the implementation's have to be staged?

`NOTES.md` §5.7 briefly said yes: stage any array whose element type is not
*identical* on both sides, even when no value mapping is needed, because the
ABI's `MPI_Aint` and an implementation's may be distinct C types. That was
reasoning from the language and it was wrong. This measures the two things it
should have measured first.

```sh
dev/type-identity/run.sh /path/to/mpi/include "label"
```

## What the ABI actually fixes

Representation, not spelling. Measured on macOS 26/arm64 (Apple clang) and
Ubuntu 24.04/aarch64 (gcc 13):

| implementation | `MPI_Aint` | `MPI_Count` | `MPI_Offset` | `MPI_Fint` |
|---|---|---|---|---|
| MPICH 4.3.1, macOS | same type | same type | same type | same type |
| Open MPI 5.0.6, macOS | same type | same type | same type | same type |
| Open MPI 6.1.0a1, macOS | same type | same type | same type | same type |
| MPICH 4.1, Linux | same type | **`long long` vs `long`** | **`long long` vs `long`** | same type |
| Open MPI 4.1, Linux | same type | **`long long` vs `long`** | **`long long` vs `long`** | same type |

Size and signedness are identical in **every** case. The Linux rows differ only
because glibc's `int64_t` is `long` while both MPIs spell their `MPI_Count` as
`long long`; on macOS `int64_t` is itself `long long` and even the spellings
agree. `MPI_Aint` is `long` on both sides everywhere, so the case that prompted
the original rule -- `intptr_t` against `long` -- does not arise at all: the ABI
header's `intptr_t` *is* `long` on every LP64 target.

So the difference is a C type-identity difference, which costs a cast:

```
nocast.c:13:27: warning: passing argument 2 of 'reader' from incompatible
                pointer type [-Wincompatible-pointer-types]
```

and nothing else. On macOS there is not even a diagnostic.

## Whether the cast is safe

The worry worth testing is type-based alias analysis: an array written as one
type and read through a pointer to another is a strict-aliasing violation on
paper, even when the layout is identical. `aliasing.c` is the *worst* case, not
the realistic one -- a single translation unit at `-O3 -fstrict-aliasing`, where
the optimizer sees both the stores and the loads and may use TBAA freely, with
values that need all 64 bits.

Correct at `-O0`, `-O2` and `-O3`, with both compilers, on both platforms. In
the real system the implementation is a separate shared library that cannot see
our stores at all, so the compiler has strictly less to work with than it does
here.

## Conclusion

Stage an array when the elements need **value mapping** (handles, sentinels) or
when the **representation** differs (the status blob). A difference in spelling
is handled by a cast, and the `_Static_assert`s in `src/mpiwrapper/internal.h`
on the size and signedness of `MPI_Aint`, `MPI_Count`, `MPI_Offset` and
`MPI_Fint` are what license it: a target where the representations genuinely
differed would fail the build rather than reach this reasoning.
