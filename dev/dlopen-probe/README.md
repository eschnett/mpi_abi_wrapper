# `dev/dlopen-probe/`

A mock-up of this project's three-library structure, to settle how `libmpiwrapper`
must be loaded. It contains no MPI: `libimpl` stands in for `libmpi`, `libwrap` for
`libmpiwrapper`, `libabi` for `libmpi_abi`, each with a one-argument `MPI_Send` that
announces itself.

```sh
./run.sh                                    # this platform
docker build -t dlopen-probe . && docker run --rm dlopen-probe   # Linux
```

## Why it exists

`libmpi_abi` and `libmpi` both define `MPI_Send`. `libmpi_abi` therefore loads the
wrapper with `dlopen` — but that only avoids the collision at *link* time. At *run*
time the loader still has to decide what `libmpiwrapper`'s own call to `MPI_Send`
means, and on ELF the answer is not the obvious one.

## The three tests

| | question | correct answer |
|---|---|---|
| **T1** | Does the wrapper's `MPI_Send` call reach `libimpl`? | via `libabi` → `libimpl` |
| **T2** | Does the *implementation's own internal* `MPI_Send` call reach `libimpl`? | `libimpl` directly |
| **T3** | Does a second, later-`dlopen`ed plugin reach the native MPI through `libabi`? | via `libabi` → `libimpl` |

T2 models Open MPI's ROMIO and io components, which are written against the MPI
interface. T3 models the mpi4py shape: a host executable that knows nothing about
MPI, loading two MPI-using extension modules in turn.

`CAPTURED` means `libabi::MPI_Send` was re-entered — infinite recursion in the real
system. `BYPASSED` means the caller reached the native MPI *without* passing through
the ABI layer, and would be handed ABI-typed arguments.

## Results

**Linux, glibc 2.36, aarch64, gcc 12.2**

| mode | T1 | T2 | T3 |
|---|---|---|---|
| `dlopen(RTLD_LOCAL)` | **CAPTURED** | **CAPTURED** | OK |
| `dlopen(RTLD_GLOBAL)` | **CAPTURED** | **CAPTURED** | **BYPASSED** |
| `dlopen(RTLD_LOCAL\|RTLD_DEEPBIND)` | OK | OK | OK |
| `dlmopen(LM_ID_NEWLM)` | OK | OK | OK |

**Linux, glibc 2.36, arm32v7 (32-bit), gcc 12.2** — identical in every cell to the
64-bit run above, so none of this depends on pointer width.

| mode | T1 | T2 | T3 |
|---|---|---|---|
| `dlopen(RTLD_LOCAL)` | **CAPTURED** | **CAPTURED** | OK |
| `dlopen(RTLD_GLOBAL)` | **CAPTURED** | **CAPTURED** | **BYPASSED** |
| `dlopen(RTLD_LOCAL\|RTLD_DEEPBIND)` | OK | OK | OK |
| `dlmopen(LM_ID_NEWLM)` | OK | OK | OK |

**macOS 26, arm64, Apple clang**

| mode | T1 | T2 | T3 |
|---|---|---|---|
| `dlopen(RTLD_LOCAL)` | OK | OK | OK |
| `dlopen(RTLD_GLOBAL)` | OK | OK | OK |
| `RTLD_LOCAL`, wrapper built `-flat_namespace` | **CAPTURED** | OK | — |

## What this establishes

**1. Plain `dlopen` is broken on Linux, in the default configuration.** `RTLD_LOCAL`
captures both T1 and T2. `LOCAL` versus `GLOBAL` controls what the loaded object
*exports*, not how its own references resolve, so it does not help. The loader
states the reason itself under `LD_DEBUG=scopes`:

```
object=./libwrap.so [0]
 scope 0: ./app /probe/out/libabi.so /lib/.../libc.so.6 /lib/ld-linux-aarch64.so.1
 scope 1: ./libwrap.so /probe/out/./libimpl.so /lib/.../libc.so.6 ...
```

Scope 0 — the global scope, which contains `libabi` because the executable links it
— is searched before the object's own dependency subtree in scope 1. `libimpl` gets
the same scope list, which is why T2 fails too.

**2. `RTLD_GLOBAL` is strictly worse, and T3 shows it.** It appends the
`dlopen`ed object *and its dependencies* to the global scope, so `libimpl`'s
`MPI_Send` becomes globally visible. A plugin loaded afterwards then binds past the
ABI layer:

```
binding file ./libplugin2.so [0] to /probe/out/./libimpl.so [0]: normal symbol `MPI_Send'
```

Both the correct and the broken path end in `libimpl`, so this is invisible to a
test that only checks the return value — the first version of this probe reported a
false pass. The instrumentation now distinguishes "reached `libimpl` because we
forwarded" from "reached `libimpl` directly".

**3. `RTLD_DEEPBIND` is sufficient, and it applies transitively.** This was the open
question: `libimpl` is a dependency loaded by the same `dlopen`, not the object
named in the call, so it was not obvious that `DEEPBIND` would redirect the
implementation's *own* internal `MPI_*` references. T2 passes, so it does.

**4. `dlmopen` also works**, on all three, and `dladdr` still resolves across the
namespace boundary on this glibc — so the load-time isolation check keeps working
under it.

**5. macOS is safe because of the two-level namespace, and the flat-namespace
control proves that is the reason.** Force `-flat_namespace` on the wrapper and T1
captures immediately. The assumption is load-bearing, so a macOS build must not
acquire `-flat_namespace` by accident.

**6. The proposed load-time isolation check works.** On the flat-namespace build it
reported `isolation check: FAIL (same base object)` before any call was made — which
is the difference between a clear diagnostic at startup and a stack overflow on the
first message.

## Scope: what this probe could not see

**S1 disproved the generalisation of the `dlmopen` row.** It passes every test *here*
and does not work with a real MPI: both MPICH 4.1 and Open MPI 4.1 segfault inside
glibc's own loader during `MPI_Init`, in `add_to_global_resize`, because an MPI that
`dlopen`s its own components (PMIx, Open MPI's MCA) asks to add to the global scope
of a namespace that has no main map. MPICH 3.1.4, which predates PMIx and loads
nothing at run time, *does* work under `dlmopen` — confirming the diagnosis from the
other side: `dlmopen` is not broken, it is unusable with any MPI that loads plugins.

Why this probe missed it is the part worth keeping: `libimpl` here is a single shared
object that `dlopen`s nothing. The mock reproduced the *symbol resolution* faithfully
and the *loading behaviour* not at all, and nothing in its own output hinted at the
gap. A mock is evidence only about the axis it models.

The recommendation below therefore stands for `RTLD_DEEPBIND`, which S1 confirmed on
real MPIs, and **not** for `dlmopen`. See `NOTES.md` §2.

## Conclusion

| platform | how to load the wrapper |
|---|---|
| Linux | `dlopen(RTLD_LOCAL \| RTLD_DEEPBIND)` by default; `dlmopen(LM_ID_NEWLM)` selectable |
| macOS | `dlopen(RTLD_LOCAL)`, and never `-flat_namespace` |
| FreeBSD | `dlopen(RTLD_LOCAL \| RTLD_DEEPBIND)`; no `dlmopen` |

`RTLD_GLOBAL` is wrong everywhere. Keep the two Linux modes selectable at run time:
`RTLD_DEEPBIND` is known to interfere with `malloc` interposition and with
sanitizers, so the sanitizer CI jobs are the reason `dlmopen` stays available.

## Not yet measured

- i386 specifically. arm32v7 was run and matched 64-bit exactly; i386 emulation is
  not available on an arm64 Docker host, and there is no reason to expect a
  difference.
- Behaviour under ASan, which is the case `RTLD_DEEPBIND` is most likely to disturb.
- glibc versions other than 2.36, and musl, which has no `dlmopen` and ignores
  `RTLD_DEEPBIND`.
