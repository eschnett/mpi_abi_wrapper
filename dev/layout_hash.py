#!/usr/bin/env python3
"""Compute (or check) MPIWRAPPER_LAYOUT_HASH from the vtable slot list.

The hash exists so that a libmpi_abi and a libmpiwrapper generated from
different slot lists refuse each other at load time instead of calling through
a shifted slot (NOTES.md #2). It has to be derived from the slot list rather
than bumped by hand, or it stops tracking what it is supposed to track.

Definition, which S2's generator must reproduce: take the text between
`struct mpiwrapper_vtable {` and the matching `};`, delete comments, collapse
all whitespace runs to nothing, and take FNV-1a/32 of the resulting bytes.
Whitespace is collapsed so that reformatting the header -- clang-format moving
a line break -- does not change the value, while adding, removing, renaming or
retyping a slot does.

Usage: dev/layout_hash.py [--check] [header]
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_HEADER = ROOT / "src" / "include" / "mpiwrapper_vtable.h"

STRUCT_START = "struct mpiwrapper_vtable {"
HASH_MACRO = "MPIWRAPPER_LAYOUT_HASH"


def slot_list_text(header_text: str) -> str:
    start = header_text.index(STRUCT_START) + len(STRUCT_START)
    end = header_text.index("\n};", start)
    body = header_text[start:end]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.DOTALL)
    body = re.sub(r"//[^\n]*", "", body)
    return re.sub(r"\s+", "", body)


def fnv1a32(data: bytes) -> int:
    h = 0x811C9DC5
    for b in data:
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return h


def main() -> int:
    args = [a for a in sys.argv[1:] if a != "--check"]
    check = "--check" in sys.argv[1:]
    header = Path(args[0]) if args else DEFAULT_HEADER
    text = header.read_text()

    value = fnv1a32(slot_list_text(text).encode())
    nslots = len(re.findall(r"\(\*P?MPI_", text[text.index(STRUCT_START):]))

    if not check:
        print(f"{value:#010x}u  ({nslots} slots in {header})")
        return 0

    m = re.search(rf"#define\s+{HASH_MACRO}\s+(0x[0-9a-fA-F]+)u?", text)
    if not m:
        print(f"{header}: no {HASH_MACRO} definition found", file=sys.stderr)
        return 1
    committed = int(m.group(1), 16)
    if committed != value:
        print(
            f"{header}: {HASH_MACRO} is {committed:#010x} but the {nslots}-slot "
            f"list hashes to {value:#010x}.\n"
            f"The slot list changed: update the macro (and remember that a "
            f"libmpi_abi built before the change will now refuse this wrapper, "
            f"which is the point).",
            file=sys.stderr,
        )
        return 1
    print(f"{HASH_MACRO} {value:#010x} matches the {nslots}-slot list")
    return 0


if __name__ == "__main__":
    sys.exit(main())
