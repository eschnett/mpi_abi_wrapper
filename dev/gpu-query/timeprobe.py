"""What the optional-header pre-check costs (HISTORY.md #1.26).

Times dev/probe_impl.py's `#include <mpi.h>` / `#include <mpi-ext.h>`
compile against the whole probe, over native Open MPI 5.0.6. The header
cache is warmed first, so what is measured is the extra compile rather
than the first read of <mpi.h>. Read-only. See README.md.
"""
import subprocess
import sys
import tempfile
import time
from pathlib import Path

R = Path(__file__).resolve().parents[2]  # the repository root
sys.path.insert(0, str(R / "dev"))
import probe_impl as pi  # noqa: E402

CC = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc"
INC = subprocess.run(["/Users/eschnett/src/mpi_abi_wrapper/build/mpi/openmpi-native/bin/mpicc",
                      "-showme:incdirs"], capture_output=True, text=True).stdout.split()
flags = [f"-I{d}" for d in INC]

guards = pi.wanted(pi.DEFAULT_SOURCES, R / "dev" / "entrypoints.txt")
names = sorted({p for v in guards.values() for p in v})

with tempfile.TemporaryDirectory() as wd:
    pi.compile_only_flags(CC, wd)   # warm the cache
    pi.compile_ok(CC, flags, "#include <mpi.h>\n", wd)  # warm the header cache
    t0 = time.perf_counter()
    for _ in range(5):
        ok, _ = pi.compile_ok(CC, flags,
                              "#include <mpi.h>\n#include <mpi-ext.h>\n", wd)
    t1 = time.perf_counter()
    print(f"pre-check (mpi-ext.h present={ok}): {(t1 - t0) / 5 * 1000:.0f} ms")

t2 = time.perf_counter()
present, headers = pi.probe(CC, flags, names)
t3 = time.perf_counter()
print(f"whole probe, {len(names)} spellings: {(t3 - t2) * 1000:.0f} ms, "
      f"headers={headers}")
