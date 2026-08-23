# Is a datatype's envelope a property of the type, or of the constructor?

MPI-4.0 gave every datatype constructor a large-count twin, gave
`MPI_TYPE_GET_ENVELOPE` a fourth count (`num_large_counts`), and gave
`MPI_TYPE_GET_CONTENTS_C` the matching `array_of_large_counts`. Two readings are
available:

- **(a) the envelope describes the type.** A count that fits in an `int` comes
  back from `array_of_integers` however the type was built, and
  `num_large_counts` is nonzero only for a value that genuinely does not fit.
- **(b) the envelope describes the constructor.** A type built by
  `MPI_Type_contiguous_c` reports its count in `array_of_large_counts` even when
  the count is 5, and one built by `MPI_Type_contiguous` never does.

This decides whether this project's narrowing fallback is observable. Where an
implementation has no `_c` constructors, we serve `MPI_Type_contiguous_c` by
calling `MPI_Type_contiguous` with the count narrowed to an `int` (`NOTES.md`
§5.10). Under (a) that is indistinguishable from a native large-count
implementation. Under (b) it is not.

`probe.c` asks MPICH, which has both halves. Open MPI 5.0.x has neither and
cannot answer.

```sh
cc -I$MPI/include -L$MPI/lib -o probe probe.c -lmpi   # add -lpmpi for MPICH
./probe
```

## Results

MPICH 4.3.1 (conda-forge, osx-arm64), 2026-08-22. **It is (b), and MPICH says so
in words.**

| built by | `envelope_c` | `envelope` (small) |
|---|---|---|
| `MPI_Type_contiguous(5, MPI_INT)` | 1 integer, 0 large counts | 1 integer — succeeds |
| `MPI_Type_contiguous_c(5, MPI_INT)` | **0 integers, 1 large count** | **refused** |
| `MPI_Type_create_struct(2, …)` | 3 integers, 2 addresses, 0 large counts | 3 integers, 2 addresses — succeeds |
| `MPI_Type_create_struct_c(2, …)` | **0 integers, 0 addresses, 5 large counts** | **refused** |
| `MPI_Type_contiguous_c(INT_MAX + 7, MPI_BYTE)` | 0 integers, 1 large count | refused |

A count of 5 fits in an `int` with room to spare, so nothing about its magnitude
puts it in the large half. The constructor does.

The refusal is `MPI_ERR_TYPE` (class 3, *not* `MPI_ERR_ARG`), and MPICH's error
stack states the rule outright:

> `MPIR_Type_get_envelope_impl(148): use MPI_Type_get_envelope_c to query large count datatype`

Two details worth keeping beyond the headline:

- **The struct's displacements move too.** `MPI_Type_create_struct_c` reports
  *five* large counts and **zero** addresses — the count, the two blocklengths
  and the two displacements, all in `array_of_large_counts`, where the small
  form splits them across `array_of_integers` and `array_of_addresses`. So the
  large half is not "the counts"; it is every integer-valued argument.
- **A count that does not fit behaves no differently.** The `INT_MAX + 7` row
  reports the same shape as the `5` row. There is no threshold anywhere in this
  behaviour.

## What this means for the wrapper

Over an implementation with no `_c` constructors, every datatype the wrapper can
build is built by a small-count constructor, because that is what the fallback
narrows onto. So:

- `MPI_Type_get_envelope_c`'s fallback — ask the small envelope, widen, report
  `num_large_counts = 0` — is **exactly right for the types that exist in such a
  build**. There is no case where it under-reports, because there is no way to
  create a type whose contents it would have to report as large.
- `MPI_Type_get_contents_c`'s fallback is right for the same reason, and
  `array_of_large_counts` is never written.
- **The difference from a native MPI-4 implementation is real and is
  observable.** A program that calls `MPI_Type_contiguous_c(5, …)` and then
  `MPI_Type_get_envelope_c` sees `1 integer, 0 large counts` through the
  wrapper's fallback and `0 integers, 1 large count` on MPICH. It also finds the
  small `MPI_Type_get_envelope` *succeeding* on that type through the fallback,
  where MPICH refuses it.

That difference is a limitation, not a bug, and it is recorded in `NOTES.md`
§13.2 rather than worked around. Two reasons it is the right trade:

1. **It is self-consistent.** Envelope and contents agree with each other and
   with what the type actually is, so the round trip every real consumer
   performs — envelope, then contents, then rebuild — produces the same datatype
   either way. Only a program that hard-codes "a `_c`-built type reports large
   counts" can tell, and such a program is asserting about the implementation
   rather than about its own data.
2. **The alternative is worse.** Matching (b) exactly would mean the wrapper
   keeping a side table of which datatypes the caller happened to build through
   the large-count entry points, keyed on a handle the implementation owns and
   may recycle — the same unsound shape `NOTES.md` §5.2 rejects for statuses,
   bought here for a strictly cosmetic gain.

The one place this is *not* cosmetic is the direction of permissiveness, and it
runs the same way as §5.7's note about staged arrays: the fallback accepts a call
a native implementation refuses. That hides a user error rather than inventing a
wrong answer, which is the tolerable direction.
