Draft for pmodels/mpich. Not filed from this repository — Erik posts it.

---

**Title:** `b99300bad` drops the user's `CFLAGS` for every embedded module (libfabric, hwloc, json-c, yaksa)

**Body:**

`v5.0.2rc1` no longer passes a user's `CFLAGS` to the embedded modules that are
configured inside a `PAC_PUSH_ALL_FLAGS` / `PAC_RESET_ALL_FLAGS` bracket. `v5.0.1`
does. The regression is in `confdb/aclocal_util.m4`.

### Reproducer

```sh
./configure --prefix=/tmp/opt CFLAGS="-g -O2 -Wno-error=incompatible-pointer-types"
grep 'running.*configure' config.log   # or configure's own stdout
```

On `v5.0.2rc1`:

```
running .../src/mpl/configure            ... 'CFLAGS=-g -O2 -Wno-error=incompatible-pointer-types  -fvisibility=hidden -DHAVE_VISIBILITY'
running .../src/pmi/configure            ... 'CFLAGS=-g -O2 -Wno-error=incompatible-pointer-types   '
running .../src/mpi/romio/configure      ... 'CFLAGS=-g -O2 -Wno-error=incompatible-pointer-types   '
running .../src/pm/hydra/configure       ... 'CFLAGS=-g -O2 -Wno-error=incompatible-pointer-types    -O2'
running .../modules/libfabric/configure  ... 'CFLAGS= -fvisibility=hidden'          <-- dropped
running .../modules/hwloc/configure      ... 'CFLAGS= -fvisibility=hidden'          <-- dropped
running .../modules/json-c/configure     ... 'CFLAGS='                              <-- dropped
running .../src/mpi/datatype/typerep/yaksa/configure ... 'CFLAGS='                  <-- dropped
```

On `v5.0.1` all eight carry the flag.

### Cause

`b99300bad` ("configure: don't pass compile flags through WRAPPER") changed
`PAC_PREFIX_FLAG`:

```diff
 AC_DEFUN([PAC_PREFIX_FLAG],[
-	$1_$2=$$2
+	$1_$2=""
 	export $1_$2
 	AC_SUBST($1_$2)
 ])
```

That achieves the commit's goal — `WRAPPER_CFLAGS` no longer inherits build
flags, and `mpicc CFLAGS:` is correctly empty. But `configure.ac` calls
`PAC_PREFIX_ALL_FLAGS` twice, for two unrelated purposes:

- line 270, `PAC_PREFIX_ALL_FLAGS(WRAPPER)` — the compiler wrappers' flags, which
  the commit intends to empty;
- line 331, `PAC_PREFIX_ALL_FLAGS(USER)` — the snapshot of the flags the user
  actually passed, which `PAC_RESET_ALL_FLAGS` restores before each bracketed
  sub-configure so that the module sees the user's flags rather than MPICH's
  accumulated ones (`src/mpid/ch4/netmod/ofi/subconfigure.m4:321-327`, whose own
  comment says so).

Emptying the shared macro emptied both. `PAC_RESET_ALL_FLAGS` still behaves as
designed; the snapshot it restores from is what stopped being taken.

### Impact

Any `CFLAGS` a user or packager supplies is silently dropped for four modules, so
the build is half compiled as asked and half not — `-march=`, `-mtune=`,
hardening flags, and `-Wno-error=` overrides for warnings in vendored code.

It is fatal on ILP32 today. `modules/libfabric/include/ofi_cma.h:67` passes
`unsigned long *` where `ofi_consume_iov` takes `size_t *`; on 32-bit those are
distinct types, and gcc 14 makes `-Wincompatible-pointer-types` an error, so
`src/fabric.c` fails to compile:

```
modules/libfabric/include/ofi_cma.h:67:40: error: passing argument 2 of
    'ofi_consume_iov' from incompatible pointer type [-Wincompatible-pointer-types]
   67 |                 ofi_consume_iov(local, &local_cnt, (size_t) ret);
      |                                        ^~~~~~~~~~ long unsigned int *
modules/libfabric/include/ofi_iov.h:204:51: note: expected 'size_t *'
    {aka 'unsigned int *'} but argument is of type 'long unsigned int *'
```

`CFLAGS=-Wno-error=incompatible-pointer-types` was the supported way past that
and no longer works. (The underlying libfabric mismatch is worth a separate
report; this issue is only about the flag not arriving.)

### Suggested fix

Give the two callers different behaviour rather than sharing one macro — keep
`PAC_PREFIX_FLAG` saving the flag and add a separate initialise-to-empty macro
for the `WRAPPER` case, e.g.

```m4
dnl PAC_PREFIX_FLAG - Save flag with a prefix
AC_DEFUN([PAC_PREFIX_FLAG],[
	$1_$2=$$2
	export $1_$2
	AC_SUBST($1_$2)
])

dnl PAC_INIT_PREFIX_FLAG - Declare a prefixed flag variable, initially empty
AC_DEFUN([PAC_INIT_PREFIX_FLAG],[
	$1_$2=""
	export $1_$2
	AC_SUBST($1_$2)
])
```

with `PAC_PREFIX_ALL_FLAGS(WRAPPER)` using the new one and
`PAC_PREFIX_ALL_FLAGS(USER)` keeping the old. Found while testing `5.0.2rc1`
before release.
