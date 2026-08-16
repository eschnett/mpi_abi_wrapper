# `examples/`

Worked examples of the shapes the generator has to produce, and of the hand-written
runtime they call into. Illustrative, not part of the build — but they *do* compile,
and `check.sh` compiles them, because an example that has never been through a
compiler is a guess.

**Since S1 these are narrated excerpts, not the reference.** The reference is
`src/`, which compiles against two implementations and passes tests; a shape that
exists in both places is a second source of truth, and the tested one wins.

Everything found wrong here so far, as a warning about how this file drifts —
five of the six were invisible to `check.sh`, because compiling proves nothing
about agreement (`HISTORY.md` §5 keeps the same list as a standing warning):

1. the reverse-map tables were `static`, which does not compile against an
   implementation whose handles are addresses;
2. the bitmask mapper needed splitting by role (`filemode` vs `winassert`);
3. the `dlopen` narration predated the `RTLD_LOCAL`-plus-isolation correction;
4. `mpiwrapper_convert.c` compared the ABI protocol version against
   `MPIABI_VERSION` (the MPI standard level, 5) instead of `MPIABI_ABI_VERSION`
   (the handshake version, 1) — it rejected every valid pairing and compiled
   cleanly, because both macros exist;
5. `mpi_abi_side.c` still *defaulted* to `dlmopen` on Linux, which `src/` never
   did and which is now known not to work with a real MPI at all;
6. the slot count said 1376 where the real vtable has 1366 — the five entry
   points MPI-3.0 deleted are answered by `libmpi_abi` itself and have no slot,
   while all 1376 names are still exported (`NOTES.md` #1, #3).

All six are corrected. What is still deliberately absent here is the
behavioural probe (`src/mpi_abi/bootstrap.c`'s decoy vtable): the `dladdr` check
narrated below is necessary and not sufficient, and `NOTES.md` §2 explains why.

| file | corresponds to | written by |
|---|---|---|
| `mpiabi.h` | `gen/include/mpiabi.h` | generator |
| `mpiwrapper_vtable.h` | `gen/include/mpiwrapper_vtable.h` | generator |
| `mpi_abi_side.c` | `src/mpi_abi/bootstrap.c` + `gen/mpi_abi/entrypoints.c` | hand + generator |
| `mpiwrapper_wrappers.c` | `gen/mpiwrapper/wrappers.c` | generator |
| `mpiwrapper_convert.c` | `src/mpiwrapper/*.c` | hand |

The headers are excerpts: nine vtable slots of 1366, and only the types and constants
the examples reference.

## Checking them

```sh
./check.sh
```

It needs two include directories and finds them itself if `mpif`'s build tree is
present:

- an **ABI** `mpi.h` (the mpi-abi-stubs header), for `mpi_abi_side.c`
- an **implementation** `mpi.h`, for the two wrapper files

Override with `ABI_INCLUDE=` and `MPI_INCLUDE=`. Both must be *different* headers:
compiling the wrapper side against the ABI header is the self-wrapping mistake that
`CMakeLists.txt` refuses at configure time.

## What each example is for

**`mpi_abi_side.c`** — that the ABI side contains no conversion at all. Every entry
point is one line, arguments pass through without a cast, and `MPI_X`/`PMPI_X` are
two definitions reaching two *different* slots. The interesting code is the
bootstrap: the environment variable, why the load must be `RTLD_LOCAL` **plus**
active isolation (`RTLD_DEEPBIND` or `dlmopen` on Linux, the two-level namespace on
macOS) and never `RTLD_GLOBAL`, why the binding mode defaults to `RTLD_LAZY`, and
the version/hash/size handshake. There is no lazy guard: the constructor-ordering
argument in `src/mpi_abi/bootstrap.c` explains why one is unnecessary and
`dev/dispatch-bench/` says what it would cost.

**`mpiwrapper_wrappers.c`** — the generated body shapes, in increasing difficulty:
`MPI_Send` (the base case, and why `dest` and `tag` go through different
functions), `MPI_Waitall` (staged arrays, `MPI_STATUSES_IGNORE`, and why write-back
must happen on the error path), `MPI_File_open` (an out-handle local and a bitmask),
`MPI_Error_string` (output-string staging and truncation).

**`mpiwrapper_convert.c`** — the runtime: the dense ABI→implementation switch, the
initialization-time reverse map and why it cannot be a switch, the rank/tag split,
the status blob with its `_Static_assert`s, the bitmask decomposition, the staging
helper, and the op trampoline pool with the one place a slot may be released.

The full reasoning for every choice is in `../NOTES.md`; the examples carry only
the reasoning you need while reading the code in front of you.
