# `dev/third-implementations/`

What the MVAPICH and Intel MPI CI rows assume, checked in a container rather
than asserted. NOTES.md §9's matrix asks for both — MVAPICH, Intel MPI and Cray
MPICH are named there as "the same argument extends to" after MPICH and Open
MPI — and everything below is a claim some comment in `ci-scripts/` or
`.github/workflows/ci.yaml` now makes.

```sh
dev/third-implementations/run.sh              # both
dev/third-implementations/run.sh mvapich      # ~20 min: builds from source
dev/third-implementations/run.sh intelmpi     # apt only; needs linux/amd64
```

Each probe ends with `PROBE-<NAME>: all claims hold` or names what failed.
Neither is wired into `ctest` or CI: these are measurements behind a claim, not
a gate. The CI rows themselves are the standing check.

**Everything below was measured in a container first and then confirmed by CI**
(run 32611538158, the first run of both rows). The container and the runner
agreed exactly, including the MVAPICH hang and its 45 s cap, which is the only
reason the numbers here are quoted without hedging. One thing CI found that the
container could not: the consumption-routes step was `skipped` on the MVAPICH
legs, because a failed `ctest` short-circuits a step whose default condition is
`success()`. Running the two by hand locally hid it. Both jobs now carry
`if: ${{ !cancelled() }}` on that step, so the leg those rows exist for reports
even when a hang fails the one before it.

## Why the two rows differ in shape

| | MVAPICH 4.1 | Intel MPI 2021.15 |
|---|---|---|
| how CI gets it | `ci-scripts/install-mvapich.sh`, pinned tarball, cached | `apt install intel-oneapi-mpi-devel=2021.15.0-493` |
| our 13 tests | **12 pass**, `abi_arrays_test` hangs upstream | **13 pass**, with `LD_LIBRARY_PATH` cleared |
| `check-install.sh` | 6 legs pass | 6 legs pass |
| job | two `linux-source` legs | the `linux-oneapi` job |
| arches | x86_64 and aarch64 | x86_64 only — Intel ships no aarch64 build |
| MPICH base | 4.3-or-later (it has `--enable-mpi-abi` and `src/binding/abi/`) | MPICH-derived, version not published |
| license | BSD | proprietary, free to download |

The vendor page's MVAPICH 4.1 quickstart still says "integrated and ABI
compatible with MPICH-3.4.3". That is stale by two major versions, which is why
every number here came out of the tarball.

## What both establish, and it is not the conversion tables

Both are MPICH-derived, so handle values, error classes and status layout are
MPICH's and §5's conversion tables are already exercised by the MPICH rows.
What a third and fourth implementation add is a third and fourth *installation*
shape — library naming, `mpicc`, launcher — which is where decision 19's "all
three consumption routes must build and run a program" meets something nobody
tuned it for. Both pass `check-install.sh`'s six legs.

## Measured

### The `_c` large-count surface is not MPICH's alone

NOTES.md §9's version table said MPICH >= 4.0 was "the only implementation that
actually provides the `_c` surface". Two of these four now do:

| | declares | `_c` surface |
|---|---|---|
| MVAPICH 4.1 | `MPI_VERSION 4` / `MPI_SUBVERSION 1` | 387 `_c` prototypes in `src/include/mpi_proto.h`; `MPI_Type_size_c` links and returns 4 |
| Intel MPI 2021.15 | `MPI_VERSION 3` / `MPI_SUBVERSION 1` | `MPI_Type_size_c` links and returns 4 **anyway** — the declared level understates the surface |
| Open MPI 5.0.10 | `MPI_VERSION 3` / `MPI_SUBVERSION 1` | none at all — still what makes decision 6's stubs load-bearing |

**Both probes check this by linking and running, never by grepping a header**,
because grepping is how you get it wrong twice over. `src/include/mpi.h.in` in
the MVAPICH tarball contains *no* prototypes — `mpi.h` includes
`src/include/mpi_proto.h` — so a grep of the obvious file reports zero `_c`
entry points for an implementation that has 387. And `install-mpich.sh`'s
header already records the other direction: MPICH 3.1.4 built
`--disable-fortran` *declares* `MPI_Type_create_f90_real` and cannot link it.

### Symbol binding: a third pattern, and it tracks the build not the family

`linux-test.sh` prints this every row as evidence for NOTES.md §2's claim that
`MPI_*` and `PMPI_*` both exist and reach the same code. Three patterns now:

| | binding |
|---|---|
| Ubuntu MPICH, Open MPI | both strong, at one address |
| macOS MPICH | `PMPI_*` in a separate library |
| MVAPICH 4.1 `libmpi.so.0` | **672 weak `MPI_*` over 672 strong `PMPI_*`** |
| Intel MPI 2021.15 `libmpi.so.12` | **613 weak `MPI_*` over 613 strong `PMPI_*`** |

`MPI_Send` and `PMPI_Send` share an address in every one, which is the whole of
what §2 requires. The two agreeing rows are the MPICH-derived newcomers while
Ubuntu's MPICH is not among them, so the pattern follows the *build* rather than
the family — which is why this stays a per-row report instead of a table
recorded once and trusted.

### MVAPICH needs two packages, and no configure flag

MVAPICH is InfiniBand-first and its documented configurations name `ch4:ucx`
(IB/RoCE) and `ch4:ofi` (Slingshot/OPX/PSM3), none of which a runner has. That
looked like this row's main risk. It is not: `configure.ac` defaults
`with_device=ch4`, ch4's default netmod is `ofi` over the libfabric bundled in
`modules/libfabric`, and configure's own epilogue for that case says it "should
work for TCP networks". A stock configure is correct, exactly as for the other
two installers.

What does bite is a *build* dependency, and it is where "MPICH-derived so its
build is MPICH's" stops being true. MVAPICH's bundled libfabric carries two
providers MPICH's does not — `prov/mverbs` and `prov/ucr` — and both include
`<infiniband/ib.h>` unconditionally. A stock build gets through configure and
every other module before dying:

```
modules/libfabric/prov/mverbs/src/mverbs_ofi.h:56:10:
    fatal error: infiniband/ib.h: No such file or directory
make[3]: *** [Makefile:32107: prov/ucr/src/src_libfabric_la-ucr_domain.lo] Error 1
```

`libibverbs-dev` and `librdmacm-dev` fix it. Headers only: no hardware, no
kernel module, nothing consulted at run time, and **no `--with-device` and no
patch**, so §9's stock-configure rule survives. `ci.yaml` installs them on the
MVAPICH legs alone rather than in the shared toolchain step, because the MPI
cache key hashes the install scripts and *not* the runner's package list — a
package that changes what MPICH's or Open MPI's configure finds would change
their installed bytes with nothing to invalidate the caches serving them.

### MVAPICH 4.1 hangs in `MPI_Dist_graph_create`, and it is not ours

`abi_arrays_test` does not return. The backtrace is the finding — frames 0–7 are
all inside `libmpi.so.0`, and the two wrapper frames above them are
pass-through:

```
#2  ofi_gettime_ms ()             from /opt/mvapich/lib/libmpi.so.0
#3  sock_cq_sreadfrom ()          from /opt/mvapich/lib/libmpi.so.0
#4  MPIDI_NM_progress.isra.0 ()   from /opt/mvapich/lib/libmpi.so.0
#5  MPIC_Probe ()                 from /opt/mvapich/lib/libmpi.so.0
#6  MPIR_Dist_graph_create_impl ()from /opt/mvapich/lib/libmpi.so.0
#7  PMPI_Dist_graph_create ()     from /opt/mvapich/lib/libmpi.so.0
#8  w_MPI_Dist_graph_create ()    from libmpiwrapper.so
#9  MPI_Dist_graph_create ()      from libmpi_abi.so
```

Then the check that settles it, which is `ci.yaml`'s "two ranks with no wrapper
involved" gate applied to one call: a twelve-line program calling
`MPI_Dist_graph_create` on `MPI_COMM_WORLD`, compiled with MVAPICH's own
`mpicc` and run under its own `mpiexec -n 2`, **times out with nothing of this
project loaded**. `FI_PROVIDER=tcp` and `FI_PROVIDER=shm` do not change it, so
there is no `host-env.sh`-style workaround to name. Measured on
`ubuntu:24.04`/aarch64, MVAPICH 4.1, stock configure.

**One test, and only one.** With `CTEST_TIMEOUT=45` the rest of the suite
finishes and the result is 12 of 13 — `abi_arrays_test` times out and the other
twelve pass, including `isolation-check` and `exported-symbols`.
`check-install.sh`'s six consumption legs pass too, which is what this row was
added for. So the MVAPICH legs are one known upstream hang away from gating.

The row is therefore **report-only and capped, not excluded**: `CTEST_TIMEOUT=45`
on those legs, because at ctest's 1500 s default this one test is 25 minutes of a
90-minute job spent re-learning the paragraph above.
`suite/timelimit-ci-openmpi.txt` established the rule — *cap a hang, do not
exclude it* — and the reason applies here unchanged: a capped test still runs and
still reports, so the day MVAPICH fixes this the row turns green by itself, where
an `-E` would have made that day invisible.

### Intel MPI ships its own `libmpi_abi.so`, and it wins the loader race

The one finding here that is about this project rather than about an
implementation, and the reason the `linux-oneapi` job does not export
`LD_LIBRARY_PATH`.

Intel MPI 2021.18 implements the standard MPI ABI natively and installs it
beside its own library:

```
$I_MPI_ROOT/lib/libmpi_abi.so -> libmpi_abi.so.1.0   SONAME libmpi_abi.so.1
    670 MPI_* symbols, 159 of them `_c`
```

That is the same *filename* this project builds. `mpicc -show` does not link it,
so the plain `mpicc` is still an ordinary wrap target — but `vars.sh` puts that
directory on `LD_LIBRARY_PATH`, and our test binaries record

```
NEEDED   libmpi_abi.so
RUNPATH  /tmp/build-intelmpi
```

**`DT_RUNPATH` is searched after `LD_LIBRARY_PATH`**, so the loader binds them to
Intel's library, not ours:

| environment | resolves to | ctest |
|---|---|---|
| `vars.sh`'s `LD_LIBRARY_PATH` exported | `/opt/intel/oneapi/mpi/2021.18/lib/libmpi_abi.so` | **5 of 13 fail** |
| `env -u LD_LIBRARY_PATH` | `/tmp/build-intelmpi/libmpi_abi.so` | **13 of 13 pass** |

The five are `abi_prototype_test`, `abi_arrays_test`, `abi_tools_test`,
`abi_converters_test` and `abi_state_test` — exactly the five black-box tests
that load a wrapper, and nothing else. `ld.so` even says so out loud
(`no version information available`, because ours carries the `MPIABI_1` version
script and Intel's binary has no matching verdef), which is the only reason this
was noticed at all rather than filed as five mysterious failures.

Two things follow, and the second is the one that matters:

1. Nothing needs the variable. `mpicc -show` bakes `-Xlinker -rpath` to
   `$I_MPI_ROOT/lib` into every link, `mpiexec` and the hydra proxies carry
   their own RPATHs, and Intel MPI reaches its bundled libfabric through
   `I_MPI_ROOT`. Dropping it costs nothing and all 13 pass.
2. **Five red tests is the lucky outcome.** A test tolerant enough to pass
   against a genuine ABI implementation would have gone green here having never
   loaded this project — `HISTORY.md` §2.14's failure mode reached by a new
   route. `check-install.sh` was immune by construction, because it runs every
   consumption route under `env -u LD_LIBRARY_PATH` for exactly this class of
   accident; its six legs passed on the same run where ctest failed, and that
   disagreement is what made the failures worth a backtrace instead of a shrug.

This is not Intel-specific. Any wrapped MPI that ships a standard-ABI library of
its own is the same hazard — MPICH 5.0 behind `--enable-mpi-abi` and MVAPICH 4.1
both carry the flag — so it is recorded in `NOTES.md` §13.2 as a limitation of
the design rather than here as a fact about one vendor. It is also *not*
NOTES.md §10's fifth oracle: deliberately wrapping an ABI-implementing MPI is a
different row that does not exist yet and needs `MPIWRAPPER_WRAP_ABI_IMPL`.

### Which Intel MPI merits wrapping

The question that decides whether the row exists, and the answer moved the pin
from "newest" to a two-year-old release. Wrapping an MPI that already implements
the standard ABI is not what this project is for, so the row is only worth
having below the line where Intel started shipping one. Bisected by downloading
each `intel-oneapi-mpi-devel-<v>` package and listing its contents — no
installs:

| release | ships `libmpi_abi` | `MPI_T_event_dropped_cb_function` | wrapper builds |
|---|---|---|---|
| 2021.9.0 | no | not declared | — |
| 2021.11 | no | not declared | — |
| 2021.13 | no | not declared | — |
| **2021.15** | **no** | **not declared** | **yes, 13/13** |
| 2021.16 | no | `int count` | **no** |
| 2021.17 | **yes** | `MPI_Count count` | — (redundant) |
| 2021.18 | **yes** | `MPI_Count count` | yes, 13/13 |

Both changes landed in the same release, 2021.17: the ABI library appeared and
the callback's first parameter was corrected to `MPI_Count`. Intel documents
the ABI as an MPI-5.0 standard technical preview, C only, reached through
`mpicc -mpi-abi`, so on 2021.17+ the default `mpicc` is still an ordinary wrap
target — just a pointless one to wrap.

**So the pin is 2021.15, and 2021.16 is a hole rather than a floor.** That one
release declares the dropped-events callback with `int count` where MPI-4.1 and
everything else say `MPI_Count count`, so `src/mpiwrapper/toolevents.c` fails
with `conflicting types for 'mpiwrapper_t_event_dropped_tramp'`. 2021.15 does
not declare the callback at all, so the availability probe leaves the trampoline
out. That is NOTES.md §13.4's entry, and the general lesson is sharper than the
one release: **`MPIWRAPPER_HAVE_*` asks whether an entry point is declared, and
this is the first case where the answer had to be "declared with what
signature".**

Worth noting how it was found. A row pinned to the newest release would never
have compiled against 2021.16 either — it would simply have skipped over it —
and the reason to pin *old* is also what exposed the gap.

## The failure that was the harness's

The first MVAPICH build failed somewhere else entirely, and the mistake is worth
keeping because it cost an hour and looked exactly like an upstream bug:

```
../libtool: line 2112: syntax error near unexpected token `library'
```

`bash -n` reproduced it on the generated `modules/hwloc/libtool`, and the file
was genuinely malformed — a `case` arm truncated mid-pattern and spliced into a
region a hundred lines away:

```
    case " $hookable_fns " in
      *"                   ;;
          esac
	  ;;
	freebsd-aout)
```

That is not bad content, it is two writers. The broken file was 9421 lines; the
same file regenerated once with `./config.status libtool` was 11813 and valid.
The cause was the harness: `MPI_SRC_DIR` pointed at a Docker bind mount to the
macOS host, and concurrent writes through that mount truncate and interleave.
Building on the container's own filesystem, the corruption never recurred — and
the *real* MVAPICH problem, the missing `infiniband/ib.h`, was one stage further
on and had been hidden behind it.

Hence `run.sh` setting `MPI_SRC_DIR=/build` and both probes saying so in their
headers. This is `CLAUDE.md`'s "reasons that are the machine's, not the
project's" and `HISTORY.md` §2's habit of overturning a confident first reading
by measuring it — in this case twice in one build, since the bind mount also
made the first diagnosis ("MVAPICH races its own libtool under `make -j`") look
well-supported.
