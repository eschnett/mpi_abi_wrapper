# Brief for the consistency review

Written at the end of the design session that produced `NOTES.md`, after S0 and S1
had run. Its purpose is to hand over the one thing that dies with that session's
context: **which claims were superseded, and when.** A reader coming to `NOTES.md`
fresh sees a coherent document and has no way to tell a current statement from a
fossil that survived an update.

Do the review in a **fresh session**. The author of a document is its worst
proofreader, and that is sharpened here: the design session wrote every superseded
version too, so on hitting a stale sentence it auto-corrects to what it believes
rather than noticing the contradiction.

## Known fault lines

Four conclusions were overturned by S1. Every place that touches one is a candidate
fossil.

| overturned | by | old claim still findable? |
|---|---|---|
| `dlmopen` is a usable Linux mode | `9c844c2` — segfaults in `MPI_Init` with any MPI that `dlopen`s components | **yes, see below** |
| the `dladdr` isolation check is sufficient | `14ec73d` — dyld coalesces weak definitions; needed a behavioural probe, which is itself incomplete (`bf8c387`) | **yes, see below** |
| the floor is MPI-4.0 | `928b938` — verified MPI-3.0 with MPICH 3.1.4 | partly; §1 and decision 3 still frame MPI-4.0 as the expectation |
| `MPI_ABI_VERSION` renames to `MPIABI_VERSION` | `21be31d` — plain prefix-strip gives `MPIABI_ABI_VERSION` | fixed in `mpiabi.h`; **had left a live defect in `mpiwrapper_convert.c`**, corrected here |

## Confirmed inconsistencies, not yet fixed

Left for the review because fixing them well needs S1's context, not this session's.

1. **`NOTES.md` §2 contradicts itself about `dlmopen`.** The probe-results table
   (~line 352) and the per-platform table (~line 382, "both measured sufficient …
   `dlmopen(LM_ID_NEWLM)` selectable") are followed at ~line 392 by "`dlmopen` does
   not survive contact with a real MPI". The later text supersedes, but a reader
   meets the tables first and they read as current guidance. The tables are accurate
   *about the mock* — so the fix is scoping, not deletion.
2. **`NOTES.md` ~line 375 still credits the `dladdr` check** ("the check survives
   that mode too") while §2's later section (~486–546) establishes that it cannot
   see a capture where the sampled call binds outward.
3. **`NOTES.md` ~line 386–389** gives "the sanitizer CI jobs are the concrete reason
   `dlmopen` has to stay available", which S9 can no longer rely on. `928b938` gives
   the replacement: an MPI configured with its components built in.

## Fixed here

- `examples/mpiwrapper_convert.c` compared the ABI protocol version against
  `MPIABI_VERSION` (= 5, the MPI *standard* level) instead of `MPIABI_ABI_VERSION`
  (= 1). It rejected every valid pairing, and compiled cleanly because both macros
  exist, so `examples/check.sh` could not catch it. This is the fossil class in its
  purest form: a rename landed in the definition and not in the consumer.
- `STAGES.md` S0/S1 marked done; the prototype size corrected from 15 to 29 in
  `STAGES.md` and in `NOTES.md` decision 17.
- `dev/dlopen-probe/README.md` gained a scope section recording that its `dlmopen`
  row does not generalise, and why the mock could not have seen it.

## What to check that nobody has

- **`examples/` against `src/`.** Since S1 the examples are narrated excerpts and
  `src/` is the reference. `check.sh` proves they compile, not that they still agree
  with the tested implementation. Three divergences were already found by S1 this
  way; the version-macro defect above was a fourth, found only by reading.
- **Every numeric claim in `NOTES.md`** against `dev/entrypoints.txt` and the frozen
  tallies. Counts drifted twice in this design's history (688 vs 664, 26 vs 28
  converters).
- **The decision list against the sections.** Decisions were edited in place as
  things changed; a decision whose §-reference no longer says the same thing is the
  most likely remaining fossil.
- **Anything asserted rather than measured.** `dev/` holds five probes now. A claim
  about loader behaviour, performance, or an implementation's internals that has no
  probe behind it is where the next S1-style surprise will come from.
