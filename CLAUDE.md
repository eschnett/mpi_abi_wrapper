# MPI ABI wrapper — how to work here

This file is loaded into every session, so it stays short: the reading
protocol, this host's quirks, and the session rules. Everything else lives in
three sibling documents, read by the section a task names:

| | holds |
|---|---|
| **`CLAUDE.md`** | how to work here: the reading protocol, the host, the session rules |
| `CODE.md` | what the repository contains now, and the number behind every claim |
| `NOTES.md` | the design, its reasons, and what is missing, broken or undecided |
| `HISTORY.md` | roads not taken, beliefs that were measured false, and the stage record |

`TODO.md` is Erik's open-task list.

## Reading protocol

- **Read sections, not files.** `NOTES.md` alone is ~2,300 lines; loading it
  whole spends the context the work needs. Find the heading's line with
  `grep -n '^#' <file>.md`, then read that range only.
- **"Where is X?"** — `CODE.md` §3 (layout) and `gen/report.txt` answer most of
  these on their own; `CODE.md` §2 has every count, each with an authority.
- **"Why is X this way?"** — the `NOTES.md` section the code cites. Roughly two
  hundred source comments cite `NOTES.md #5.7`-style numbers, so **§1–§13 keep
  their numbers and topics permanently — never renumber them.** The map:
  §1 goal and scope · §2 architecture and loader isolation · §3 the generator ·
  §4 verified facts about the ABI and the two implementations · §5 conversion
  rules · §6 callbacks and lifetimes · §7 the numbered decisions · §8 the
  hand-written set · §9 building · §10 testing · §11 sequencing and remaining
  work · §12 risks · §13 missing, broken, or undecided.
- **"Was Y tried?"** — `HISTORY.md` §1 (abandoned approaches) and §2 (beliefs a
  measurement overturned), *before* designing anything new. At least three of
  its entries are the first idea a reader invents on meeting the problem.

## This host

- **`source scripts/host-env.sh` before running anything MPI on this laptop.**
  It exports the four variables that keep MPICH and Open MPI off the VPN
  interface and past the application firewall; without them 6 of 13 tests fail
  for reasons that are the machine's, not the project's (`CODE.md` §12,
  `test/README.md`).
- **Do not benchmark MPI progress on this laptop.** `host-env.sh`'s own
  `FI_PROVIDER=tcp` makes every MPICH progress poll three orders of magnitude
  slower than the container rows, which are the numbers (`HISTORY.md` §2.17).
- **Name the Fortran compiler on this laptop.** MacPorts installs
  `gfortran-mp-15`, CMake's plain search looks for `gfortran`, and both local
  MPIs' `mpifort` name a conda `gfortran` that is not installed — so with
  `MPI_ABI_FORTRAN` at its default `ON` a configure here stops until told
  `FC=/opt/local/bin/gfortran-mp-15` (or `-DCMAKE_Fortran_COMPILER=`). That is
  the option working, not misfiring: before it existed this host quietly built
  no Fortran probe at all (`NOTES.md` §9).
- One build directory per MPI, selected by `-DMPI_C_COMPILER`:

  ```sh
  cmake -S . -B build/mpich -DMPI_C_COMPILER=/path/to/mpicc \
      -DCMAKE_Fortran_COMPILER=/opt/local/bin/gfortran-mp-15
  cmake --build build/mpich -j8
  ctest --test-dir build/mpich --output-on-failure
  ```

  `CODE.md` §8 has the options, §10 what each test establishes; the MPICH C
  suite runs via `ci-scripts/suite/run-suite.sh`.
- **Generated code is committed.** After touching `dev/generate*.py`, run the
  `regenerate` target and commit the diff; `ctest -R generated-up-to-date`
  must pass.

## Session rules

- **Do not re-litigate the numbered decisions** (`NOTES.md` §7). They have
  reasons recorded, several of them measured. Reopen one only with a new
  argument or a new measurement, and then update the decision rather than
  working around it.
- **New findings go in the section that owns the rule, not only in a stage
  narrative.** This is the failure mode a review found three times: S7's
  `status.MPI_ERROR` rule, its displacement sentinel and its attribute-value
  class were all recorded accurately in a per-stage account and left out of
  `NOTES.md` §5, which is the section a later session actually reads. A finding
  is not filed until the rule it changes says so.
- **New measurements go in `dev/`.** A claim in a commit message is a claim
  nobody will find again.
- **Prefer a benchmark or a probe to an argument** for anything performance- or
  loader-related, and **check any benchmark against its own disassembly** —
  three benchmarks here have reported confidently wrong numbers first
  (`HISTORY.md` §2.11, §2.17).
- **Check every count against the artifact, not against another sentence.**
  Eleven have been wrong, each one `grep` from being right. `CODE.md` carries
  an authority column for this reason; if a number is worth writing down, write
  down how to re-derive it.
- **Commit message subjects are imperative.** "Add the FreeBSD row", not "The
  FreeBSD row, added" — the subject completes *if applied, this commit will
  ___*, so `git log --oneline` reads as a list of what each commit does. Only
  the subject's mood is fixed by this; the body keeps the documents' style,
  which is the reason for the change, the measurement behind any claim in it,
  and what was verified.
- **End committed and green.** Work that leaves the tree red has not produced
  something the next session can build on.
