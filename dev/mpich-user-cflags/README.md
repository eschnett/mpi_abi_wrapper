# `dev/mpich-user-cflags/`

**MPICH 5.0.2rc1 drops a user's `CFLAGS` for every embedded module.** Found by
pinning the prerelease (`ci-scripts/README.md`'s second named exception to the
released-tarball rule), which is what that pin is for. Upstream report drafted in
`upstream-issue.md`; not filed from here.

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

## How this project works around it

`ci-scripts/suite/i386-suite.sh` moves the flag into `CC`, which survives because
`PAC_RESET_ALL_FLAGS` resets only `CFLAGS`, `CPPFLAGS`, `CXXFLAGS`, `FFLAGS`,
`FCFLAGS`, `LDFLAGS` and `LIBS`. MPICH bakes `$CC` into its compiler wrappers, so
the same script then strips the flag back out of the installed prefix and
**asserts with `mpicc -show` that no wrapper passes it to user code** — the
wrapper's own generated conversion code is exactly where an incompatible pointer
type must stay an error. Move the flag back to `CFLAGS`, which is the narrower
place for it, when upstream fixes the regression.
