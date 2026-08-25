# `dev/macos-weak-symbols/`

What Mach-O's weak definitions do to this project, in the two places they
matter — and which pull in opposite directions. `NOTES.md` §13.2 states the
question; this and `dev/weakdef-probe/` answer it between them.

```sh
sh probe.sh          # macOS only
```

`dev/symbol-versioning/` is the ELF counterpart. There is no ELF question here:
that lookup does not distinguish weak definitions from strong ones.
`dev/weakdef-probe/` is the other half of the Mach-O question — it varies the
implementation where this varies us.

## The tension

Every other implementation of the standard ABI exports `MPI_*` as `weak
external`; this project exports them `external`. Checked against the artifacts
rather than assumed:

```
ours                                     external      _MPI_Init
Open MPI's ABI branch, libmpi_abi.1.dylib   weak external _MPI_Init
```

Being weak is what the **substitution** convention requires. Being strong is
what keeps us *out* of a **coalescing** contest we would otherwise win against
the implementation, capturing the wrapper's own outward call. So the two uses
disagree, and the question is what a change would cost.

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

## 2. Capture — settled, with `dev/weakdef-probe/`

Part 2 models §2's three-library shape with an implementation exporting
`MPI_Send` weakly from a two-level library, and asks where the wrapper's
outward call lands:

| `libmpi_abi` exports | result |
|---|---|
| strong | reached the implementation (isolated) |
| weak | re-entered `libmpi_abi` (captured) |

**This was first recorded as contradicting `HISTORY.md` §2.3**, which measured
the *strong* case being captured against a real ABI-built Open MPI, and so as
evidence of nothing. `HISTORY.md` §2.19 removed the contradiction rather than
the result: that capture was a `-flat_namespace` implementation build, which is
global-load-order lookup and not a coalescing outcome at all.

So the table above stands, and `dev/weakdef-probe/` — which varies the
implementation while holding our exports strong, the complement of what is
varied here — gives it its mechanism: **dyld's coalescing is decided among the
images that have weak definitions, and the first loaded wins.** An all-strong
`libmpi_abi` never enters the contest; a weak one does, and it is loaded first,
being what the application links.

Three mechanisms, not one, and only the middle is this probe's:

| | decided by | measured in |
|---|---|---|
| substitution | our export style vs. the client's link-time lookup | part 1, above |
| coalescing | which images export weakly, then load order | part 2, above |
| flat lookup | the *implementation's* namespace mode | `dev/weakdef-probe/`, §2.19 |

## What this does not settle

**No measurement covers a weak-exporting `libmpi_abi` in the real
configuration** — part 2 is a mock, and the real runs (`dev/weakdef-probe/`'s
ground truth, §10's oracle 5) are all of the library as it ships, strong. The
composed rule predicts capture there; nothing has watched it happen. Anyone
proposing the export-style change owes that run: build this project against
Open MPI's ABI branch (`--enable-standard-abi`, which
`ci-scripts/install-abi-mpi.sh` already produces), make `MPI_*` weak, and read
`DYLD_PRINT_BINDINGS` on the call that matters.

One dead end, recorded so it is not retried as if new. A third part testing the
shared-install-name hypothesis — that in the real configuration the
implementation's library is **also called `libmpi_abi`**, as MPI-5.0 §20.2.1
requires — was attempted and removed. With both libraries carrying
`@rpath/libmpi_abi.1.dylib`, the strong row stopped reaching `dlopen` at all,
failing with `Library not loaded`. Its motivation is gone now that §2.19 has
the real mechanism, and §2.3's own `dladdr` result already argued against it:
it resolved to the implementation, so both libraries were loaded and distinct.
