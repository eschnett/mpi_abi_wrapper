# `dev/mpich-user-cflags/`

**MPICH 5.0.2rc1 drops a user's `CFLAGS` for every embedded module.** Found by
pinning the prerelease (`ci-scripts/README.md`'s second named exception to the
released-tarball rule), which is what that pin is for.

**Reported and fixed upstream, and the fix is verified here but not yet
released.** Erik filed pmodels/mpich#7959 from `upstream-issue.md`; hzhou opened
pmodels/mpich#7960 the same day, which is the two-macro split that report
suggested: `PAC_PREFIX_FLAG` goes back to `$1_$2=$$2`, a new `PAC_INIT_FLAG` /
`PAC_INIT_ALL_FLAGS` takes the empty case, and only the `WRAPPER` call site
switches to it, so `PAC_PREFIX_ALL_FLAGS(USER)` snapshots again. **Measured, both
halves at once** (see "Verifying the fix" below): the flag reaches all eight
sub-configures *and* `mpicc CFLAGS:` stays empty — so it repairs the regression
without reintroducing the `WRAPPER` leak that PR7921 existed to stop.

**It shipped, in 5.0.2rc2.** The concern recorded here on 2026-09-10 was that
#7960 targeted `main` while `5.0.x` — byte-identical to `v5.0.2rc1` and the
branch 5.0.2 is cut from — still carried `$1_$2=""`, so merging to `main` alone
would not reach the release. That was resolved upstream: `v5.0.2rc2` carries the
fix as `e8de23b0b`, and reading the tag confirms all three parts of it —
`PAC_PREFIX_FLAG` copies again, `PAC_INIT_FLAG`/`PAC_INIT_ALL_FLAGS` exist, and
`configure.ac:271` calls `PAC_INIT_ALL_FLAGS(WRAPPER)` while `:332` keeps
`PAC_PREFIX_ALL_FLAGS(USER)`.

So `ci-scripts/suite/i386-suite.sh` has moved the libfabric flag back to
`CFLAGS`, where it belongs, and dropped the strip that the `CC` detour required.
What it kept is the `mpicc -show` assertion, repurposed: MPICH has now had build
flags in the wrong place in both directions inside two releases — leaking into
`mpicc` through 5.0.1, missing from the embedded modules in 5.0.2rc1 — so
whether the wrappers carry them is worth checking on every run rather than
assuming.

```sh
dev/mpich-user-cflags/run.sh              # both tags, side by side
dev/mpich-user-cflags/run.sh 5.0.2rc1     # one tag
```

It runs `configure` and nothing else — no build, no container, no 32-bit
anything — because the question is what each sub-configure is *handed*, and
configure prints that. Minutes per tag rather than the half-hour an ILP32 build
costs. Exits non-zero if a tag could not be fetched or unpacked, so a silent run
is not mistaken for a clean one.

## The symptom

All four `suite-i386` legs of run 34371453201 died in the embedded libfabric:

```
modules/libfabric/include/ofi_cma.h:67:40: error: passing argument 2 of
    'ofi_consume_iov' from incompatible pointer type [-Wincompatible-pointer-types]
   67 |                 ofi_consume_iov(local, &local_cnt, (size_t) ret);
      |                                        ^~~~~~~~~~ long unsigned int *
modules/libfabric/include/ofi_iov.h:204:51: note: expected 'size_t *'
    {aka 'unsigned int *'} but argument is of type 'long unsigned int *'
```

That mismatch is old news and harmless at this width — `ci-scripts/suite/i386-suite.sh`
has carried `-Wno-error=incompatible-pointer-types` for it since the row existed.
What is new is that the flag no longer arrives.

## The evidence, from the run's own configure output

The same MPICH configure line, `CFLAGS=-g -O2 -Wno-error=incompatible-pointer-types`,
reaches some embedded modules and not others:

| sub-configure | `CFLAGS` it was handed |
|---|---|
| `src/mpl` | `-g -O2 -Wno-error=incompatible-pointer-types -fvisibility=hidden -DHAVE_VISIBILITY` |
| `src/pmi` | `-g -O2 -Wno-error=incompatible-pointer-types` |
| `src/mpi/romio` | `-g -O2 -Wno-error=incompatible-pointer-types` |
| `src/pm/hydra` | `-g -O2 -Wno-error=incompatible-pointer-types -O2` |
| **`modules/libfabric`** | **` -fvisibility=hidden`** |
| **`modules/hwloc`** | **` -fvisibility=hidden`** |
| **`modules/json-c`** | **`` (empty)** |
| **`src/mpi/datatype/typerep/yaksa`** | **`` (empty)** |

The split is not arbitrary, and it is not about position in the file. The four
that lose the flags are each configured inside a bracket that the other four do
not use — `src/mpid/ch4/netmod/ofi/subconfigure.m4:322-327` is the clearest copy,
and its own comment says what the bracket is for:

```m4
dnl Unset all of these env vars so they don't pollute the libfabric configuration
PAC_PUSH_ALL_FLAGS()
PAC_RESET_ALL_FLAGS()
CFLAGS="$CFLAGS $VISIBILITY_CFLAGS"
PAC_CONFIG_SUBDIR_ARGS([modules/libfabric],[$ofi_subdir_args],...)
PAC_POP_ALL_FLAGS()
```

So a bracketed module is meant to see **the user's** flags rather than MPICH's
accumulated ones, which is a deliberate and reasonable design. `src/mpl`,
`src/pmi`, `romio` and `hydra` are configured without the bracket and therefore
see MPICH's `$CFLAGS`, which still contains the user's.

## The cause

`confdb/aclocal_util.m4` defines two macros that work as a pair:

```m4
AC_DEFUN([PAC_PREFIX_FLAG],[ $1_$2=$$2 ; export $1_$2 ; AC_SUBST($1_$2) ])

AC_DEFUN([PAC_RESET_ALL_FLAGS],[
        if test "$FROM_MPICH" = "yes" ; then
           CFLAGS="$USER_CFLAGS"
           ...
```

`configure.ac:331` calls `PAC_PREFIX_ALL_FLAGS(USER)` to snapshot the user's
flags into `USER_*`, and `PAC_RESET_ALL_FLAGS` restores that snapshot before each
embedded module's configure, so a module sees what the user asked for rather than
what MPICH accumulated. Upstream `b99300bad` changed the first macro:

```diff
-	$1_$2=$$2
+	$1_$2=""
```

The commit's subject is *"configure: don't pass compile flags through WRAPPER"*
and its purpose is real — it stops build flags leaking into `mpicc`'s
`WRAPPER_CFLAGS`, and after it `mpicc CFLAGS:` is correctly empty. But
`PAC_PREFIX_FLAG` is shared, and `configure.ac` calls
`PAC_PREFIX_ALL_FLAGS` twice: once for `WRAPPER` (line 270) and once for `USER`
(line 331). So the change emptied `USER_*` too, and every bracketed module's
`CFLAGS` is now reset to the empty string instead of to what the user asked for.
`PAC_RESET_ALL_FLAGS` still does exactly what it always did; the snapshot it
restores from is what stopped being taken.

| tag | `PAC_PREFIX_FLAG` body | `USER_CFLAGS` |
|---|---|---|
| `v5.0.1` | `$1_$2=$$2` | the user's `CFLAGS` |
| `v5.0.2rc1` | `$1_$2=""` | always empty |

**Both halves reproduce on the development laptop** — macOS, clang, 64-bit —
which is the point of the probe being configure-only: the dropped flag has
nothing to do with the architecture, the compiler or the operating system, and
ILP32 is only where the consequence is fatal. `run.sh` there:

| sub-configure | bracket | 5.0.1 | 5.0.2rc1 |
|---|---|---|---|
| `src/mpl` | plain | has the flag | has the flag |
| `src/pmi` | plain | has the flag | has the flag |
| `src/mpi/romio` | plain | has the flag | has the flag |
| `src/pm/hydra` | plain | has the flag | has the flag |
| `modules/libfabric` | reset | has the flag | **dropped** |
| `modules/hwloc` | reset | has the flag | **dropped** |
| `modules/json-c` | reset | has the flag | **dropped** |
| `src/mpi/datatype/typerep/yaksa` | reset | has the flag | **dropped** |

The `bracket` column is written into `run.sh` from reading the m4, *before* the
run — so the split falling exactly along it, eight for eight, is a prediction
that held rather than a pattern read out afterwards. The table further up is the
same result from run 34371453201's own configure output, with the flag values
quoted in full.

## What it costs beyond this project

Any `CFLAGS` a packager or user passes on MPICH's configure line stops reaching
libfabric, hwloc, json-c and yaksa: `-march=`, `-mtune=`, hardening flags,
`-Wno-error=` overrides for exactly the kind of vendored-code warning this row
hit. Nothing warns, and the build is silently inconsistent — half the object
files compiled as asked, half not.

Two things this is deliberately *not* claimed to be. It is not a change in what
MPICH's *own* accumulated flags do, which still reach the unbracketed modules
untouched. And it is not a sanitizer bug: `--enable-asan` appends to `CFLAGS`
rather than to `USER_CFLAGS`, so its flags never reached the bracketed modules on
either tag, and that predates this commit.

## Verifying the fix, without autoconf

The PR changes `configure.ac` and an m4 macro, so testing it in a release tarball
would ordinarily mean regenerating `configure`. It does not have to: the whole
effect of the PR on the generated script is the eight `USER_*` assignments, and
5.0.1's generated `configure` already shows the shape the PR restores. So mirror
that block into a copy of 5.0.2rc1's generated `configure` and leave the eight
`WRAPPER_*` ones alone —

```sh
cp -r mpich-5.0.2rc1 mpich-5.0.2rc1-pr7960
perl -i -pe 's/^\tUSER_([A-Z_]+)=""$/\tUSER_$1=\$$1/' mpich-5.0.2rc1-pr7960/configure
diff <(grep -E '^\tUSER_[A-Z_]+=' mpich-5.0.2rc1-pr7960/configure) \
     <(grep -E '^\tUSER_[A-Z_]+=' mpich-5.0.1/configure)      # must be identical
grep -c '^\tWRAPPER_[A-Z_]*=""$' mpich-5.0.2rc1-pr7960/configure   # must still be 8
```

— then configure it the way `run.sh` does. Both assertions are in the recipe on
purpose: the first says the patch reproduces the PR rather than something like
it, the second says it did not also undo PR7921.

Result on the development laptop, `CFLAGS=-g -O2 -Wno-error=incompatible-pointer-types`:

| | `modules/libfabric` gets | `mpicc CFLAGS:` |
|---|---|---|
| 5.0.1 | the flag | **the flag** — the leak PR7921 fixed |
| 5.0.2rc1 | **dropped** — this bug | empty |
| 5.0.2rc1 + #7960's effect | the flag | empty |

The middle column is the regression and the right column is what PR7921 was for;
#7960 is the first of the three trees to get both right. **5.0.2rc2 is that tree
as released** — the third row is what it does, reached by the same commit — so
`run.sh 5.0.1 5.0.2rc1 5.0.2rc2` now shows the whole arc in one table, and the
middle tag is the only one that ever dropped the flag. hwloc, json-c and yaksa
move with libfabric, and the four unbracketed modules never lost the flag.

## How this project works around it

`ci-scripts/suite/i386-suite.sh` moves the flag into `CC`, which survives because
`PAC_RESET_ALL_FLAGS` resets only `CFLAGS`, `CPPFLAGS`, `CXXFLAGS`, `FFLAGS`,
`FCFLAGS`, `LDFLAGS` and `LIBS`. MPICH bakes `$CC` into its compiler wrappers, so
the same script then strips the flag back out of the installed prefix and
**asserts with `mpicc -show` that no wrapper passes it to user code** — the
wrapper's own generated conversion code is exactly where an incompatible pointer
type must stay an error. Move the flag back to `CFLAGS`, which is the narrower
place for it, when upstream fixes the regression.
