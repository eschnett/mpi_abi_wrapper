# `dev/symbol-versioning/`

Why neither version script under `cmake/` names its node (`NOTES.md` decision
22, `HISTORY.md` §1.23).

```sh
docker build -t mpiabi-symver . && docker run --rm mpiabi-symver
sh probe.sh          # or directly, on any glibc host
```

## The question

`NOTES.md` §9 asks for a version script on ELF, for one stated reason: the
export set. `-fvisibility=hidden` cannot reach the symbols the linker inserts
into every shared object (`_init`, `_fini`, `_edata`, `_end`, `__bss_start`),
and a version script can make them local. Both scripts were written with a
named node — `MPIABI_1`, `MPIWRAPPER_1` — because that is what a version script
conventionally looks like.

Naming the node does something the export set does not show, and this probe is
three commands that show it.

## What it measures

Two copies of the same stand-in library, built with two version scripts that
differ in one identifier, then an application linked against each, then the
swap the standard ABI exists to permit: running each application against
*another implementation's* `libmpi_abi`, which defines the same names and has
never heard of `MPIABI_1`.

Measured on `ubuntu:24.04`, gcc 13, glibc 2.39:

| | exports | client binary records | against another implementation |
|---|---|---|---|
| `MPIABI_1 { … }` | `MPI_Send@@MPIABI_1`, `PMPI_Send@@MPIABI_1`, **`MPIABI_1`** | `MPI_Send@MPIABI_1` | **fails to start** |
| `{ … }` (anonymous) | `MPI_Send`, `PMPI_Send` | `MPI_Send` | runs |

`internal_helper` is absent from both, which is the point about filtering: the
anonymous node does the whole of what §9 wanted, and the named node's extra
effect is confined to the last two columns.

The failure is not a diagnostic and a fallback. It is:

```
./app_named: /tmp/other/libnamed.so: no version information available (required by ./app_named)
Inconsistency detected by ld.so: dl-lookup.c: 106: check_match:
  Assertion `version->filename == NULL || ! _dl_name_match_p (version->filename, map)' failed!
```

— a warning, then an assertion failure inside the loader, exit 127. A versioned
undefined reference can be satisfied only by a library defining that version,
and no other implementation of the standard ABI defines ours.

## What it settles

That the convention is backwards for *this* library. Symbol versioning protects
a library whose ABI its author owns and whose consumers link that author's
copy; `libmpi_abi` is neither. Had 1.0 shipped with the named node, every binary
built against it would have failed to start against precisely the libraries the
ABI exists to let it run against — and the export set, the only thing the
project checked, would have looked correct throughout.

The third column is also the reason `test/check_exports.cmake` no longer
carries an exemption list: an anonymous node contributes no symbol of its own.
