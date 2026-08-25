# `dev/macos-weak-symbols/`

What Mach-O's weak definitions do to this project, in the two places they
matter — and which pull in opposite directions. `NOTES.md` §13.2 states the
question; this answers half of it.

```sh
sh probe.sh          # macOS only
```

`dev/symbol-versioning/` is the ELF counterpart. There is no ELF question here:
that lookup does not distinguish weak definitions from strong ones.

## The tension

Every other implementation of the standard ABI exports `MPI_*` as `weak
external`; this project exports them `external`. Checked against the artifacts
rather than assumed:

```
ours                                     external      _MPI_Init
Open MPI's ABI branch, libmpi_abi.1.dylib   weak external _MPI_Init
```

Being weak is what the **substitution** convention requires. Being strong is
what makes our definitions win a **coalescing** contest, which is the mechanism
`HISTORY.md` §2.3 measured as a capture. So the two uses disagree, and the
question is what a change would cost.

## 1. Substitution — settled

Two libraries differing in that one attribute and nothing else, both carrying
the install name a real ABI library carries, so each is a drop-in for the other
in an installed prefix (decision 21). One client linked against each, then run
against both.

| client linked against | library present | result |
|---|---|---|
| weak | weak | runs |
| weak | **strong** | **`dyld: Symbol not found: _MPI_Init`** |
| strong | weak | runs |
| strong | strong | runs |

**Exactly one cell fails, and it is ours.** A client linked against a
weak-exporting `libmpi_abi` records a weak-def-only lookup that a strong
definition does not satisfy. The reverse is fine: a weak definition satisfies
an ordinary lookup, so a binary built against *this* project runs against a
vendor's library.

That asymmetry is the whole practical cost, and it falls on the direction that
matters most: **build against a vendor's ABI library, then run through this
wrapper over a site MPI** — which is the HPC case the ABI exists for.

## 2. Capture — *not* settled, and the probe says so

Part 2 models §2's three-library shape with an implementation exporting
`MPI_Send` weakly, and asks where the wrapper's outward call lands:

| `libmpi_abi` exports | result |
|---|---|
| strong | reached the implementation (isolated) |
| weak | re-entered `libmpi_abi` (captured) |

**This contradicts `HISTORY.md` §2.3**, which measured the *strong* case being
captured, against a real ABI-built Open MPI. A mock that disagrees with a
measurement is a broken mock or a broken memory of the measurement, and it is
not evidence either way — `HISTORY.md` §2.11 and §2.17 are two benchmarks that
reported confidently wrong numbers here before, which is why this section says
so instead of reporting a result.

**Do not act on part 2.** In particular, do not conclude from it that going
weak causes capture.

## What this does not settle, and the lead to follow

The likeliest missing ingredient is that in the real configuration the
implementation's library is **also called `libmpi_abi`** — MPI-5.0 §20.2.1
requires that name of anything implementing the ABI — so both it and ours carry
the same install-name leaf, where in part 2 the names differ.

That was tried and removed rather than left broken. What it showed before it
was cut: with both libraries carrying `@rpath/libmpi_abi.1.dylib`, the strong
row stopped reaching `dlopen` at all, failing with `Library not loaded` /
returning the probe's dlopen-failed code. That is consistent with dyld unifying
two libraries by install name and never loading the second — which would make
§2.3's capture a *library-identity* effect rather than a weak-coalescing one,
and would change §13.2's analysis substantially.

Against that reading: §2.3 reports `dladdr` resolving to the implementation,
which means both libraries were loaded and distinct. So the two accounts are
not yet reconciled, and reconciling them is the work.

Whoever picks this up: the decisive experiment is the real one, not a mock —
build this project against Open MPI's ABI branch (`--enable-standard-abi`,
which `ci-scripts/install-abi-mpi.sh` already produces) with `MPI_ABI_WRAP_ABI_IMPL`,
and read `nm -m` on both libraries plus `DYLD_PRINT_BINDINGS` on the call that
matters. §10's oracle 5 is that configuration, and it is Linux-only today for
exactly this reason.
