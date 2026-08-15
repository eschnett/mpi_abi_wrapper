# Is `max_datatypes` an upper bound?

MPI-5.0 §5.1.13 makes `MPI_TYPE_GET_CONTENTS`' three `max_*` arguments upper
bounds: each "must be at least as large as" the count `MPI_TYPE_GET_ENVELOPE`
reported, and the call writes that many entries. Passing an array with room to
spare is legal, and so is a wrapper forwarding whatever the caller passed.

`probe.c` asks whether that is true in practice. It is not.

```sh
cc -I$MPI/include -L$MPI/lib -o probe probe.c -lmpi   # add -lpmpi for MPICH
./probe
```

## Results

MPICH 4.3.1 (conda-forge, osx-arm64) and Open MPI 5.0.6 (built from source),
2026-08-15:

| `max_datatypes` | MPICH | Open MPI |
|---|---|---|
| exactly the envelope's count | returns `MPI_SUCCESS`, leaves the tail alone | returns `MPI_SUCCESS`, leaves the tail alone |
| larger, tail zeroed | returns `MPI_SUCCESS` | **SIGSEGV inside `MPI_Type_get_contents`** |

Open MPI 5.0.6 walks the whole of `max_datatypes` and dereferences each entry
it finds — it is retaining the caller's *previous* array contents, which for an
OUT parameter are whatever happened to be in the caller's memory. The faulting
address tracks the fill value (`0x10` for a zeroed array, `0x8f` for a `0x7f`
one), which is what identifies it as a dereference of the array's existing
contents rather than of anything the call produced. Nothing in the standard
lets it read those bytes at all.

## What the wrapper does about it

`dev/generate.py`'s array-extent table passes the *envelope's* count as
`max_datatypes` rather than the caller's, and stages an array of that size —
see `ARG_SUBSTITUTE` there. That is conforming, because the envelope's count
satisfies "at least as large as" exactly; it is what the implementation was
going to write either way; and it removes the whole class of "the
implementation read what we never wrote", which also matters under a sanitizer
where our staged array's tail is genuinely uninitialized.

A caller that passes a `max_datatypes` *smaller* than the envelope's count is
in error, and the substitution preserves that: the clamp is `min(envelope,
caller's)`, so the implementation still sees the too-small value and still
rejects it.

This is not the same failure as a wrapper bug and it is worth keeping apart
from one: a native Open MPI program that passes a generous `max_datatypes`
crashes identically, with no wrapper involved.
