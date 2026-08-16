# `dev/s1-reference/`

The four files S1 hand-wrote as stand-ins for generated output, frozen exactly
as S1 left them, together with the two implementations they were tested
against. They are **not compiled** and must not be edited: their whole job is
to be the thing S2's generator is measured against.

| file | what the generator emits instead |
|---|---|
| `mpiwrapper_vtable.h` | `gen/include/mpiwrapper_vtable.h` (58 slots -> 1366) |
| `entrypoints.c` | `gen/mpi_abi/entrypoints.c` (29 entry points -> 688) |
| `wrappers.c` | `gen/mpiwrapper/wrappers.c` (20 bodies -> 563 generated, the rest hand-written) |
| `constants.c` | `gen/mpiwrapper/constants.c` |

**Every number inside these four files is S1's and is not maintained.** Frozen
means frozen, so where the finished library has moved on they still say what
was true when they were written — the vtable header's "1376 slots" is the
clearest case, since the five entry points MPI-3.0 deleted did not yet have
their own answer on the ABI side and so had not yet stopped needing slots
(`NOTES.md` #3). Read counts out of `CODE.md` or `gen/report.txt`, never out of
this directory; their `STAGES.md` references are `HISTORY.md` now for the same
reason.

`dev/check_prototype.py` compares them item by item and is wired up as the
`prototype-reproduced` test, so S2's exit check keeps running rather than
having been asserted once. It compares *normalized* text — comments dropped,
macro continuations joined, whitespace collapsed — because the generator does
not run clang-format and does not write S1's per-function prose, and a byte
comparison would fail on formatting and say nothing about the code.

At the time of writing: **194 items, 190 reproduced exactly, 4 exempted**, and
each exemption names its reason in `dev/check_prototype.py`. An exemption that
stops firing fails the test, so the list cannot quietly outlive its reason —
when S3's first half generated `MPI_Waitall`, this test is what said so, and
the exemption that replaced the old one enumerates the four ways the generated
body differs from S1's, all of them naming and scoping rather than code.

Why keep them at all, rather than deleting them once the check has run once:
the check is only worth something if it can fail *later*. A generator change
that quietly stops reproducing `MPI_Send` is exactly the class of regression
this project is built to catch, and there is nothing else in the tree that
would notice.
