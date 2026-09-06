"""What each configured build tree's availability probe found.

Reads the mpiwrapper_impl_config.h of the three build trees CLAUDE.md
describes and prints, per row, how many MPIWRAPPER_HAVE_* guards were
asked, how many were answered, and the names that were not. Read-only.
See README.md for what it said and why it matters (NOTES.md #7
decision 28, CODE.md 4).
"""
import re
import sys
from pathlib import Path

R = Path(__file__).resolve().parents[2]  # the repository root
sys.path.insert(0, str(R / "dev"))
import probe_impl as pi  # noqa: E402

guards = set(pi.wanted(pi.DEFAULT_SOURCES, R / "dev" / "entrypoints.txt"))
for name, cfg in (("MPICH 4.3.1", "build/mpich"),
                  ("Open MPI 5.0.6", "build/openmpi"),
                  ("ompi-main ABI", "build/ompi-main-abi")):
    text = (R / cfg / "wrapper-include" / "mpiwrapper_impl_config.h").read_text()
    have = set(re.findall(r"#define MPIWRAPPER_HAVE_(\w+) 1", text))
    ext = "MPI_EXT_H" in have
    have -= {"MPI_EXT_H"}
    absent = sorted(guards - have)
    print(f"{name}: {len(guards)} asked, {len(have)} available, "
          f"{len(absent)} absent, mpi-ext.h={ext}")
    print("    " + ", ".join(absent))
    print()
