# Consistency review — brief, and outcome

Written at the end of the design session that produced `NOTES.md`, after S0 and S1
had run, to hand over the one thing that dies with that session's context: **which
claims were superseded, and when.** The review it asked for has now run, in a fresh
session; this file records what it found so that the next reader does not repeat it.

**This file is a working note, not a durable document.** `NOTES.md` and `STAGES.md`
are the pair that survives. Once the "still open" list below is empty, delete this.

---

## Outcome of the review

### Fixed in `NOTES.md` / `STAGES.md`

*Contradictions — two statements in the design that cannot both be acted on:*

- **§2 "Bootstrap" required an atomic acquire-load guard** on the vtable pointer,
  which the *next* subsection, decision 8 and `src/mpi_abi/bootstrap.c` all reject.
  Rewritten to the constructor-only shape that is implemented, with the reason the
  guard is unnecessary.
- **§2 "Locating the wrapper" still name-tagged the wrapper** (`libmpiwrapper-mpich-4.3.so`)
  against §9 and decision 5, which drop it. Rewritten.
- **§8 listed the staged-temporary forms as hand-written (S4)** while §3, §5.7 and
  S3 treat staging as a generated argument class. Resolved in favour of S3: the set
  is exactly eight `*alltoallw*` forms, generated, with S1's `MPI_Ialltoallw` as
  the template.
- **§9's "Open MPI 4.1 fails the MPI-4.0 minimum, so distro LTS does not serve"**
  was the pre-S1 rationale; §1 now records that no *released* Open MPI has `_c`
  entry points, so the reason had stopped applying while the conclusion stayed.
  Replaced with a table of what each version row is actually for.
- **`STAGES.md` S9 still planned `dlmopen` as the sanitizer fallback**, which §2
  had already killed, and cited §13 for a risk that lives in §12.

*Counts, each checked against the artifact that decides it:*

| claim | was | is | authority |
|---|---|---|---|
| deprecated entry points | 31 | **12** | `grep '; /\* deprecated' gen/include/mpi.h` |
| predefined handles (§4.1, §10) | 104 | **103** | the header; `PREDEF(...)` rows in `constants.c` |
| error classes (§11) | 81 | **80** | 62 `MPI_ERR_*` + 18 `MPI_T_ERR_*`; `MPI_ERR_LASTCODE` is a bound |
| callback registration functions (§6.1, §6.2) | 16 | **15** | the section's own table |
| hand-written set (§3, §8, S4) | ~50 | **~90** | §8's own list, added up |
| S1 prototype (§11) | 28 entry points, 56 slots, 19 generated-shape | **29 / 58 / 20** | `src/include/mpiwrapper_vtable.h`, `dev/layout_hash.py` |
| S1 stand-in files (§11) | "three" | **four** | they are named on the same line |
| planned prototype size (§11, decision 17) | fifteen | **sixteen** | the table has sixteen functions in thirteen rows |

The §11 additions list also omitted `MPI_Op_create` and `MPI_Get_version`, which is
where the 28 came from.

*Fossils the brief predicted, now scoped rather than deleted:*

- the `dev/dlopen-probe` results table and the per-platform table now say they are
  statements about the mock's loader behaviour, with the real-MPI `dlmopen` failure
  named where a reader meets them;
- "the check survives that mode too" is qualified: the `dladdr` check was later
  found to answer a different question, and even the behavioural probe is
  incomplete;
- "the sanitizer CI jobs are the concrete reason `dlmopen` has to stay available"
  is marked as superseded, with S9's actual options.

*One claim that was contradicted by a later measurement in the same section:* §2
said that if a native macOS build with weak `MPI_*` ever appeared the wrapper would
refuse it — and native Open MPI 6.1.0a1 is exactly that and wraps correctly. The
symbol table now carries both 6.1.0a1 builds and says plainly that strong `MPI_*`
is sufficient, not necessary.

### Fixed elsewhere

- `CMakeLists.txt`, `src/include/README.md`, `src/mpi_abi/README.md`,
  `src/mpiwrapper/README.md`, `src/mpiwrapper/handwritten.h`,
  `src/mpiwrapper/constants.c`: the 28/56/104/~50 counts above.
- `src/mpi_abi/bootstrap.c` comments: the decoy narration still described
  `MPI_Wtime` as the probe (it is `MPI_Get_version`, and *every* decoy slot points
  at the recorder); it named `test/isolation_test.c`, which does not exist
  (`test/check_isolation.cmake`); and it repeated the sanitizer/`dlmopen` claim.
- `examples/`, which the brief asked to be read against `src/`:
  - `mpi_abi_side.c` **defaulted to `dlmopen` on Linux** — a fifth divergence, and
    the sharpest, since `src/` defaults to `RTLD_DEEPBIND` and `dlmopen` is now
    known not to work with a real MPI at all;
  - `mpiwrapper_vtable.h` and `mpiwrapper_wrappers.c` still asserted the
    weak-alias/no-separate-profiling-library shape that S1 disproved on macOS;
  - `mpiwrapper_convert.c`'s 104s;
  - the header excerpt said seven slots of 688 and "1374 more" where it shows nine
    of 1376.
  - `examples/README.md` now lists all five divergences found so far, since the
    pattern — *compiles, therefore unchecked* — is the point.

## Settled after the review

The three things the review left open were decided immediately afterwards, in
`src/mpiwrapper/getvtable.c` and §2:

1. **The vtable handshake requires exact equality on all four fields**, `size`
   included. "Accept a smaller size and serve the common prefix" is gone: it was
   unreachable behind the layout hash, and a provision no input can reach is a
   story about forward compatibility rather than the thing itself. The size check
   stays because it is the only one that catches a mismatch the hash cannot — the
   hash is over the slot list's *text*, so a 32-bit `libmpi_abi` against a 64-bit
   `libmpiwrapper` hashes identically and differs in `sizeof`.
2. **The comment over the map construction now says what the CAS actually
   guarantees** — once, not a barrier — and names the two things it does not
   cover (a second concurrent caller, and a failed build leaving `initialized`
   set), why neither can happen with a constructor as the only caller, and what
   would have to change if a second entry point ever appeared. The code is
   unchanged; the claim was the defect.
3. **The capture diagnostic advises `dlopen(RTLD_LOCAL | RTLD_DEEPBIND)`**, the
   actual default, and says that `dlmopen` is selectable but does not work with
   an MPI that `dlopen`s its components.
## What is worth re-checking next time

- **Anything asserted rather than measured.** `dev/` holds five probes. A claim
  about loader behaviour, performance, or an implementation's internals with no
  probe behind it is where the next S1-style surprise will come from.
- **The decision list against the sections.** It was edited in place as things
  changed; this pass found decisions 3, 7, 8 and 17 out of step with their own
  §-references, and the numbering itself out of order.
- **Every count, against the artifact rather than against another sentence.**
  Eight of them were wrong, and each was one `grep` from being right.
