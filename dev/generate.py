#!/usr/bin/env python3
"""The generator (NOTES.md #3).

Reads the two machine-readable inputs -- `gen/include/mpi.h`, which the S0 step
produces, and the vendored `dev/apis.json` -- and emits the seven artifacts of
NOTES.md #3:

    gen/include/mpi.h                 S0 step, dev/generate_headers.py
    gen/include/mpiabi.h              S0 step, dev/generate_headers.py
    gen/include/mpiwrapper_vtable.h
    gen/mpi_abi/entrypoints.c
    gen/mpiwrapper/wrappers.c
    gen/mpiwrapper/constants.c
    gen/report.txt

The first two are the S0 step and live in `dev/generate_headers.py`, which this
script imports rather than duplicates; `--check` covers all seven.

**What it emits.** S2's mechanical argument classes: passthrough scalars and
passthrough arrays, scalar handles in all three directions, error codes, ranks,
tags, the mapped integer constants, the two mode bitmasks, choice buffers with
their sentinels, in-direction arrays needing element-wise conversion, and the
scalar out-status. S3's first half added the rest of the arrays: out, inout and
status arrays, the extents apis.json records as `*`, the three lifetime rules
of NOTES.md #5.7 -- which turn out to be two mechanical tests, see
stages_past_return() and emit_releases() -- and the six pure ABI-side status
accessors.

S3's second half completed the set: keyvals with their dynamic registry,
output-string buffers with an explicit length, MPI_T's six handle classes and
six enumerated families, MPI_T's rule that any OUT parameter may be null, and
the obj_handle whose class comes from a query rather than from the signature.
Nothing is deferred now. What is not generated is the ledger below, and the
line between them is a matter of judgement rather than of effort: a callback
installed on the way back into user code, a lifetime the wrapper has to track,
a conversion that is per-function rather than per-argument.

**The ledger.** Every one of the 688 entry points is generated, named in
HAND_WRITTEN, or deferred with a reason. The generator fails if one is in none
of the three, and fails if HAND_WRITTEN names something the header does not
have -- which is what makes "nothing was silently dropped" a checked property
rather than a hope.

Usage: dev/generate.py [--check]
"""

import json
import re
import sys
import textwrap
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import generate_headers as gh  # noqa: E402  (path set immediately above)
import layout_hash as lh  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
APIS_JSON = ROOT / "dev" / "apis.json"
HANDWRITTEN_H = ROOT / "src" / "mpiwrapper" / "handwritten.h"

OUT_VTABLE_H = ROOT / "gen" / "include" / "mpiwrapper_vtable.h"
OUT_ENTRYPOINTS_C = ROOT / "gen" / "mpi_abi" / "entrypoints.c"
OUT_WRAPPERS_C = ROOT / "gen" / "mpiwrapper" / "wrappers.c"
OUT_CONSTANTS_C = ROOT / "gen" / "mpiwrapper" / "constants.c"
OUT_REPORT = ROOT / "gen" / "report.txt"

# Frozen tallies, so that a new apis.json or a new ABI header reclassifies
# loudly rather than silently (NOTES.md #3). Each is counted from the artifact
# it describes, never copied from prose.
FROZEN = {
    "entry points": 688,
    "vtable slots": 1366,
    "handle classes": 11,
    "predefined handles": 103,
    "error classes": 80,
    "generated": 563,
    "hand-written": 120,
    # S4b's exit check as a tally rather than an assertion: every ledger entry
    # has a body in src/mpiwrapper/, counted from handwritten.h. A body that
    # disappears -- or a new ledger entry nobody wrote -- fails here rather
    # than becoming one more run-time-reporting stub.
    "hand-written bodies": 120,
    # The five MPI-3.0 deleted from the standard, answered by libmpi_abi in
    # terms of their replacements rather than forwarded to an implementation
    # that need not have them. Frozen, because a sixth is a decision.
    "ABI-side aliases": 5,
    # S3b closed this out: every one of the 688 is now generated or in the
    # ledger. The tally stays, so that a future apis.json or ABI header
    # introducing a class the generator cannot place fails here instead of
    # quietly emitting one more stub.
    "deferred to S3": 0,
    # Where a staged temporary has to outlive the call that made it, which is
    # the property S3's exit check cannot see and the one a wrong body would
    # get wrong silently (NOTES.md #5.7, #6.3). Frozen so that a routine
    # entering or leaving the set is a deliberate act.
    "staged past return": 8,
}

# ---------------------------------------------------------------------------
# The ledger
# ---------------------------------------------------------------------------

# NOTES.md #8's set, with the reason on every line. An entry point here is
# never generated: the vtable initializer names `mpiwrapper_w_MPI_X` instead,
# and the generator checks that name against src/mpiwrapper/handwritten.h -- so
# a ledger entry with no body yet becomes a stub the report names, and a body
# with no ledger entry is a hard error.
#
# Three groups differ from what #8 lists, each recorded there as an S2 finding
# rather than left as a silent divergence:
#
#  - MPI_Wtime and MPI_Wtick are *not* here. #8 lists them under "no error code
#    to map", but a `double` return with no error code is mechanical, and S1
#    put MPI_Wtime in wrappers.c rather than handwritten.c -- which is the
#    artifact that decides it.
#  - The MPI-5.0 `_toint`/`_fromint` handle converters are here. #8's "22
#    Fortran handle converters" predates them; they are the same conversion
#    against a different integer type, and there are 22 more.
#  - MPI_Remove_error_class/_code/_string are here beside MPI_Add_error_*:
#    they are the other half of the dynamic error-code registry of #5.6.
HAND_WRITTEN = {}


def _ledger(reason, *names):
    for name in names:
        assert name not in HAND_WRITTEN, name
        HAND_WRITTEN[name] = reason


_ledger(
    "lifecycle: initialization state the wrapper itself has to track",
    "MPI_Init", "MPI_Init_thread", "MPI_Finalize", "MPI_Abort",
    "MPI_Initialized", "MPI_Finalized", "MPI_Session_init",
    "MPI_Session_finalize",
)
_ledger(
    "ABI introspection: answers about this library, not about the wrapped MPI",
    "MPI_Abi_get_version", "MPI_Abi_get_info", "MPI_Abi_get_fortran_info",
    "MPI_Abi_set_fortran_info", "MPI_Abi_get_fortran_booleans",
    "MPI_Abi_set_fortran_booleans",
)
_ledger(
    "consumes a status in the *in* direction (NOTES.md #5.2)",
    "MPI_Get_count", "MPI_Get_count_c", "MPI_Get_elements",
    "MPI_Get_elements_c", "MPI_Get_elements_x", "MPI_Test_cancelled",
    "MPI_Status_set_cancelled", "MPI_Status_set_elements",
    "MPI_Status_set_elements_c", "MPI_Status_set_elements_x",
)
_ledger(
    "Fortran status converter: memcpy-shaped, not argument-shaped",
    "MPI_Status_c2f", "MPI_Status_f2c", "MPI_Status_c2f08", "MPI_Status_f082c",
)
_ledger(
    "callback registration: installs a trampoline or a pair (NOTES.md #6.1)",
    "MPI_Op_create", "MPI_Op_create_c", "MPI_Comm_create_errhandler",
    "MPI_File_create_errhandler", "MPI_Win_create_errhandler",
    "MPI_Session_create_errhandler", "MPI_Comm_create_keyval",
    "MPI_Type_create_keyval", "MPI_Win_create_keyval",
    # MPI_Keyval_create was here until the deleted MPI-1 entry points moved to
    # the ABI side: it is MPI_Comm_create_keyval under an older name, so it now
    # forwards to that one's slot and the trampoline judgement lives in exactly
    # one place rather than two. See ABI_ALIAS above.
    "MPI_Grequest_start", "MPI_Register_datarep", "MPI_Register_datarep_c",
    "MPI_T_event_register_callback", "MPI_T_event_set_dropped_handler",
)
# S3b settled where the callback boundary falls, because two of its deferred
# entry points sat against this group and the split was a matter of memory. The
# rule is **a callback-typed parameter**, not the word "callback" in a class
# name:
#
#  - MPI_T_event_callback_get_info and _set_info take a CALLBACK_SAFETY, which
#    is an enumerator naming a safety level, not a function. They convert an
#    enum and a registration handle and nothing else, so S3b generates them.
#  - MPI_T_event_handle_free takes an MPI_T_event_free_cb_function. Installing
#    it means a trampoline that converts an implementation registration handle
#    and cb_safety back to the ABI's on the way *in* to user code, which is
#    #6.1's mechanism and #6.2's lifetime question -- the same judgement as the
#    two registrars above, and so the same ledger entry rather than a generated
#    body that would have to invent it.
_ledger(
    "callback registration: the free callback runs on the way back into user "
    "code, so it needs the same trampoline as the two registrars beside it "
    "(NOTES.md #6.1). Its `user_data` can carry the {user_fn, user_extra} "
    "pair, so this one needs no pool.",
    "MPI_T_event_handle_free",
)
_ledger("genuinely variadic", "MPI_Pcontrol")
_ledger(
    "dynamic error codes: renumbered into the ABI's range rather than passed "
    "through (NOTES.md #5.6)",
    "MPI_Add_error_class", "MPI_Add_error_code", "MPI_Add_error_string",
    "MPI_Remove_error_class", "MPI_Remove_error_code",
    "MPI_Remove_error_string",
)
_ledger(
    "spawn: argv, array_of_argv and array_of_errcodes together",
    "MPI_Comm_spawn", "MPI_Comm_spawn_multiple",
)
_ledger(
    "buffer attach/detach: MPI_BUFFER_AUTOMATIC and the buffer's ownership",
    "MPI_Buffer_attach", "MPI_Buffer_attach_c", "MPI_Buffer_detach",
    "MPI_Buffer_detach_c", "MPI_Comm_attach_buffer",
    "MPI_Comm_attach_buffer_c", "MPI_Comm_detach_buffer",
    "MPI_Comm_detach_buffer_c", "MPI_Session_attach_buffer",
    "MPI_Session_attach_buffer_c", "MPI_Session_detach_buffer",
    "MPI_Session_detach_buffer_c",
)
_ledger(
    "handle converter: the reason mpif can run over any MPI",
    *[f"MPI_{cls}_{conv}"
      for cls in ("Comm", "Errhandler", "File", "Group", "Info", "Message",
                  "Op", "Request", "Session", "Type", "Win")
      for conv in ("c2f", "f2c", "toint", "fromint")],
)
_ledger(
    # S7, and the only ledger entry found by running something rather than by
    # reading apis.json: MPICH's attr/baseattr2 asks for MPI_HOST and gets the
    # implementation's MPI_PROC_NULL, which is the ABI's MPI_ANY_SOURCE. The
    # class of an attribute value is not in the signature -- it is whatever
    # the keyval means -- so no parameter kind marks it and no assertion over
    # the emitted text can see it (NOTES.md #5.1).
    "attribute value whose class the keyval decides, not the signature "
    "(NOTES.md #5.4, #5.6)",
    "MPI_Comm_get_attr", "MPI_Win_get_attr",
)
_ledger(
    "output string buffer with no length argument: truncate or error is a "
    "per-parameter judgement (NOTES.md #5.8)",
    "MPI_Error_string", "MPI_Get_library_version", "MPI_Get_processor_name",
    "MPI_Comm_get_name", "MPI_Type_get_name", "MPI_Win_get_name",
    "MPI_Info_get_nthkey", "MPI_Open_port", "MPI_Lookup_name",
    "MPI_File_get_view",
)
# MPI_Waitall and MPI_Ialltoallw were here until S3, as S1's stand-ins for the
# two classes it could not yet generate -- an inout request array whose staged
# temporaries are released at completion, and temporaries that outlive their
# call. Both are generated now, with every other member of their families, and
# the bodies are gone from src/mpiwrapper/handwritten.c.

# ---------------------------------------------------------------------------
# Implemented on the ABI side, in terms of another entry point
# ---------------------------------------------------------------------------

# The five entry points MPI-3.0 *deleted* from the standard. The ABI header
# still declares them -- an ABI is a promise about symbols, and removing one
# would break a binary that was linked years ago -- but an implementation is
# under no obligation to define them any more, and Open MPI main's `libmpi_abi`
# does not: it declares all 688 and defines 683, and these are the five.
#
# Forwarding them through a vtable slot is therefore the wrong shape. The slot
# would resolve against a name the implementation need not have, which is not a
# run-time report but a **link** failure of the whole wrapper -- decision 6's
# promise broken at the one point it cannot cover, because `dev/probe_impl.py`
# asks the compiler and the compiler sees the declaration.
#
# So they are implemented in `libmpi_abi` instead, in terms of the replacements
# MPI-2.0 named for them. That is exact rather than approximate: each is the
# same function under an older name, and the generator checks it below --
# return type, arity, and every parameter type, with the two callback typedefs
# compared by the function type they name rather than by their spelling. What
# it buys is that these five now work over *any* implementation that has the
# MPI-2 attribute interface, which is all of them, instead of over the ones
# that kept the MPI-1 spelling.
#
# There is no slot and no wrapper body, so nothing here reaches
# `libmpiwrapper` at all. `libmpi_abi` still exports all 1376 names, which is
# what the ABI actually promises.
#
# The set is closed, and it is closed by the header rather than by memory: it
# is exactly the entry points the ABI header marks `deprecated: MPI-2.0`, and
# the generator fails if a sixth appears without an entry here. MPI-3.0's other
# deletions -- MPI_Address, MPI_Type_extent, MPI_Errhandler_create and the rest
# -- are not in the ABI header at all, so there is nothing to do for them.
ABI_ALIAS = {
    "MPI_Attr_delete": "MPI_Comm_delete_attr",
    "MPI_Attr_get": "MPI_Comm_get_attr",
    "MPI_Attr_put": "MPI_Comm_set_attr",
    "MPI_Keyval_create": "MPI_Comm_create_keyval",
    "MPI_Keyval_free": "MPI_Comm_free_keyval",
}

# The two typedef pairs the alias check has to accept: MPI_Keyval_create takes
# the MPI-1 spellings of the callbacks MPI_Comm_create_keyval takes, and they
# name the same C function type. Checked rather than asserted -- parse_typedefs
# compares what each expands to -- because a future ABI that changed one of the
# four would otherwise turn a type error into a silent miscall.
#
# Their sentinel values agree too, which is what makes the pass-through exact
# rather than merely well-typed: MPI_NULL_COPY_FN and MPI_COMM_NULL_COPY_FN are
# both 0x0, MPI_DUP_FN and MPI_COMM_DUP_FN both 0x1, MPI_NULL_DELETE_FN and
# MPI_COMM_NULL_DELETE_FN both 0x0. A cast to a pointer type is not an integer
# constant expression, so that pair is checked at run time in
# test/abi_tools_test.c rather than by a _Static_assert here.
ALIAS_TYPEDEF_PAIRS = {
    ("MPI_Copy_function", "MPI_Comm_copy_attr_function"),
    ("MPI_Delete_function", "MPI_Comm_delete_attr_function"),
}

# ---------------------------------------------------------------------------
# Named tables: the per-(routine, parameter) exceptions
# ---------------------------------------------------------------------------

# The join between the header and apis.json is positional and arity-checked, so
# a parameter whose name differs between the two is a disagreement worth naming
# rather than papering over. Every one of these is spelling only -- the *kind*,
# which is what the generator uses, agrees in all of them -- except that
# MPI_Precv_init's partitioned-receive rank is `dest` in the header and
# `source` in apis.json, where apis.json is right and the header reads as a
# copy-paste from MPI_Psend_init.
NAME_DISAGREEMENTS = {
    ("MPI_Graph_create", "indx"): "index",
    ("MPI_Graph_get", "indx"): "index",
    ("MPI_Graph_map", "indx"): "index",
    ("MPI_Request_get_status_any", "indx"): "index",
    ("MPI_Testany", "indx"): "index",
    ("MPI_Waitany", "indx"): "index",
    ("MPI_Status_get_error", "error"): "err",
    ("MPI_Status_set_error", "error"): "err",
    ("MPI_Precv_init", "dest"): "source",
    ("MPI_T_enum_get_item", "indx"): "index",
}
for _f in ("handle_alloc", "handle_free", "read", "readreset", "reset",
           "session_create", "session_free", "start", "stop", "write"):
    NAME_DISAGREEMENTS[(f"MPI_T_pvar_{_f}", "session")] = "pe_session"

# Where MPI_IN_PLACE is legal, by base routine and parameter. apis.json does
# not record it -- every choice buffer is just BUFFER -- and the difference is
# real: MPI_IN_PLACE is (void *)1 in the ABI and (void *)-1 in MPICH, so a site
# that omits the test hands MPICH an address of 1, and a site that adds it
# where the standard does not allow it translates a value that was meant to be
# rejected.
#
# The base names below cover their nonblocking (`I` prefix), persistent
# (`_init` suffix) and large-count (`_c` suffix) forms, which is how MPI-5.0
# states the rule. The neighbourhood collectives are deliberately absent:
# MPI_IN_PLACE is not permitted there.
# Parameters the standard makes IN but some implementation declares without
# `const`, so that the generated local has to lose the qualifier and the call
# has to cast. Named per (routine, parameter) with the implementation that
# forces it, because the alternative -- casting wherever a call happens not to
# compile -- is how a real const violation gets absorbed.
#
# The cast is safe exactly as far as the standard is: MPI-5.0 marks these IN,
# so an implementation that wrote through the pointer would be corrupting a
# caller's array with or without us.
CONST_MISMATCH = {
    ("MPI_Pready_list", "array_of_partitions"):
        "Open MPI 5.0.x declares it `int partition_list[]`",
}

IN_PLACE_SEND = {
    "allgather", "allgatherv", "allreduce", "alltoall", "alltoallv",
    "alltoallw", "exscan", "gather", "gatherv", "reduce", "reduce_scatter",
    "reduce_scatter_block", "scan",
}
IN_PLACE_RECV = {"scatter", "scatterv"}

# ---------------------------------------------------------------------------
# Array extents: how many elements a call touches (S3)
# ---------------------------------------------------------------------------

# `apis.json` gives an array's length as the name of another parameter wherever
# it is one, and the generator uses that directly. The table below is for the
# rest, and there are exactly two kinds of rest:
#
#  - `length: '*'`, meaning the length is a property of an *object* -- the
#    communicator's group, the topology's degrees, the datatype's envelope --
#    so the only place to ask is the implementation, through src/mpiwrapper/
#    extents.c. A body asks *before* the call it wraps and returns the query's
#    error if one fails; the two coincide, since asking a communicator with no
#    topology for its neighbour counts fails with the MPI_ERR_TOPOLOGY the
#    neighbourhood collective would itself have returned.
#
#  - an OUT array the implementation may fill only *partly*: the caller says
#    how much room there is, the object says how much of it gets written, and
#    the two are different numbers. Converting the tail would convert
#    uninitialized elements -- a wrong answer where the garbage happens to
#    collide with an implementation sentinel, and a sanitizer report always.
#    So these carry an `alloc` (the caller's room) and a smaller `conv`.
#
# Everything here is a per-(routine, parameter) judgement, which is why it is a
# named table and not a rule in the emitter.


class Extent:
    """How many elements of an array parameter a call touches.

    `alloc` sizes the temporary and must be valid *before* the call; `conv`
    counts the elements converted and may be computed after it. `probe` is an
    extents.c query, `pre` plain lines that follow it, `reject` guards that
    return an ABI error before anything is allocated, and `post` lines emitted
    after the call for a `conv` that only then becomes knowable.
    """

    __slots__ = ("alloc", "conv", "ctype", "probe", "pre", "reject", "post")

    def __init__(self, alloc, conv=None, ctype="int", probe=None, pre=(),
                 reject=(), post=()):
        self.alloc = alloc
        self.conv = conv if conv is not None else alloc
        self.ctype = ctype
        self.probe = probe            # (declaration line, call expression)
        self.pre = list(pre)
        self.reject = list(reject)    # (condition, ABI error) -> early return
        self.post = list(post)


# The group whose size sizes MPI_Alltoallw's datatype arrays -- the *remote*
# group on an intercommunicator, which extents.c handles.
_COMM_PROBE = ("int ntypes = 0;", "mpiwrapper_comm_extent(comm, &ntypes)")

# The neighbourhood forms instead take one degree per direction. indegree sizes
# the receive arrays and outdegree the send ones, which is the argument order
# here and the reason the two are not interchangeable.
_NEIGHBOR_PROBE = ("int nsendtypes = 0, nrecvtypes = 0;",
                   "mpiwrapper_neighbor_extents(comm, &nrecvtypes, &nsendtypes)")

_DIST_PROBE = ("int nsources = 0, ndestinations = 0;",
               "mpiwrapper_dist_graph_extents(comm, &nsources, &ndestinations)")

ARRAY_EXTENT = {}

# The twelve *alltoallw* forms. Their datatype arrays are the one array class
# S2 could not size at all, and eight of the twelve are also where a staged
# temporary has to outlive its call (NOTES.md #5.7, #6.3).
for _base in ("Alltoallw", "Alltoallw_c", "Alltoallw_init", "Alltoallw_init_c",
              "Ialltoallw", "Ialltoallw_c"):
    for _p in ("sendtypes", "recvtypes"):
        ARRAY_EXTENT[("MPI_" + _base, _p)] = Extent("ntypes", probe=_COMM_PROBE)
for _base in ("Neighbor_alltoallw", "Neighbor_alltoallw_c",
              "Neighbor_alltoallw_init", "Neighbor_alltoallw_init_c",
              "Ineighbor_alltoallw", "Ineighbor_alltoallw_c"):
    ARRAY_EXTENT[("MPI_" + _base, "sendtypes")] = Extent(
        "nsendtypes", probe=_NEIGHBOR_PROBE)
    ARRAY_EXTENT[("MPI_" + _base, "recvtypes")] = Extent(
        "nrecvtypes", probe=_NEIGHBOR_PROBE)

# The graph constructors. `edges` is as long as the last entry of the index
# array says, which is an expression over two other parameters rather than an
# object's property -- so no probe, and the rejection is ours because we are
# about to size a temporary from it.
for _name in ("MPI_Graph_create", "MPI_Graph_map"):
    ARRAY_EXTENT[(_name, "edges")] = Extent(
        "nedges",
        pre=["const int nedges = nnodes > 0 ? indx[nnodes - 1] : 0;"],
        reject=[("nedges < 0", "MPIABI_ERR_ARG")])

ARRAY_EXTENT[("MPI_Dist_graph_create", "destinations")] = Extent(
    "ndestinations",
    pre=["int ndestinations = 0;"],
    reject=[("!mpiwrapper_sum_degrees(degrees, n, &ndestinations)",
             "MPIABI_ERR_ARG")])

# The graph queries, where the caller's maximum and the topology's actual
# extent are different numbers.
ARRAY_EXTENT[("MPI_Graph_get", "edges")] = Extent(
    "maxedges", conv="nedges",
    probe=("int nedges = 0;", "mpiwrapper_graph_nedges(comm, &nedges)"),
    pre=["if (nedges > maxedges) nedges = maxedges;",
         "if (nedges < 0) nedges = 0;"])
ARRAY_EXTENT[("MPI_Graph_neighbors", "neighbors")] = Extent(
    "maxneighbors", conv="nneighbors",
    probe=("int nneighbors = 0;",
           "mpiwrapper_graph_nneighbors(comm, rank, &nneighbors)"),
    pre=["if (nneighbors > maxneighbors) nneighbors = maxneighbors;",
         "if (nneighbors < 0) nneighbors = 0;"])
ARRAY_EXTENT[("MPI_Dist_graph_neighbors", "sources")] = Extent(
    "maxindegree", conv="nsources", probe=_DIST_PROBE,
    pre=["if (nsources > maxindegree) nsources = maxindegree;",
         "if (nsources < 0) nsources = 0;"])
ARRAY_EXTENT[("MPI_Dist_graph_neighbors", "destinations")] = Extent(
    "maxoutdegree", conv="ndestinations", probe=_DIST_PROBE,
    pre=["if (ndestinations > maxoutdegree) ndestinations = maxoutdegree;",
         "if (ndestinations < 0) ndestinations = 0;"])

# MPI_Type_get_contents writes as many datatypes as the envelope says, and the
# standard makes the caller's max_datatypes an upper bound on it -- so the
# staged array is the envelope's size and the *implementation* is told that
# size too, not the caller's. dev/get-contents-extent measures why: Open MPI
# 5.0.6 walks the whole of max_datatypes and dereferences each entry it finds
# there, which for an OUT parameter is whatever the caller's memory held, and
# it segfaults on a legal program with no wrapper in sight. Passing the
# envelope's count satisfies "at least as large as" exactly, is what the
# implementation was going to write either way, and keeps our staged array's
# uninitialized tail out of its reach. A caller's too-*small* max still
# reaches the implementation and is still rejected, because the clamp below is
# a minimum.
ARRAY_EXTENT[("MPI_Type_get_contents", "array_of_datatypes")] = Extent(
    "ndatatypes",
    probe=("int ndatatypes = 0;",
           "mpiwrapper_type_ndatatypes(datatype, &ndatatypes)"),
    pre=["if (ndatatypes > max_datatypes) ndatatypes = max_datatypes;",
         "if (ndatatypes < 0) ndatatypes = 0;"])
ARRAY_EXTENT[("MPI_Type_get_contents_c", "array_of_datatypes")] = Extent(
    "ndatatypes", ctype="MPI_Count",
    probe=("MPI_Count ndatatypes = 0;",
           "mpiwrapper_type_ndatatypes_c(datatype, &ndatatypes)"),
    pre=["if (ndatatypes > max_datatypes) ndatatypes = max_datatypes;",
         "if (ndatatypes < 0) ndatatypes = 0;"])

# The capacity argument that goes with the array above: what the wrapper tells
# the implementation the array holds. Named per (routine, parameter) with the
# reason, because silently passing something other than what the caller passed
# is exactly the kind of thing that must not be a rule in the emitter.
ARG_SUBSTITUTE = {
    ("MPI_Type_get_contents", "max_datatypes"): "ndatatypes",
    ("MPI_Type_get_contents_c", "max_datatypes"): "ndatatypes",
}

# Arrays that MPI_IN_PLACE makes *ignored*, so that the wrapper must not read
# them. MPI-5.0 §6.11: with MPI_IN_PLACE at sendbuf, "sendcounts, sdispls and
# sendtypes are ignored", and a legal program may pass a null pointer for any
# of them. Only the arrays a wrapper reads element by element need naming --
# the ones it merely forwards carry a null through harmlessly, which is why
# this list is the datatype arrays of the six non-neighbourhood alltoallw
# forms and nothing else. (The neighbourhood forms do not take MPI_IN_PLACE at
# all.)
#
# What goes into the temporary instead is the implementation's null datatype.
# Measured rather than assumed: MPICH 4.3.1 and Open MPI 5.0.6 both accept an
# MPI_IN_PLACE alltoallw whose sendtypes are MPI_DATATYPE_NULL *and* one whose
# sendtypes pointer is null, so neither reads the argument at all.
IN_PLACE_IGNORES = {
    ("MPI_" + base, "sendtypes")
    for base in ("Alltoallw", "Alltoallw_c", "Alltoallw_init",
                 "Alltoallw_init_c", "Ialltoallw", "Ialltoallw_c")
}

# MPI_T_event_get_info's two arrays. apis.json gives their length as
# `num_elements`, which is the one place that answer is a *pointer* rather than
# an int: the parameter is INOUT, carrying the caller's capacity in and the
# number the event type actually needs out, and MPI-5.0 15.3.8 lets a caller
# pass a null pointer for it to say it wants neither array. So the capacity is
# read before the call and clamped, and the converted count afterwards is the
# smaller of what the implementation asked for and what there was room for --
# converting past that would convert the uninitialized tail of our own
# temporary.
ARRAY_EXTENT[("MPI_T_event_get_info", "array_of_datatypes")] = Extent(
    "nelements", conv="nelements_out",
    pre=["int nelements = num_elements ? *num_elements : 0;",
         "if (nelements < 0) nelements = 0;"],
    post=["int nelements_out = num_elements ? *num_elements : 0;",
          "if (nelements_out > nelements) nelements_out = nelements;",
          "if (nelements_out < 0) nelements_out = 0;"])

# The status arrays. The *all* forms fill one per request; the *some* forms
# fill `outcount` of them and leave the rest untouched, and outcount is
# MPI_UNDEFINED when no request was active -- hence the clamp rather than a
# bare assignment.
for _name in ("MPI_Waitall", "MPI_Testall", "MPI_Request_get_status_all"):
    ARRAY_EXTENT[(_name, "array_of_statuses")] = Extent("count")
for _name in ("MPI_Waitsome", "MPI_Testsome", "MPI_Request_get_status_some"):
    ARRAY_EXTENT[(_name, "array_of_statuses")] = Extent(
        "incount", conv="nstatuses",
        post=["int nstatuses = *outcount;",
              "if (nstatuses < 0 || nstatuses > incount) nstatuses = 0;"])

# Arrays that cross unconverted although their kind says otherwise, each with
# the reason. This is not the general passthrough rule -- these are named
# exceptions to it.
ARRAY_PASSTHROUGH = {
    ("MPI_Group_range_incl", "ranges"):
        "apis.json calls the whole triplet a RANK, and two thirds of it is: "
        "ranges[i][0] and [1] are ranks in the group. ranges[i][2] is a "
        "*stride*, and mapping it would be a wrong answer rather than an "
        "unnecessary one -- a stride of -1 is MPI_ANY_SOURCE's ABI value and "
        "would arrive at MPICH as -2. Neither of the two genuine ranks can be "
        "a sentinel, since both have to name a member of the group.",
}
ARRAY_PASSTHROUGH[("MPI_Group_range_excl", "ranges")] = \
    ARRAY_PASSTHROUGH[("MPI_Group_range_incl", "ranges")]
ARRAY_PASSTHROUGH[("MPI_Info_create_env", "argv")] = (
    "an array of char*, and neither the pointers nor the strings mean "
    "anything to MPI: apis.json calls the kind ARGUMENT_LIST because the "
    "Fortran binding has to marshal it, and the C binding is main()'s own "
    "argv. The implementation reads it and may write nothing an ABI type "
    "appears in.")

# ---------------------------------------------------------------------------
# Output string buffers with an explicit length (S3b, NOTES.md #5.8)
# ---------------------------------------------------------------------------

# #5.8's ten *without* a length argument are in the ledger above: an
# implementation whose MPI_MAX_* exceeds the ABI's would write past the
# caller's array, so each needs a staged copy and a per-parameter
# truncate-or-error judgement. The ones below are the other set. The caller
# passes the buffer size, so the implementation cannot overflow it and there is
# nothing to convert: a `char *` is a `char *` on both sides, and the length is
# an ordinary passthrough int. The body hands both straight through.
#
# The table names the length parameter rather than deriving it, for one reason:
# it is the *only* thing separating this class from the dangerous one, so
# "which parameter bounds this buffer" has to be stated per site and checked
# against the signature, not inferred from a spelling convention. A STRING out
# parameter with no entry here stays deferred and says so.
#
# The MPI_T half is MPI-5.0 15.3.3's convention: a (buf, buf_len) pair, where
# buf_len is IN as the buffer size and OUT as the string's length plus one, and
# where a null buf or a zero buf_len asks for the length alone.
STRING_OUT_LENGTH = {
    ("MPI_Info_get", "value"): "valuelen",
    ("MPI_Info_get_string", "value"): "buflen",
    ("MPI_Session_get_nth_pset", "pset_name"): "pset_len",
    ("MPI_T_category_get_info", "name"): "name_len",
    ("MPI_T_category_get_info", "desc"): "desc_len",
    ("MPI_T_cvar_get_info", "name"): "name_len",
    ("MPI_T_cvar_get_info", "desc"): "desc_len",
    ("MPI_T_enum_get_info", "name"): "name_len",
    ("MPI_T_enum_get_item", "name"): "name_len",
    ("MPI_T_event_get_info", "name"): "name_len",
    ("MPI_T_event_get_info", "desc"): "desc_len",
    ("MPI_T_pvar_get_info", "name"): "name_len",
    ("MPI_T_pvar_get_info", "desc"): "desc_len",
    ("MPI_T_source_get_info", "name"): "name_len",
    ("MPI_T_source_get_info", "desc"): "desc_len",
}

# ---------------------------------------------------------------------------
# OUT parameters a caller may legally leave null (S3b)
# ---------------------------------------------------------------------------

# The MPI_T query functions all say it, each in its own words and each in the
# section named below: "if any OUT parameter is a NULL pointer, the
# implementation will ignore the parameter and not return a value for it".
# Nothing else in MPI works this way -- an ordinary OUT parameter must point at
# storage -- so this is a property of these routines rather than a general rule,
# and it is recorded per routine with the citation.
#
# What it costs a generated body: a converted OUT parameter is written through
# a local and copied back afterwards, and both halves have to become
# conditional. A parameter that merely passes through needs nothing, because a
# null pointer forwarded to the implementation is exactly what the caller asked
# for -- which is why this set is consulted only where a conversion happens.
NULLABLE_OUT_ROUTINES = {
    "MPI_T_cvar_get_info": "MPI-5.0 15.3.6",
    "MPI_T_pvar_get_info": "MPI-5.0 15.3.7",
    "MPI_T_event_get_info": "MPI-5.0 15.3.8",
    "MPI_T_category_get_info": "MPI-5.0 15.3.9",
    "MPI_T_source_get_info": "MPI-5.0 15.3.8",
}

# ---------------------------------------------------------------------------
# MPI_T's obj_handle (S3b)
# ---------------------------------------------------------------------------

# The three MPI_T handle allocators take `void *obj_handle`, which MPI-5.0
# 15.3.6 describes as "an address to a local variable that stores the object's
# handle" -- so it points at an *ABI* handle whose class is not in the argument
# list at all. The class is whatever a prior get_info call reported in `bind`,
# and the wrapper has to ask the same question before it can convert anything.
#
# Hence one entry per routine naming the query, exactly as ARRAY_EXTENT names a
# probe: the query differs per routine and is not derivable from the parameter.
# The standard makes obj_handle ignored when bind is MPI_T_BIND_NO_OBJECT, and
# permits a null pointer for every OUT parameter of these queries, which is what
# lets src/mpiwrapper/toolobj.c ask for `bind` alone.
TOOL_OBJ_BIND = {
    "MPI_T_cvar_handle_alloc":
        ("int tool_bind = 0;", "mpiwrapper_cvar_bind(cvar_index, &tool_bind)"),
    "MPI_T_pvar_handle_alloc":
        ("int tool_bind = 0;", "mpiwrapper_pvar_bind(pvar_index, &tool_bind)"),
    "MPI_T_event_handle_alloc":
        ("int tool_bind = 0;", "mpiwrapper_event_bind(event_index, &tool_bind)"),
}

# apis.json's BIND_TYPE enumerator -> the handle class it names. Generated into
# constants.c as mpiwrapper_tool_obj_fromabi's switch, so that the guards it
# needs are probed like every other (dev/probe_impl.py reads the generated
# sources, not these).
TOOL_OBJ_CLASSES = {
    "MPI_T_BIND_MPI_COMM": "comm",
    "MPI_T_BIND_MPI_DATATYPE": "datatype",
    "MPI_T_BIND_MPI_ERRHANDLER": "errhandler",
    "MPI_T_BIND_MPI_FILE": "file",
    "MPI_T_BIND_MPI_GROUP": "group",
    "MPI_T_BIND_MPI_INFO": "info",
    "MPI_T_BIND_MPI_MESSAGE": "message",
    "MPI_T_BIND_MPI_OP": "op",
    "MPI_T_BIND_MPI_REQUEST": "request",
    "MPI_T_BIND_MPI_SESSION": "session",
    "MPI_T_BIND_MPI_WIN": "win",
}

# The six status accessors of NOTES.md #5.2 that are *pure ABI-side*: the ABI
# status already holds its three named fields in the ABI's own encoding, put
# there by mpiwrapper_status_toabi, so reading or writing one is a field access
# and the implementation is not involved at all. That is also why these are the
# only generated bodies emitted without a MPIWRAPPER_HAVE_ guard: they work
# over an implementation that does not have the function.
STATUS_FIELD = {
    "MPI_Status_get_source": ("MPI_SOURCE", "get"),
    "MPI_Status_set_source": ("MPI_SOURCE", "set"),
    "MPI_Status_get_tag": ("MPI_TAG", "get"),
    "MPI_Status_set_tag": ("MPI_TAG", "set"),
    "MPI_Status_get_error": ("MPI_ERROR", "get"),
    "MPI_Status_set_error": ("MPI_ERROR", "set"),
}

# Constants a conforming implementation may legitimately not have. Everything
# else is emitted unguarded, so that an implementation missing one fails the
# build naming it rather than silently dropping a mapping (NOTES.md #5.9).
# That is the whole of the judgement; the rest is derived.
#
#  - the optional sized Fortran types, matched by name because that is what
#    makes them optional in the standard rather than anything in the header;
#  - the predefined handles and enumerators the ABI gained after the MPI-3.0
#    floor, which the header marks with `added:` for error classes and does not
#    mark at all elsewhere -- so those are named;
#  - MPI_T's error classes, since the whole tool interface is optional;
#  - MPIX_TYPECLASS_LOGICAL, which is not a standard name at all.
OPTIONAL_PREDEF_RE = re.compile(r"^MPI_(LOGICAL|INTEGER|REAL|COMPLEX)\d+$")
OPTIONAL_CONSTANTS = {
    "MPI_ERRORS_ABORT": "added in MPI-4.0",
    "MPI_SESSION_NULL": "added in MPI-4.0, and with it the whole class",
    "MPIX_TYPECLASS_LOGICAL":
        "a legacy alias the ABI header carries; not a standard name",
    "MPI_COMM_TYPE_HW_UNGUIDED": "added in MPI-4.0",
    "MPI_COMM_TYPE_HW_GUIDED": "added in MPI-4.0",
    "MPI_COMM_TYPE_RESOURCE_GUIDED": "added in MPI-4.1",
    "MPI_COMBINER_VALUE_INDEX": "added in MPI-4.1",
    "MPI_DISPLACEMENT_CURRENT":
        "MPI-IO's, and an implementation built without it has neither the "
        "constant nor MPI_File_set_view",
}

# The enforced floor is MPI-3.0 (decision 3), so a constant the header marks as
# added after MPI-3.1 may be absent from a conforming implementation.
FLOOR_VERSION = (3, 1)

# **Every one of those guards is `#ifdef MPIWRAPPER_HAVE_<name>`, from
# dev/probe_impl.py, and never `#ifdef <the implementation's own name>.**
#
# The second spelling is the obvious one and it is quietly wrong. `#ifdef` sees
# macros; it does not see enumerators, and implementations use both. MPICH
# spells MPI_COMBINER_* and MPI_CART as enumerators, Open MPI spells
# MPI_THREAD_SINGLE, MPI_COMM_TYPE_SHARED and MPI_IDENT that way, and either
# way the `#ifdef` answers *no* for a constant that is right there -- so the
# case drops out, the default arm passes an unmapped value through, and nothing
# fails. It is not hypothetical: MPICH 4.3.1 has MPI_COMBINER_VALUE_INDEX as
# `= 20` in an enum, and an S2 draft that used `#ifdef` on it silently stopped
# translating that combiner.
#
# The probe asks the compiler instead, which sees both, and asks about every
# name in one translation unit rather than one configure test per constant.


def guard(name):
    """The #ifdef a row carries, or None if it is emitted unconditionally."""
    if (OPTIONAL_PREDEF_RE.match(name) or name in OPTIONAL_CONSTANTS
            or name.startswith(("MPI_T_", "MPIABI_T_"))):
        return "MPIWRAPPER_HAVE_" + name
    return None


def guard_reason(name):
    """Why a guarded row is guarded, for the comment beside its #ifdef."""
    if name in OPTIONAL_CONSTANTS:
        return OPTIONAL_CONSTANTS[name]
    if OPTIONAL_PREDEF_RE.match(name):
        return "an optional sized Fortran type"
    return "MPI_T is optional in full"

# ---------------------------------------------------------------------------
# Parsing the ABI header
# ---------------------------------------------------------------------------

_PROTO_RE = re.compile(
    r"^(?P<ret>[A-Za-z_][\w ]*?\s*\**)\s*"
    r"(?P<name>P?MPI_[A-Za-z0-9_]+)\s*"
    r"\((?P<args>[^;{]*)\)\s*;\s*(?P<comment>/\*.*\*/)?\s*$"
)
_HANDLE_TYPEDEF_RE = re.compile(
    r"^typedef\s+struct\s+MPI_ABI_(\w+)\s*\*\s*MPI_(\w+)\s*;")
_HANDLE_CONST_RE = re.compile(
    r"^#define\s+(MPI_[A-Za-z0-9_]+)\s+"
    r"\(\(MPI_([A-Za-z0-9_]+)\s*\*?\)(0x[0-9a-fA-F]+)\)")
_ENUM_MEMBER_RE = re.compile(
    r"^(MPIX?_[A-Za-z0-9_]+)\s*=\s*(-?\w+)\s*,?\s*(?:/\*\s*(?P<note>.*?)\s*\*/)?$")
_ADDED_RE = re.compile(r"added:\s*MPI-(\d+)\.(\d+)")


class Param:
    __slots__ = ("base", "name", "suffix", "kind", "direction", "length",
                 "constant", "cls")

    def __init__(self, base, name, suffix):
        self.base = base
        self.name = name
        self.suffix = suffix
        self.kind = None
        self.direction = None
        self.length = None
        self.constant = False
        self.cls = None

    @property
    def is_array(self):
        return self.length is not None or self.suffix != ""

    @property
    def is_pointer(self):
        return self.base.endswith("*")

    def pointee(self):
        """The implementation-side type an out parameter's local has."""
        assert self.is_pointer, self.base
        return self.base[:-1].strip()

    def elem_type(self):
        """The implementation-side element type of an array parameter."""
        base = self.base[:-1].strip() if (self.is_pointer and not self.suffix) \
            else self.base
        return base.replace("const ", "").strip()


class EntryPoint:
    __slots__ = ("name", "ret", "params", "deprecated", "deprecated_in",
                 "status", "detail", "ret_kind", "unguarded")

    def __init__(self, name, ret, params, deprecated, deprecated_in=None):
        self.name = name
        self.ret = ret
        self.params = params
        self.deprecated = deprecated
        # Which version deprecated it, from the header's own marker. What it
        # separates is not cosmetic: `deprecated: MPI-2.0` is the MPI-1 set
        # MPI-3.0 went on to *delete*, which is ABI_ALIAS's closed set, and
        # everything later is merely deprecated and must still be provided.
        self.deprecated_in = deprecated_in
        self.status = None   # 'generated' | 'hand-written' | 'deferred'
        self.detail = None   # the reason, for gen/report.txt
        self.unguarded = False  # true where the body never calls the impl

    @property
    def base(self):
        """The name without its MPI_/PMPI_ prefix."""
        return self.name.split("MPI_", 1)[1]


def parse_params(text):
    text = text.strip()
    if text == "void":
        return []
    out = []
    for raw in text.split(","):
        raw = raw.strip()
        if raw == "...":
            out.append(Param("...", "...", ""))
            continue
        m = re.match(
            r"^(?P<base>.+?)(?P<name>[A-Za-z_]\w*)\s*(?P<suffix>(\[[^\]]*\])*)$",
            raw)
        if not m:
            raise SystemExit(f"cannot parse parameter {raw!r}")
        out.append(Param(m.group("base").strip(), m.group("name"),
                         m.group("suffix")))
    return out


def parse_prototypes(text):
    """name -> EntryPoint, for all 1376 (MPI_ and PMPI_ alike)."""
    protos = {}
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith(("typedef", "#", "/*", "*", "enum")):
            continue
        m = _PROTO_RE.match(line)
        if not m:
            continue
        comment = m.group("comment") or ""
        since = re.search(r"deprecated:\s*(MPI-[\d.]+)", comment)
        protos[m.group("name")] = EntryPoint(
            m.group("name"), m.group("ret").strip(),
            parse_params(m.group("args")),
            "deprecated" in comment, since.group(1) if since else None)
    return protos


def parse_handle_classes(text):
    """['Comm', 'Datatype', ...] in header order."""
    classes = []
    for line in text.splitlines():
        m = _HANDLE_TYPEDEF_RE.match(line.strip())
        if m and m.group(1) == m.group(2):
            classes.append(m.group(1))
    return classes


def parse_handle_constants(text, classes):
    """'Comm' -> [(macro, value)], header order, aliases dropped."""
    out = {c: [] for c in classes}
    for line in text.splitlines():
        m = _HANDLE_CONST_RE.match(line.strip())
        if m and m.group(2) in out:
            out[m.group(2)].append((m.group(1), int(m.group(3), 16)))
    return out


def parse_enum_members(text):
    """name -> (value, note) for every `MPI_X = n, /* note */` enum line."""
    out = {}
    in_enum = False
    for line in text.splitlines():
        stripped = line.strip()
        if re.match(r"^(typedef\s+)?enum\b.*\{$", stripped):
            in_enum = True
            continue
        if not in_enum:
            continue
        if stripped.startswith("}"):
            in_enum = False
            continue
        m = _ENUM_MEMBER_RE.match(stripped)
        if m:
            out[m.group(1)] = (int(m.group(2), 0), m.group("note") or "")
    return out


_TYPEDEF_FN_RE = re.compile(
    r"^typedef\s+(?P<ret>[A-Za-z_][\w ]*?\s*\**)\s*"
    r"\((?P<name>MPI_[A-Za-z0-9_]+)\)\s*\((?P<args>[^;]*)\)\s*;")


def parse_typedefs(text):
    """name -> (return type, [parameter base types]) for the function typedefs.

    Only what the alias check needs: two typedefs are interchangeable when they
    name the same function type, and that is what this compares -- never the
    spelling of the name, which is the whole point.
    """
    out = {}
    for line in text.splitlines():
        m = _TYPEDEF_FN_RE.match(line.strip())
        if m:
            out[m.group("name")] = (
                m.group("ret").strip(),
                [p.base for p in parse_params(m.group("args"))])
    return out


def check_aliases(protos, typedefs, deprecated_mpi2):
    """An ABI-side alias must be its replacement under an older name.

    Checked, not asserted: return type, arity and every parameter type, with a
    differing pair accepted only when both are function typedefs naming the
    same type and ALIAS_TYPEDEF_PAIRS says so. A future ABI that changed either
    signature fails here rather than producing a forwarder that miscalls.
    """
    missing = deprecated_mpi2 - set(ABI_ALIAS)
    if missing:
        raise SystemExit(
            "the ABI header marks these deleted-in-MPI-3.0 entry points and "
            "ABI_ALIAS does not name a replacement for them: "
            + ", ".join(sorted(missing)))
    for name, target in ABI_ALIAS.items():
        if name not in protos:
            raise SystemExit(f"ABI_ALIAS names {name}, which the ABI header "
                             "does not have")
        if target not in protos:
            raise SystemExit(f"{name}'s replacement {target} is not an entry "
                             "point")
        if target in ABI_ALIAS:
            raise SystemExit(f"{name} forwards to {target}, which is itself an "
                             "ABI-side alias")
        a, b = protos[name], protos[target]
        if a.ret != b.ret or len(a.params) != len(b.params):
            raise SystemExit(f"{name} and {target} no longer have the same "
                             "shape; the ABI-side forwarder would miscall")
        for pa, pb in zip(a.params, b.params):
            if (pa.base, pa.suffix) == (pb.base, pb.suffix):
                continue
            ta, tb = pa.base.rstrip("* "), pb.base.rstrip("* ")
            if ((ta, tb) in ALIAS_TYPEDEF_PAIRS and pa.suffix == pb.suffix
                    and ta in typedefs and typedefs[ta] == typedefs.get(tb)):
                continue
            raise SystemExit(
                f"{name}'s {pa.name} is {pa.base!r} where {target}'s "
                f"{pb.name} is {pb.base!r}, and they are not a known pair of "
                "typedefs for the same type")


def added_after_floor(note):
    m = _ADDED_RE.search(note)
    return bool(m) and (int(m.group(1)), int(m.group(2))) > FLOOR_VERSION


# ---------------------------------------------------------------------------
# Classification
# ---------------------------------------------------------------------------

# apis.json kind -> handle class. The generator checks this against the
# `typedef struct MPI_ABI_X *MPI_X` list in the header, so a twelfth handle
# class in a future ABI reclassifies loudly instead of falling through.
HANDLE_KIND = {
    "COMMUNICATOR": "comm", "DATATYPE": "datatype", "ERRHANDLER": "errhandler",
    "FILE": "file", "GROUP": "group", "INFO": "info", "MESSAGE": "message",
    "OPERATION": "op", "REQUEST": "request", "SESSION": "session",
    "WINDOW": "win",
}

# Kinds that cross the boundary unchanged: counts, displacements, sizes,
# indices, logicals, attribute values and extra state. Representation is what
# an ABI fixes, and dev/type-identity measured size and signedness identical
# for MPI_Aint/Count/Offset on every implementation and platform tried; the
# _Static_asserts in internal.h are what license it (NOTES.md #5.7).
PASSTHROUGH_KIND = {
    "ALLOC_MEM_NUM_BYTES", "ARRAY_LENGTH", "ARRAY_LENGTH_NNI",
    "ARRAY_LENGTH_PI", "ATTRIBUTE_VAL", "ATTRIBUTE_VAL_10", "CAT_INDEX",
    "COMM_SIZE", "COMM_SIZE_PI", "COORDINATE", "CVAR_INDEX",
    "CVAR_INDEX_SPECIAL", "DEGREE", "DIMENSION", "DISPLACEMENT",
    "DISPLACEMENT_NNI", "DISPOFFSET_SMALL", "DROPPED_COUNT", "EVENT_INDEX",
    "EXTRA_STATE", "EXTRA_STATE2", "FILE_DESCRIPTOR", "GENERIC_DTYPE_COUNT",
    "GENERIC_DTYPE_INT", "INDEX", "INFO_VALUE_LENGTH", "KEY", "KEY_INDEX",
    "LOCATION_SMALL", "LOGICAL", "LOGICAL_OPTIONAL", "LOGICAL_VOID", "MATH",
    "NUM_BYTES", "NUM_BYTES_SMALL", "NUM_DIMS", "OFFSET", "PARTITION",
    "POLYDISPLACEMENT", "POLYDISPLACEMENT_AINT_COUNT",
    "POLYDISPLACEMENT_COUNT", "POLYDISPOFFSET", "POLYDTYPE_NUM_ELEM",
    "POLYDTYPE_NUM_ELEM_NNI", "POLYDTYPE_NUM_ELEM_PI", "POLYDTYPE_PACK_SIZE",
    "POLYDTYPE_STRIDE_BYTES",
    # MPI_Pack_external's `position`: the large-count twin of LOCATION_SMALL
    # above, an inout byte offset, which is representation and not spelling in
    # exactly the way its small form is (#5.7).
    "POLYLOCATION",
    # MPI_Info_create_env's `argc`, which is main()'s: apis.json gives it a
    # kind of its own because the Fortran binding has no argument vector to
    # count, and in C it is an int the implementation reads.
    "ARGUMENT_COUNT",
    "POLYNUM_BYTES", "POLYNUM_BYTES_NNI",
    "POLYNUM_PARAM_VALUES", "POLYRMA_DISPLACEMENT", "POLYXFER_NUM_ELEM",
    "POLYXFER_NUM_ELEM_NNI", "PROCESS_GRID_SIZE", "PROFILE_LEVEL",
    "PVAR_INDEX", "RMA_DISPLACEMENT_NNI", "SOURCE_INDEX", "STRING_LENGTH",
    "TOOLENUM_INDEX", "TOOLENUM_SIZE", "TOOLS_NUM_ELEM_SMALL",
    "TOOLS_TICK_COUNT", "TOOL_VAR_VALUE", "TYPECLASS_SIZE", "UPDATE_NUMBER",
    "VERSION", "WINDOW_SIZE", "WIN_ATTACH_SIZE", "XFER_NUM_ELEM",
    "XFER_NUM_ELEM_NNI",
}

# apis.json kind -> integer-constant family. Each family gets a
# `mpiwrapper_<family>_fromabi`/`_toabi` switch in constants.c, generated from
# the ABI header's own enumerators. Which enumerators belong to a family is the
# judgement, and it lives in the table below rather than in the emitter.
SWITCH_KIND = {
    "COMBINER": "combiner",
    "COMM_COMPARISON": "compare",
    "GROUP_COMPARISON": "compare",
    "DISTRIB_ENUM": "distribute",
    "DTYPE_DISTRIBUTION": "darg",
    "LOCK_TYPE": "locktype",
    "ORDER": "order",
    "SPLIT_TYPE": "splittype",
    "THREAD_LEVEL": "threadlevel",
    "TOPOLOGY_TYPE": "topology",
    "TYPECLASS": "typeclass",
    "UPDATE_MODE": "seek",
    # S3b. A keyval is an int the implementation hands out, so unlike every
    # family above it has a dynamic half as well as a predefined one; see
    # SWITCH_DEFAULT.
    "KEYVAL": "keyval",
    # S3b: MPI_T's six enumerated families. Two of them -- cbsafety and
    # sourceorder -- are spelled as enum *types* in the ABI header rather than
    # as ints, which costs nothing here: the mappers take and return int, and C
    # converts in both directions at the call site.
    "BIND_TYPE": "tbind",
    "CALLBACK_SAFETY": "tcbsafety",
    "PVAR_CLASS": "tpvarclass",
    "SOURCE_ORDERING": "tsourceorder",
    "TOOL_VAR_VERBOSITY": "tverbosity",
    "VARIABLE_SCOPE": "tscope",
}

SWITCH_FAMILY_MEMBERS = {
    "combiner": r"^MPI_COMBINER_",
    "compare": r"^MPI_(IDENT|CONGRUENT|SIMILAR|UNEQUAL)$",
    "distribute": r"^MPI_DISTRIBUTE_(BLOCK|CYCLIC|NONE)$",
    "darg": r"^MPI_DISTRIBUTE_DFLT_DARG$",
    "locktype": r"^MPI_LOCK_",
    "order": r"^MPI_ORDER_",
    "splittype": r"^MPI_COMM_TYPE_",
    "threadlevel": r"^MPI_THREAD_",
    "topology": r"^MPI_(GRAPH|CART|DIST_GRAPH|UNDEFINED)$",
    "typeclass": r"^MPIX?_TYPECLASS_",
    "seek": r"^MPI_SEEK_",
    # The ABI's own "Predefined Attribute Keys" enum, in full: the invalid
    # marker, the seven communicator keys and the five window keys. Matched by
    # name rather than by prefix because they share none.
    "keyval": r"^MPI_(KEYVAL_INVALID|TAG_UB|IO|HOST|WTIME_IS_GLOBAL|APPNUM|"
              r"LASTUSEDCODE|UNIVERSE_SIZE|WIN_BASE|WIN_DISP_UNIT|WIN_SIZE|"
              r"WIN_CREATE_FLAVOR|WIN_MODEL)$",
    "tbind": r"^MPI_T_BIND_",
    "tcbsafety": r"^MPI_T_CB_REQUIRE_",
    "tpvarclass": r"^MPI_T_PVAR_CLASS_",
    "tsourceorder": r"^MPI_T_SOURCE_",
    "tverbosity": r"^MPI_T_VERBOSITY_",
    "tscope": r"^MPI_T_SCOPE_",
}

# Two families that no *parameter* has and that exist anyway, because an
# attribute *value* can carry one: MPI_Win_get_attr answers MPI_WIN_CREATE_FLAVOR
# and MPI_WIN_MODEL with an enumerator whose ABI numbering is 311-314 and
# 321-322 and whose MPICH and Open MPI numbering is 1-4 and 1-2. Nothing in
# apis.json marks that, since the parameter is a void * (S7, NOTES.md #5.1);
# src/mpiwrapper/hw_attr.c is what calls these.
ATTRIBUTE_VALUE_FAMILIES = {
    "winflavor": r"^MPI_WIN_FLAVOR_",
    "winmodel":  r"^MPI_WIN_(UNIFIED|SEPARATE)$",
}
SWITCH_FAMILY_MEMBERS.update(ATTRIBUTE_VALUE_FAMILIES)

# Families whose default arm is not "pass the value through". Only keyvals:
# every other family here is a closed set of predefined names, so a value that
# matched no case is one the implementation will reject and passing it on is
# the honest answer. A keyval is not -- the implementation hands out dynamic
# ones at run time, and they can land anywhere, including on top of the ABI's
# predefined 501-507 and 601-605 (NOTES.md #5.6). So the default arm asks the
# registry that MPI_*_create_keyval fills instead.
SWITCH_DEFAULT = {
    "keyval": ("mpiwrapper_keyval_dynamic_fromabi(abi_keyval)",
               "mpiwrapper_keyval_dynamic_toabi(keyval)"),
}

# The one *integer* sentinel (S7). #5.3's rule is about pointers -- MPI_BOTTOM,
# MPI_IN_PLACE, MPI_UNWEIGHTED -- because those are the parameters where a
# distinguished value has to be told apart from an address. MPI_File_set_view's
# `disp` is the same shape one type down: a byte displacement, an open numeric
# domain, plus one value that means "wherever the shared file pointer is". The
# ABI fixes it at (MPI_Offset)-1 and ROMIO -- which is what both MPICH and Open
# MPI use for MPI-IO -- spells it -54278278, so passing it through hands the
# implementation a displacement it rejects (MPICH's own io/setviewcur is what
# found this).
#
# Keyed on the parameter rather than on its kind, and that is the whole reason
# this table exists: apis.json gives `disp` kind OFFSET, which is also every
# ordinary file offset in the library and MPI_File_get_view's *outgoing* disp,
# where the value never means the sentinel. A family keyed on OFFSET would
# convert all of those.
DISPLACEMENT_SENTINEL = {
    ("MPI_File_set_view", "disp"):
        "MPI_DISPLACEMENT_CURRENT, legal here and nowhere else (MPI-5.0 14.3)",
}

# ---------------------------------------------------------------------------
# MPI_T's six handle classes (S3b)
# ---------------------------------------------------------------------------

# These are *not* the eleven handle classes of #5.1 and deliberately do not go
# through their machinery. The eleven have up to 103 predefined values apiece,
# spelled in the implementation as addresses that are not compile-time
# constants, which is what the perfect-hash reverse map exists for. MPI_T's
# have at most two apiece, so each direction is one or two compares -- the
# sentinel shape of #5.3, not the map shape of #5.1. Keeping them out also
# keeps "handle classes: 11" and "predefined handles: 103" the frozen tallies
# they were.
#
# The sentinels do have to be translated, which is the whole reason this is not
# a bit-cast: the ABI fixes MPI_T_PVAR_ALL_HANDLES at (MPI_T_pvar_handle)1,
# Open MPI 5.0.6 spells it (MPI_T_pvar_handle)-1, and MPICH 4.3.1 makes it an
# `extern ... * const` object whose value is not a compile-time constant at all
# -- so it cannot be a case label anywhere and has to be a run-time compare, in
# both directions.
TOOL_HANDLE_KIND = {
    "TOOLS_ENUM": "t_enum",
    "CVAR": "t_cvar_handle",
    "PVAR": "t_pvar_handle",
    "PVAR_SESSION": "t_pvar_session",
    "EVENT_REGISTRATION": "t_event_registration",
    "EVENT_INSTANCE": "t_event_instance",
}

# family -> (implementation type, [sentinel constants], the ABI null used when
# a call fails or is stubbed). The type name is also the guard: an
# implementation without MPI_T events has no MPI_T_event_registration to
# declare, and `sizeof (T)` is what dev/probe_impl.py asks the compiler.
#
# The last two carry no sentinel and no ABI-named null, because the ABI header
# defines none: there is no MPI_T_EVENT_REGISTRATION_NULL. A cast zero is what
# an out parameter gets on a path that produced no handle, which is the only
# place the value is ever read.
TOOL_HANDLE = {
    "t_enum": ("MPI_T_enum", ["MPI_T_ENUM_NULL"], "MPIABI_T_ENUM_NULL"),
    "t_cvar_handle": ("MPI_T_cvar_handle", ["MPI_T_CVAR_HANDLE_NULL"],
                      "MPIABI_T_CVAR_HANDLE_NULL"),
    "t_pvar_handle": ("MPI_T_pvar_handle",
                      ["MPI_T_PVAR_HANDLE_NULL", "MPI_T_PVAR_ALL_HANDLES"],
                      "MPIABI_T_PVAR_HANDLE_NULL"),
    "t_pvar_session": ("MPI_T_pvar_session", ["MPI_T_PVAR_SESSION_NULL"],
                       "MPIABI_T_PVAR_SESSION_NULL"),
    "t_event_registration": ("MPI_T_event_registration", [],
                            "(MPIABI_T_event_registration)0"),
    "t_event_instance": ("MPI_T_event_instance", [],
                         "(MPIABI_T_event_instance)0"),
}

# The two bitmask roles. One mapper round-trips on MPICH and is wrong on Open
# MPI, which gives its window asserts the same bits it gives the first five
# file modes -- so the role belongs in the function name, exactly as for ranks
# and tags (NOTES.md #5.5).
BITMASK_KIND = {"ACCESS_MODE": "filemode", "ASSERT": "winassert"}

RANK_KIND = {"RANK", "RANK_NNI"}
TAG_KIND = {"TAG"}
ERRORCODE_KIND = {"ERROR_CODE", "ERROR_CLASS"}

RANK_MEMBERS = ["MPI_ANY_SOURCE", "MPI_PROC_NULL", "MPI_ROOT", "MPI_UNDEFINED"]
TAG_MEMBERS = ["MPI_ANY_TAG", "MPI_UNDEFINED"]
FILEMODE_MEMBERS = [
    "MPI_MODE_APPEND", "MPI_MODE_CREATE", "MPI_MODE_DELETE_ON_CLOSE",
    "MPI_MODE_EXCL", "MPI_MODE_RDONLY", "MPI_MODE_RDWR", "MPI_MODE_SEQUENTIAL",
    "MPI_MODE_UNIQUE_OPEN", "MPI_MODE_WRONLY",
]
WINASSERT_MEMBERS = [
    "MPI_MODE_NOCHECK", "MPI_MODE_NOPRECEDE", "MPI_MODE_NOPUT",
    "MPI_MODE_NOSTORE", "MPI_MODE_NOSUCCEED",
]

# Return kinds carrying no error code; everything else either returns one or is
# hand-written.
RET_PASSTHROUGH = {"WALL_TIME", "TICK_RESOLUTION", "LOCATION_SMALL",
                   "DISPLACEMENT"}


def in_place_site(ep, p):
    base = ep.base.lower()
    for suffix in ("_c", "_init"):
        if base.endswith(suffix):
            base = base[:-len(suffix)]
    if base.endswith("_init"):
        base = base[:-len("_init")]
    if base.startswith("i") and base[1:] in (IN_PLACE_SEND | IN_PLACE_RECV):
        base = base[1:]
    if p.name == "sendbuf":
        return base in IN_PLACE_SEND
    if p.name == "recvbuf":
        return base in IN_PLACE_RECV
    return False


def convertible_element(kind):
    """True where an array of this kind needs converting element by element."""
    return (kind in HANDLE_KIND or kind in RANK_KIND or kind in TAG_KIND
            or kind in SWITCH_KIND)


def array_extent(ep, p):
    """The Extent of an array parameter, or None if nothing can size it.

    apis.json's own answer wherever it names a parameter; the named table
    otherwise, which is where every `*` and every partly-filled OUT array is
    accounted for by hand.
    """
    named = ARRAY_EXTENT.get((ep.name, p.name))
    if named is not None:
        return named
    if isinstance(p.length, str) and p.length.isidentifier():
        return Extent(p.length, ctype=length_type(ep, p.length))
    return None


def classify(ep, p):
    """One parameter's class, or an 'S3:...' marker naming what blocks it."""
    kind, direction = p.kind, p.direction
    if p.is_array:
        # A `const char datarep[]` is a string that happens to be spelled as an
        # array; apis.json gives it length `*` because a string's length is not
        # in the argument list, which says nothing about how it converts.
        if kind == "STRING" and direction == "in" and p.constant:
            return "string_in"
        if kind in PASSTHROUGH_KIND or (ep.name, p.name) in ARRAY_PASSTHROUGH:
            return "array_passthrough"
        # The graph weights: plain ints, but MPI_UNWEIGHTED and
        # MPI_WEIGHTS_EMPTY are pointer sentinels that have to be translated
        # (NOTES.md #5.3). Nothing else about the array changes.
        if kind == "WEIGHT":
            return "array_weights_in" if direction == "in" \
                else "array_weights_out"
        if array_extent(ep, p) is None:
            return f"S3:array length {p.length!r}"
        if direction == "in" and convertible_element(kind):
            return "array_convert_in"
        if direction == "inout" and kind in HANDLE_KIND:
            return "array_stage_inout"
        if direction == "out" and kind == "STRING":
            # A `char value[valuelen]`: an output string buffer that happens to
            # be spelled as an array because apis.json records its length. Same
            # class as the scalar-spelled ones below, same reason.
            return ("string_out" if (ep.name, p.name) in STRING_OUT_LENGTH
                    else "S3:output string buffer with no length argument")
        if direction == "out":
            # Handles differ in *size* between the ABI and the implementation
            # and statuses differ in layout, so both are staged. The integer
            # families do not: an int is an int on both sides, and these are
            # the OUT arrays NOTES.md #5.7 allows to be mapped in place.
            if kind in HANDLE_KIND:
                return "array_stage_out"
            if kind == "STATUS":
                return "array_status_out"
            if convertible_element(kind):
                return "array_map_out"
        return f"S3:{direction} array of {kind}"
    if kind in HANDLE_KIND:
        return {"in": "handle_in", "out": "handle_out",
                "inout": "handle_inout"}[direction]
    if kind in TOOL_HANDLE_KIND:
        return {"in": "toolhandle_in", "out": "toolhandle_out",
                "inout": "toolhandle_inout"}[direction]
    if kind == "TOOL_MPI_OBJ":
        return "tool_obj"
    if (ep.name, p.name) in DISPLACEMENT_SENTINEL:
        return "displacement_in"
    if kind in PASSTHROUGH_KIND:
        return "passthrough"
    if kind in SWITCH_KIND:
        # inout is its own case rather than a flavour of out: MPI_*_free_keyval
        # passes a keyval *in* and gets MPI_KEYVAL_INVALID back through the
        # same pointer, so a body that only wrote the result would hand the
        # implementation an uninitialized local.
        return {"in": "switch_in", "out": "switch_out",
                "inout": "switch_inout"}[direction]
    if kind in BITMASK_KIND:
        return "bitmask_in" if direction == "in" else "bitmask_out"
    if kind in RANK_KIND:
        return "rank_in" if direction == "in" else "rank_out"
    if kind in TAG_KIND:
        return "tag_in" if direction == "in" else "tag_out"
    if kind in ERRORCODE_KIND:
        return "errorcode_in" if direction == "in" else "errorcode_out"
    if kind == "COLOR":
        return "color"
    if kind == "STRING":
        if p.constant:
            return "string_in"
        # NOTES.md #5.8's split, and the table is what decides it: with an
        # explicit length argument the caller bounds the write and nothing
        # converts, so the buffer and its length both pass through. Without
        # one, an implementation whose MPI_MAX_* exceeds the ABI's writes past
        # the caller's array, and truncate-or-error is a per-parameter
        # judgement -- so those ten are in the ledger, and anything new that
        # arrives without an entry here stays deferred saying so.
        return ("string_out" if (ep.name, p.name) in STRING_OUT_LENGTH
                else "S3:output string buffer with no length argument")
    if "BUFFER" in kind:
        return "buffer_inplace" if in_place_site(ep, p) else "buffer"
    if kind == "STATUS":
        return "status_out" if direction == "out" else "S3:status in"
    return f"S3:{kind}"


# ---------------------------------------------------------------------------
# Type rendering
# ---------------------------------------------------------------------------

_MPI_NAME_RE = re.compile(r"\bMPIX?_[A-Za-z0-9_]+\b")


def abi_type(text):
    """`const MPI_Datatype *` -> `const MPIABI_Datatype *`."""
    return _MPI_NAME_RE.sub(lambda m: gh.rename(m.group(0)), text)


def declare(base, name, suffix=""):
    sep = "" if base.endswith("*") else " "
    return f"{base}{sep}{name}{suffix}"


def abi_decl(p):
    if p.name == "...":
        return "..."
    return declare(abi_type(p.base), "abi_" + p.name, p.suffix)


def slot_type(p):
    return abi_type(p.base) + p.suffix


def renamed(base):
    """True when the ABI and implementation spellings of a type differ."""
    return bool(_MPI_NAME_RE.search(base))


def local_type(p):
    """The implementation-side type of a local holding this parameter."""
    if p.suffix:                       # `const MPI_Aint x[]` -> `const MPI_Aint *`
        # `char *argv[]` decays to `char **`, with the stars together: the
        # space belongs between the type and its pointers, not among them.
        return p.base + ("*" if p.base.endswith("*") else " *")
    return p.base


def array_row(p, name, init, ptr=None):
    """An align() row declaring the local for a decayed array parameter.

    `int ranges[][3]` decays to `int (*)[3]` rather than to `int *`, and the
    declarator wraps the name -- so the row's name column carries it. The
    one-dimensional case is the ordinary `T *const name`.
    """
    dims = p.suffix[len("[]"):]
    if dims:
        return (p.base, f"(*const {name}){dims}", init)
    return ((local_type(p).rstrip() if ptr is None else ptr) + "const", name,
            init)


# ---------------------------------------------------------------------------
# Text helpers
# ---------------------------------------------------------------------------

# S1's wrappers.c puts every macro continuation at column 80: content padded
# to 79 characters, then the backslash.
MACRO_WIDTH = 79


def macro_lines(head, body):
    """`#define X(TARGET) { ... }`, continued at column 80."""
    lines = [head] + body
    out = [ln.ljust(MACRO_WIDTH) + "\\" if len(ln) <= MACRO_WIDTH
           else ln + " \\" for ln in lines[:-1]]
    out.append(lines[-1])
    return out


def align(rows, indent=""):
    """Align `type name = init;` declarations on both columns, as
    clang-format's AlignConsecutiveDeclarations and AlignConsecutiveAssignments
    do -- with a bare trailing `*` hugging the name, which is what makes
    `MPI_Datatype *types` line up with `MPI_Datatype  types_stack[...]`.

    A declaration that still does not fit is wrapped at its ternary, aligned
    under the initializer, which is where the only over-long ones occur."""
    if not rows:
        return []
    split = []
    for t, n, init in rows:
        stars = ""
        while t.endswith("*"):
            t, stars = t[:-1].rstrip(), stars + "*"
        split.append((t, stars, n, init))
    tw = max(len(t) for t, _, _, _ in split)
    sw = max(len(s) for _, s, _, _ in split)
    named = [n for _, _, n, i in split if i is not None]
    nw = max(len(n) for n in named) if named else 0
    out = []
    for t, stars, n, init in split:
        lead = f"{t.ljust(tw)} {stars.rjust(sw)}"
        if init is None:
            out.append(f"{lead}{n};")
            continue
        head = f"{lead}{n.ljust(nw)} = "
        line = f"{head}{init};"
        if len(indent) + len(line) <= 79:
            out.append(line)
        elif " ? " in init:
            cond, rest = init.split(" ? ", 1)
            yes, no = rest.split(" : ", 1)
            pad = " " * len(head)
            out += [f"{head}{cond}", f"{pad}  ? {yes}", f"{pad}  : {no};"]
        else:
            out += [head.rstrip(), f"    {init};"]
    return [ln.rstrip() for ln in out]


def wrap(prefix, args, tail, indent, cont):
    """`prefix(a, b, c)tail`, wrapped at 79 columns."""
    one = f"{indent}{prefix}({', '.join(args)}){tail}"
    if len(one) <= 79:
        return [one]
    # Aligning continuations under the open paren is what clang-format does
    # until an argument no longer fits there; then it falls back to a four-
    # column continuation indent, and so does this.
    widest = max(len(a) for a in args) + 2
    if cont + widest > 79:
        cont = len(indent) + 4
    lines = []
    cur = f"{indent}{prefix}("
    pad = " " * cont
    for i, a in enumerate(args):
        piece = a + ("," if i < len(args) - 1 else f"){tail}")
        if cur.strip() and len(cur) + len(piece) > 79:
            lines.append(cur.rstrip())
            cur = pad
        cur += piece + (" " if i < len(args) - 1 else "")
    lines.append(cur.rstrip())
    return lines


def assign(lhs, expr, ind):
    """`lhs = expr;`, broken under the assignment when it does not fit."""
    one = f"{ind}{lhs} = {expr};"
    if len(one) <= 79:
        return [one]
    return [f"{ind}{lhs} =", f"{ind}    {expr};"]


def signature(ret, name, params):
    """`static int w_MPI_Send(const void *abi_buf, ...)`, wrapped at 79."""
    args = [abi_decl(p) for p in params] or ["void"]
    return wrap(f"{ret} {name}", args, "", "", len(ret) + 1 + len(name) + 1)


# ---------------------------------------------------------------------------
# The wrapper body
# ---------------------------------------------------------------------------

NULL_HANDLE = {
    "comm": "MPIABI_COMM_NULL", "datatype": "MPIABI_DATATYPE_NULL",
    "errhandler": "MPIABI_ERRHANDLER_NULL", "file": "MPIABI_FILE_NULL",
    "group": "MPIABI_GROUP_NULL", "info": "MPIABI_INFO_NULL",
    "message": "MPIABI_MESSAGE_NULL", "op": "MPIABI_OP_NULL",
    "request": "MPIABI_REQUEST_NULL", "session": "MPIABI_SESSION_NULL",
    "win": "MPIABI_WIN_NULL",
}


def local_name(p):
    """The implementation-side local: the parameter with `abi_` dropped, and
    -- for arrays -- without the `array_of_` prefix the standard carries for
    Fortran's benefit, which is how S1 named them."""
    name = p.name
    if p.is_array and name.startswith("array_of_"):
        name = name[len("array_of_"):]
    return name


def scalar_family(p, cls):
    """The conversion family whose `_fromabi`/`_toabi` this parameter uses.

    The role is what picks it, not the C type: `int` is a rank, a tag, an error
    code, a colour or an amode depending only on which parameter it is, which
    is why apis.json's kind is required and the header alone is not enough
    (NOTES.md #5.4, #5.5).
    """
    if cls.startswith(("rank", "tag", "errorcode")):
        return cls.split("_")[0]
    if cls.startswith("switch"):
        return SWITCH_KIND[p.kind]
    if cls.startswith("bitmask"):
        return BITMASK_KIND[p.kind]
    if cls == "array_convert_in":
        # The element's family, by the same rule.
        if p.kind in HANDLE_KIND:
            return HANDLE_KIND[p.kind]
        if p.kind in RANK_KIND:
            return "rank"
        if p.kind in TAG_KIND:
            return "tag"
        return SWITCH_KIND[p.kind]
    raise AssertionError(cls)


class Staged:
    """One array converted through a temporary rather than in place.

    `mode` is the direction of the copying, not of the parameter: "in" fills
    the temporary before the call, "out" reads it after, "inout" does both, and
    "status" is "out" plus the MPI_STATUSES_IGNORE short circuit.

    `skip` is the condition under which the caller wants no array back at all,
    and `skip_arg` what the implementation is told instead. Two things arrive
    at the same shape: MPI_STATUSES_IGNORE, and MPI_T's rule that a null OUT
    pointer means "do not return this". Both must be tested *before* anything
    is allocated, and both must suppress the copy back -- which for the null
    case is not an optimization but the whole point, since the destination is
    the null pointer.
    """

    __slots__ = ("p", "name", "elem", "mode", "extent", "family", "ignored",
                 "skip", "skip_arg")

    def __init__(self, p, name, elem, mode, extent, family, ignored=False,
                 skip=None, skip_arg=None):
        self.p = p
        self.name = name
        self.elem = elem
        self.mode = mode
        self.extent = extent
        self.family = family
        self.ignored = ignored   # MPI_IN_PLACE makes this argument ignored
        self.skip = skip
        self.skip_arg = skip_arg

    def fill(self, lead):
        """The conversion loop's assignment, `lead` being everything up to and
        including the `= `."""
        convert = f"mpiwrapper_{self.family}_fromabi(abi_{self.p.name}[i])"
        if not self.ignored:
            return [f"{lead}{convert};"]
        pad = " " * (len(lead) + 4)
        return [f"{lead}{self.p.name}_ignored",
                f"{pad}? {impl_null_handle(self.p.kind)}",
                f"{pad}: {convert};"]


def impl_null_handle(kind):
    """The implementation's null handle of a class, as it spells it."""
    return "MPI_" + HANDLE_KIND[kind].upper() + "_NULL"


def emit_body(ep):
    """The lines of BODY_MPI_X(TARGET), or None if some class blocks it."""
    if ep.name in STATUS_FIELD:
        return emit_status_field(ep)

    decls, outs, post, args = [], [], [], []
    staged, checks, checked_lengths = [], [], set()
    probes, pre, rejects, extent_post, maps, releases = [], [], [], [], [], []
    late_decls = []
    handle_out = False
    status_local = False

    # MPI_T's query functions let a caller pass a null pointer for any OUT
    # parameter to say it does not want that answer. Only a *converted* out
    # parameter cares: one that passes through carries the null to the
    # implementation, which is exactly what the caller asked for.
    nullable = ep.name in NULLABLE_OUT_ROUTINES

    def out_pointer(p, name, abi):
        """What a nullable OUT parameter passes: the local's address, or the
        null the caller asked for.

        Declared rather than written inline as `abi_x ? &x : NULL`, because the
        argument list of the implementation call must contain no ABI-typed
        parameter at all -- that assertion is a grep over the emitted text and
        it is the generator's load-bearing one, so it is worth a named local
        rather than an exception (NOTES.md #3).
        """
        outs.append((p.pointee() + " *const", name + "_p",
                     f"{abi} ? &{name} : NULL"))
        return name + "_p"

    def writeback(abi, statement):
        """`*abi_x = ...;`, guarded by `if (abi_x)` where the caller may have
        passed nothing to write to."""
        if not nullable:
            return (statement,)
        one = f"if ({abi}) {statement}"
        return (one,) if len(one) <= 79 - 6 else (f"if ({abi})", "  " + statement)

    def use(extent, elem, allocates=True):
        """Record what an Extent needs emitted before the call, once each.

        The `< 0` check is per length and the overflow check per array, which
        is what S1's MPI_Type_create_struct does and what keeps the two forms
        of a large-count routine reading alike.
        """
        if extent.probe and extent.probe not in probes:
            probes.append(extent.probe)
        for line in extent.pre:
            if line not in pre:
                pre.append(line)
        for guard in extent.reject:
            if guard not in rejects:
                rejects.append(guard)
        for line in extent.post:
            if line not in extent_post:
                extent_post.append(line)
        # An in-place map allocates nothing, so a length check here would
        # preempt the implementation's own argument error rather than protect
        # anything; an extents.c answer is non-negative by construction.
        if not allocates or not any(q.name == extent.alloc
                                    for q in ep.params):
            return extent
        if extent.alloc not in checked_lengths:
            checked_lengths.add(extent.alloc)
            checks.append((extent.alloc,
                           f"if ({extent.alloc} < 0) return MPIABI_ERR_COUNT;"))
        if extent.ctype != "int":
            # The length is wider than size_t may be, so the byte count can
            # overflow before mpiwrapper_stage sees it.
            checks.append((extent.alloc,
                           f"if ((uint64_t){extent.alloc} > SIZE_MAX / "
                           f"sizeof({elem}))"))
            checks.append((extent.alloc, "  return MPIABI_ERR_COUNT;"))
        return extent

    for p in ep.params:
        cls = p.cls
        name = local_name(p)
        abi = "abi_" + p.name
        if cls == "passthrough" and (ep.name, p.name) in ARG_SUBSTITUTE:
            # The local is still the caller's value -- the extent clamp reads
            # it -- but what reaches the implementation is the substitute.
            decls.append(("const " + p.base, name, abi))
            args.append(ARG_SUBSTITUTE[(ep.name, p.name)])
        elif cls == "passthrough":
            if p.is_pointer:
                init = f"({p.base.rstrip()})" + abi if renamed(p.base) else abi
                decls.append((p.base + "const", name, init))
            else:
                decls.append(("const " + p.base, name, abi))
            args.append(name)
        elif cls == "array_passthrough":
            ptr = local_type(p).rstrip()
            if (ep.name, p.name) in CONST_MISMATCH:
                ptr = ptr.replace("const ", "", 1)
            init = (f"({ptr})" + abi
                    if renamed(p.base) or (ep.name, p.name) in CONST_MISMATCH
                    else abi)
            decls.append(array_row(p, name, init, ptr))
            args.append(name)
        elif cls in ("buffer", "buffer_inplace"):
            # Which sentinels are legal is a property of the parameter, and so
            # is whether the result may be written: `sendbuf` returns
            # `const void *` and `recvbuf` `void *`, and the choice follows the
            # C type rather than apis.json's `constant`, which is about the
            # Fortran binding.
            role = "sendbuf" if "const" in p.base else "recvbuf"
            fn = (f"mpiwrapper_{role}_inplace_fromabi"
                  if cls == "buffer_inplace" else f"mpiwrapper_{role}_fromabi")
            decls.append((p.base + "const", name, f"{fn}({abi})"))
            args.append(name)
            if cls == "buffer_inplace":
                # One flag per array the standard makes ignored when *this*
                # buffer is MPI_IN_PLACE, named after the array rather than
                # after the buffer, so a routine with two of them cannot
                # conflate them.
                for q in ep.params:
                    if (ep.name, q.name) in IN_PLACE_IGNORES:
                        decls.append(("const int", q.name + "_ignored",
                                      f"{abi} == MPIABI_IN_PLACE"))
        elif cls == "handle_in":
            cl = HANDLE_KIND[p.kind]
            if p.is_pointer:
                # MPI_Cancel and MPI_Request_free take `MPI_Request *` and
                # apis.json calls the parameter `in`, which is right: they read
                # the handle. The local still has to be an lvalue of the
                # implementation's type, and nothing is written back.
                decls.append((p.pointee(), name,
                              f"mpiwrapper_{cl}_fromabi(*{abi})"))
                args.append("&" + name)
            else:
                decls.append(("const " + p.base, name,
                              f"mpiwrapper_{cl}_fromabi({abi})"))
                args.append(name)
        elif cls == "handle_out":
            cl = HANDLE_KIND[p.kind]
            outs.append((p.pointee(), name,
                         impl_null_handle(p.kind) if nullable else None))
            pad = " " * (len(f"*{abi} = ") + 4)
            group = (f"*{abi} = (ierror == MPI_SUCCESS)",
                     f"{pad}? mpiwrapper_{cl}_toabi({name})",
                     f"{pad}: {NULL_HANDLE[cl]};")
            if nullable:
                group = (f"if ({abi})",) + tuple("  " + ln for ln in group)
                args.append(out_pointer(p, name, abi))
            else:
                args.append("&" + name)
            post.append(group)
            handle_out = True
        elif cls == "handle_inout":
            cl = HANDLE_KIND[p.kind]
            decls.append((p.pointee(), name, f"mpiwrapper_{cl}_fromabi(*{abi})"))
            post.append((f"*{abi} = mpiwrapper_{cl}_toabi({name});",))
            args.append("&" + name)
            handle_out = True
        elif cls in ("rank_in", "tag_in", "errorcode_in", "switch_in",
                     "bitmask_in"):
            fn = "mpiwrapper_" + scalar_family(p, cls)
            decls.append(("const " + p.base, name, f"{fn}_fromabi({abi})"))
            args.append(name)
        elif cls in ("rank_out", "tag_out", "errorcode_out", "switch_out",
                     "bitmask_out"):
            fn = "mpiwrapper_" + scalar_family(p, cls)
            outs.append((p.pointee(), name, "0" if nullable else None))
            post.append(writeback(abi, f"*{abi} = {fn}_toabi({name});"))
            args.append(out_pointer(p, name, abi) if nullable else "&" + name)
        elif cls == "switch_inout":
            # The keyval of MPI_*_free_keyval, and nothing else so far: the
            # implementation reads the value it is given and writes
            # MPI_KEYVAL_INVALID back through the same pointer, so both halves
            # of the conversion are needed and the local must start as the
            # caller's value rather than as whatever the stack held.
            fn = "mpiwrapper_" + scalar_family(p, cls)
            decls.append((p.pointee(), name, f"{fn}_fromabi(*{abi})"))
            post.append((f"*{abi} = {fn}_toabi({name});",))
            args.append("&" + name)
        elif cls == "toolhandle_in":
            fam = TOOL_HANDLE_KIND[p.kind]
            decls.append(("const " + p.base, name,
                          f"mpiwrapper_{fam}_fromabi({abi})"))
            args.append(name)
        elif cls == "toolhandle_out":
            fam = TOOL_HANDLE_KIND[p.kind]
            sentinels = TOOL_HANDLE[fam][1]
            outs.append((p.pointee(), name,
                         (sentinels[0] if sentinels else "NULL")
                         if nullable else None))
            pad = " " * (len(f"*{abi} = ") + 4)
            group = (f"*{abi} = (ierror == MPI_SUCCESS)",
                     f"{pad}? mpiwrapper_{fam}_toabi({name})",
                     f"{pad}: {TOOL_HANDLE[fam][2]};")
            if nullable:
                group = (f"if ({abi})",) + tuple("  " + ln for ln in group)
                args.append(out_pointer(p, name, abi))
            else:
                args.append("&" + name)
            post.append(group)
            handle_out = True
        elif cls == "toolhandle_inout":
            fam = TOOL_HANDLE_KIND[p.kind]
            decls.append((p.pointee(), name,
                          f"mpiwrapper_{fam}_fromabi(*{abi})"))
            post.append((f"*{abi} = mpiwrapper_{fam}_toabi({name});",))
            args.append("&" + name)
            handle_out = True
        elif cls == "tool_obj":
            # The class of the object this points at is not in the argument
            # list; it is what a prior get_info reported in `bind`, so the
            # wrapper asks the same question before it can convert anything.
            # The query goes in `probes` so that a failure returns the
            # implementation's own error before a handle has been produced,
            # and the two locals go *after* it, since both read its answer.
            probe = TOOL_OBJ_BIND[ep.name]
            if probe not in probes:
                probes.append(probe)
            late_decls.append(("union mpiwrapper_tool_obj", "obj_storage",
                               None))
            late_decls.append(
                ("void *const", name,
                 f"mpiwrapper_tool_obj_fromabi(tool_bind, {abi}, "
                 "&obj_storage)"))
            args.append(name)
        elif cls == "string_out":
            # NOTES.md #5.8: the caller passed the buffer's size, so the
            # implementation cannot overflow it and a char is a char on both
            # sides. The length parameter is an ordinary passthrough int and
            # is named here only so that the generator can check it is really
            # in the signature.
            length = STRING_OUT_LENGTH[(ep.name, p.name)]
            if not any(q.name == length for q in ep.params):
                raise SystemExit(
                    f"{ep.name}: STRING_OUT_LENGTH names {length!r} as what "
                    f"bounds {p.name!r}, and it is not a parameter")
            decls.append(array_row(p, name, abi) if p.suffix
                         else (local_type(p).rstrip() + "const", name, abi))
            args.append(name)
        elif cls == "displacement_in":
            decls.append(("const " + p.base, name,
                          f"mpiwrapper_displacement_fromabi({abi})"))
            args.append(name)
        elif cls == "color":
            decls.append(("const " + p.base, name,
                          f"{abi} == MPIABI_UNDEFINED ? MPI_UNDEFINED : {abi}"))
            args.append(name)
        elif cls == "string_in":
            decls.append((local_type(p).rstrip() + "const", name, abi))
            args.append(name)
        elif cls == "status_out":
            decls.append(("const int", "ignore",
                          f"{abi} == MPIABI_STATUS_IGNORE"))
            args.append("ignore ? MPI_STATUS_IGNORE : &status")
            # _keep_error: a single OUT status must come back with the
            # caller's MPI_ERROR untouched (MPI-5.0 3.2.5 -- only the
            # multiple-completion calls of 3.7.5 set it, and those take an
            # array, which is the other site below). The implementation
            # cannot honour that itself, because what it is handed is a
            # temporary of ours (S7).
            post.append((f"if (!ignore) "
                         f"mpiwrapper_status_toabi_keep_error(&status, {abi});",))
            status_local = True
        elif cls in ("array_convert_in", "array_stage_inout",
                     "array_stage_out", "array_status_out"):
            elem = "MPI_Status" if cls == "array_status_out" else p.elem_type()
            extent = use(array_extent(ep, p), elem)
            mode = {"array_convert_in": "in", "array_stage_inout": "inout",
                    "array_stage_out": "out",
                    "array_status_out": "status"}[cls]
            family = (scalar_family(p, "array_convert_in")
                      if cls != "array_status_out" else None)
            skip = skip_arg = None
            if cls == "array_status_out":
                # NULL in the ABI, and the test has to come before we allocate
                # room for statuses nobody wants (NOTES.md #5.7).
                skip, skip_arg = "ignore", "MPI_STATUSES_IGNORE"
                decls.append(("const int", skip,
                              f"{abi} == MPIABI_STATUSES_IGNORE"))
            elif nullable and mode == "out":
                # MPI_T's null-means-do-not-return rule again. Here it is not
                # merely an optimization: the copy back would write through the
                # null pointer the caller passed.
                skip, skip_arg = name + "_absent", "NULL"
                decls.append(("const int", skip, f"{abi} == NULL"))
            staged.append(Staged(p, name, elem, mode, extent, family,
                                 (ep.name, p.name) in IN_PLACE_IGNORES,
                                 skip, skip_arg))
            args.append(f"{skip} ? {skip_arg} : {name}" if skip else name)
            # The out direction of a handle array can fail the same way a
            # scalar out handle can: a dynamic handle whose bits collide with
            # the ABI's predefined range (NOTES.md #5.1).
            if mode in ("out", "inout") and p.kind in HANDLE_KIND:
                handle_out = True
            if mode == "inout" and p.kind == "REQUEST":
                releases.append(("array", staged[-1]))
        elif cls == "array_map_out":
            # The one legitimate in-place case (NOTES.md #5.7): an OUT array
            # whose element is an int on both sides, so the implementation
            # writes straight into the caller's array and each element is
            # mapped where it lies. No const, no concurrent-read expectation,
            # no restore path.
            extent = use(array_extent(ep, p), p.elem_type(), allocates=False)
            decls.append(array_row(p, name, abi))
            maps.append((p, name, extent))
            args.append(name)
        elif cls in ("array_weights_in", "array_weights_out"):
            # Plain ints, but MPI_UNWEIGHTED and MPI_WEIGHTS_EMPTY are pointer
            # sentinels with different values on the two sides.
            fn = ("mpiwrapper_weights_fromabi" if cls == "array_weights_in"
                  else "mpiwrapper_weights_out_fromabi")
            decls.append(array_row(p, name, f"{fn}({abi})"))
            args.append(name)
        else:
            return None

    for p in ep.params:
        # A scalar inout request is a completion site too, and its pre-call
        # value has to be kept: the local is overwritten by the call, and the
        # release is keyed on what the request was.
        if p.cls == "handle_inout" and p.kind == "REQUEST":
            name = local_name(p)
            decls.append(("const " + p.pointee(), name + "_before", name))
            releases.append(("scalar", name))

    return assemble(ep, decls, outs, post, args, staged, checks, handle_out,
                    status_local, probes, pre, rejects, extent_post, maps,
                    releases, late_decls)


def length_type(ep, length):
    for p in ep.params:
        if p.name == length:
            return p.base
    raise SystemExit(f"{ep.name}: array length {length!r} is not a parameter")


def stages_past_return(ep):
    """True where this entry point's staged temporaries outlive its call.

    The whole of S3's lifetime question is in this predicate: an in-direction
    array that has to be converted, in a routine that hands back a request the
    implementation may still be reading it through. Everything else is scoped
    to the call.
    """
    return (ep.status == "generated" and request_out(ep) is not None
            and any(p.cls == "array_convert_in" for p in ep.params))


def request_out(ep):
    """The out request parameter, for the routines whose staged temporaries
    have to outlive the call."""
    for p in ep.params:
        if p.cls == "handle_out" and p.kind == "REQUEST":
            return p
    return None


def assemble(ep, decls, outs, post, args, staged, checks, handle_out,
             status_local, probes, pre, rejects, extent_post, maps, releases,
             late_decls=()):
    if staged and any(s.mode == "in" for s in staged) and request_out(ep):
        return assemble_outliving(ep, decls, args, staged, checks, probes, pre,
                                  rejects)

    # Absolute indentation, including the two columns the body macro adds:
    # the outer brace sits at column 3 and statements at column 5, which is
    # where S1's clang-formatted bodies put them.
    ind = "    "
    body = ["  {"]

    # The length checks sit immediately after the declaration of the length
    # they check, which splits the declaration group exactly where S1's
    # MPI_Type_create_struct splits it.
    if checks:
        cut = 1 + max(i for i, (t, n, v) in enumerate(decls)
                      if n in {c[0] for c in checks})
        body += [ind + ln for ln in align(decls[:cut], ind)]
        body += [ind + text for _, text in checks]
        body.append("")
        decls = decls[cut:]

    if decls:
        body += [ind + ln for ln in align(decls, ind)]
        body.append("")

    body += emit_extent_queries(ep, probes, pre, rejects, ind)

    if late_decls:
        # Locals whose initializer reads a probe's answer, so they cannot sit
        # in the declaration group above it. Aligned one at a time rather than
        # as a group: the widest of them is a union type that would pad the
        # rest past the margin for nothing.
        for row in late_decls:
            body += [ind + ln for ln in align([row], ind)]
        body.append("")

    if staged:
        rows = []
        for s in staged:
            rows.append(
                (s.elem,
                 f"{s.name}_stack[MPIWRAPPER_STAGE_BYTES / sizeof({s.elem})]",
                 None))
            rows.append((s.elem + " *", s.name, "NULL"))
        rows.append(("int", "abi_ierror", "MPIABI_ERR_INTERN"))
        body += [ind + ln for ln in align(rows, ind)]
        body.append("")
        for s in staged:
            # An array the caller does not want is not allocated at all, which
            # is the point of testing MPI_STATUSES_IGNORE -- or MPI_T's null
            # OUT pointer -- this early.
            guarded = s.skip is not None
            inner = ind + "  " if guarded else ind
            if guarded:
                if body[-1] != "":
                    body.append("")
                body.append(ind + f"if (!{s.skip}) {{")
            body += wrap(f"{s.name} = mpiwrapper_stage",
                         [f"{s.name}_stack", f"sizeof {s.name}_stack",
                          f"(size_t){s.extent.alloc}", f"sizeof *{s.name}"],
                         ";", inner,
                         len(inner) + len(s.name) + len(" = mpiwrapper_stage("))
            body.append(inner + f"if (!{s.name}) goto done;")
            if s.mode == "status":
                # Zeroed rather than left as stack garbage, so that a status
                # the implementation does not fill converts reproducibly.
                body += wrap("memset",
                             [s.name, "0", f"(size_t){s.extent.alloc} * "
                              f"sizeof *{s.name}"], ";", inner,
                             len(inner) + len("memset("))
            if guarded:
                body.append(ind + "}")
                body.append("")
        if body[-1] != "":
            body.append("")
        loops = []
        for s in staged:
            if s.mode not in ("in", "inout"):
                continue
            loops.append(ind + f"for ({s.extent.ctype} i = 0; "
                               f"i < {s.extent.alloc}; ++i)")
            loops += s.fill(f"{ind}  {s.name}[i] = ")
        if loops:
            body += loops
            body.append("")
        body.append("    {")
        ind = "      "

    if status_local:
        body.append(ind + "MPI_Status status;")
        body.append(ind + "memset(&status, 0, sizeof status);")
        body.append("")

    rows = list(outs)
    if ep.ret == "int":
        rows.append(("const int", "ierror", None))
        lines = [ind + ln for ln in align(rows, ind)]
        lines[-1] = lines[-1][:-1] + " ="
        oneline = wrap("TARGET", args, ";", lines[-1] + " ",
                       len(ind) + 4 + len("TARGET("))
        if len(oneline) == 1:
            body += lines[:-1] + oneline
        else:
            body += lines
            body += wrap("TARGET", args, ";", ind + "    ",
                         len(ind) + 4 + len("TARGET("))
    else:
        body += [ind + ln for ln in align(rows, ind)]
        body += wrap("return TARGET", args, ";", ind,
                     len(ind) + len("return TARGET("))
        body.append("  }")
        return body

    rel = emit_releases(releases, ind)
    wb = emit_writeback(staged, maps, extent_post, ind)
    tail = rel + ([""] if rel and wb else []) + wb
    if (post and outs) or tail:
        body.append("")
    for group in post:
        body += [ind + ln for ln in group]
    if post and tail:
        body.append("")
    body += tail

    if staged:
        body.append(ind + "abi_ierror = mpiwrapper_errorcode_toabi(ierror);")
        if handle_out:
            line = ind + ("if (mpiwrapper_take_handle_error()) "
                          "abi_ierror = MPIABI_ERR_INTERN;")
            if len(line) <= 79:
                body.append(line)
            else:
                body.append(ind + "if (mpiwrapper_take_handle_error())")
                body.append(ind + "  abi_ierror = MPIABI_ERR_INTERN;")
        body.append("    }")
        body.append("")
        body.append("  done:")
        for s in staged:
            body.append("    " + f"mpiwrapper_unstage({s.name}, {s.name}_stack);")
        body.append("    return abi_ierror;")
    else:
        if handle_out:
            body.append(ind + "if (mpiwrapper_take_handle_error()) "
                              "return MPIABI_ERR_INTERN;")
        body.append(ind + "return mpiwrapper_errorcode_toabi(ierror);")

    body.append("  }")
    return body


def assemble_outliving(ep, decls, args, staged, checks, probes, pre, rejects):
    """The body of a routine whose staged arrays outlive it.

    A nonblocking or persistent collective may keep reading the arrays it was
    given until the operation completes or the request is freed (NOTES.md
    #5.7), and the arrays it is given are *ours*. So the temporaries go on the
    heap in one block owned by the request, and the release rule that frees
    them is the same one every completion entry point runs.

    The two lifetimes NOTES.md #5.7 distinguishes -- freed at completion for
    the nonblocking forms, at MPI_Request_free for the persistent ones -- need
    no flag here and no distinction in the emitted text: both are exactly
    "when the implementation nulls the handle".
    """
    p_request = request_out(ep)
    others = [p for p in ep.params
              if p.cls in ("handle_out", "handle_inout") and p is not p_request]
    if others:
        raise SystemExit(
            f"{ep.name}: a routine that stages past its return may produce no "
            "handle but the request: " + ", ".join(p.name for p in others))
    elems = {s.elem for s in staged}
    if len(elems) != 1 or any(s.mode != "in" for s in staged):
        raise SystemExit(
            f"{ep.name}: temporaries that outlive their call are carved out of "
            "one block, so they must all be in-direction and of one element "
            f"type; got {sorted(elems)}")
    elem = elems.pop()
    ind = "    "
    body = ["  {"]

    if checks:
        cut = 1 + max(i for i, (t, n, v) in enumerate(decls)
                      if n in {c[0] for c in checks})
        body += [ind + ln for ln in align(decls[:cut], ind)]
        body += [ind + text for _, text in checks]
        body.append("")
        decls = decls[cut:]
    if decls:
        body += [ind + ln for ln in align(decls, ind)]
        body.append("")
    body += emit_extent_queries(ep, probes, pre, rejects, ind)

    total = " + ".join(f"(size_t){s.extent.alloc}" for s in staged)
    rows = [("const size_t", "nstaged", total), (elem + " *", "block", "NULL")]
    body += [ind + ln for ln in align(rows, ind)]
    body.append(ind + "if (nstaged <= SIZE_MAX / sizeof *block)")
    body.append(ind + "  block = malloc(nstaged * sizeof *block);")
    body.append(ind + "if (!block && nstaged > 0) {")
    body += null_out_handles(ep, ind + "  ")
    body.append(ind + "  return MPIABI_ERR_INTERN;")
    body.append(ind + "}")

    offset = None
    rows = []
    for s in staged:
        init = "block" if offset is None else f"block + {offset}"
        rows.append((elem + " *const", s.name, init))
        offset = s.extent.alloc if offset is None \
            else f"{offset} + {s.extent.alloc}"
    body += [ind + ln for ln in align(rows, ind)]
    body.append("")

    for s in staged:
        body.append(ind + f"for ({s.extent.ctype} i = 0; "
                          f"i < {s.extent.alloc}; ++i)")
        body += s.fill(f"{ind}  {s.name}[i] = ")
    body.append("")

    name = local_name(p_request)
    body.append(ind + f"{p_request.pointee()} {name};")
    body += wrap("const int ierror = TARGET", args, ";", ind,
                 len(ind) + len("const int ierror = TARGET("))
    body.append(ind + "if (ierror != MPI_SUCCESS) {")
    body.append(ind + "  free(block);")
    body += null_out_handles(ep, ind + "  ")
    body.append(ind + "  return mpiwrapper_errorcode_toabi(ierror);")
    body.append(ind + "}")
    body.append("")
    body.append(ind + f"*abi_{p_request.name} = "
                      f"mpiwrapper_request_toabi({name});")
    body.append(ind + "if (mpiwrapper_take_handle_error()) "
                      "return MPIABI_ERR_INTERN;")
    body.append("")
    # The operation is already in flight, so a table that cannot take the block
    # leaves nothing safe to do: freeing it would be a use-after-free while the
    # implementation reads it, and the operation cannot be un-started. Leaking
    # it and saying so is the only honest answer, and the limit that produced
    # it is a build-time constant.
    body.append(ind + f"if (!mpiwrapper_staged_attach({name}, block))")
    body.append(ind + "  return MPIABI_ERR_INTERN;")
    body.append(ind + "return MPIABI_SUCCESS;")
    body.append("  }")
    return body


def out_handle_nulls(ep):
    """(ABI parameter, the ABI null handle) for every out handle parameter,
    across both the eleven handle classes and MPI_T's six."""
    out = []
    for p in ep.params:
        if p.cls == "handle_out":
            out.append((f"abi_{p.name}", NULL_HANDLE[HANDLE_KIND[p.kind]]))
        elif p.cls == "toolhandle_out":
            out.append((f"abi_{p.name}", TOOL_HANDLE[TOOL_HANDLE_KIND[p.kind]][2]))
    return out


def null_out_handles(ep, ind):
    """What an early return owes the caller: a null handle in every out
    parameter, so that nobody is handed an uninitialized one -- and nothing at
    all where the caller passed no pointer to write to."""
    nullable = ep.name in NULLABLE_OUT_ROUTINES
    lines = []
    for abi, null in out_handle_nulls(ep):
        stmt = f"*{abi} = {null};"
        lines.append(ind + (f"if ({abi}) {stmt}" if nullable else stmt))
    return lines


def emit_extent_queries(ep, probes, pre, rejects, ind):
    """The extent queries of an array whose length apis.json gives as `*`.

    They run before the call they serve, so that a failure returns the
    implementation's own error and nothing has been allocated yet.
    """
    if not (probes or pre or rejects):
        return []
    body = []
    for decl, call in probes:
        nulls = null_out_handles(ep, ind + "    ")
        body.append(ind + decl)
        body.append(ind + "{")
        body += assign("const int ierror", call, ind + "  ")
        if nulls:
            body.append(ind + "  if (ierror != MPI_SUCCESS) {")
            body += nulls
            body.append(ind + "    return mpiwrapper_errorcode_toabi(ierror);")
            body.append(ind + "  }")
        else:
            body.append(ind + "  if (ierror != MPI_SUCCESS)")
            body.append(ind + "    return mpiwrapper_errorcode_toabi(ierror);")
        body.append(ind + "}")
    body += [ind + line for line in pre]
    for cond, err in rejects:
        nulls = null_out_handles(ep, ind + "  ")
        if nulls:
            body.append(ind + f"if ({cond}) {{")
            body += nulls
            body.append(ind + f"  return {err};")
            body.append(ind + "}")
        else:
            body.append(ind + f"if ({cond}) return {err};")
    body.append("")
    return body


def emit_releases(releases, ind):
    """Every completion entry point releases, not just the ones an author
    happens to think of (NOTES.md #6.3).

    The discriminator is the implementation's own: a request it has set to
    MPI_REQUEST_NULL is complete and deallocated, so nothing can still be
    reading the block. A persistent request survives its completion with the
    same handle and is released at MPI_Request_free instead -- which the same
    test covers, because that is where the implementation nulls it. There is
    no separate flag: the null-out *is* the flag.
    """
    body = []
    for kind, item in releases:
        if kind == "scalar":
            name = item
            body.append(ind + f"if (mpiwrapper_staged_any() && "
                              f"{name} == MPI_REQUEST_NULL &&")
            body.append(ind + f"    {name}_before != MPI_REQUEST_NULL)")
            body.append(ind + f"  mpiwrapper_staged_release({name}_before);")
        else:
            s = item
            body.append(ind + "if (mpiwrapper_staged_any())")
            body.append(ind + f"  for ({s.extent.ctype} i = 0; "
                              f"i < {s.extent.alloc}; ++i)")
            body.append(ind + f"    if ({s.name}[i] == MPI_REQUEST_NULL) {{")
            body.append(ind + "      const MPI_Request before =")
            body.append(ind + "          mpiwrapper_request_fromabi("
                              f"abi_{s.p.name}[i]);")
            body.append(ind + "      if (before != MPI_REQUEST_NULL) "
                              "mpiwrapper_staged_release(before);")
            body.append(ind + "    }")
    return body


def emit_writeback(staged, maps, extent_post, ind):
    """The out direction of every array: the staged temporaries copied back
    and the in-place ones mapped where they lie."""
    body = [ind + line for line in extent_post]
    for s in staged:
        if s.mode == "inout":
            # Unconditionally, *including* on error: MPI_ERR_IN_STATUS means
            # the per-request error codes are the payload, and the request
            # array has been partially updated either way.
            body.append(ind + f"for ({s.extent.ctype} i = 0; "
                              f"i < {s.extent.alloc}; ++i)")
            body.append(ind + f"  abi_{s.p.name}[i] = "
                              f"mpiwrapper_request_toabi({s.name}[i]);")
        elif s.mode == "status":
            body.append(ind + f"if (!{s.skip})")
            body.append(ind + f"  for (int i = 0; i < {s.extent.conv}; ++i)")
            body.append(ind + f"    mpiwrapper_status_toabi(&{s.name}[i], "
                              f"&abi_{s.p.name}[i]);")
        elif s.mode == "out":
            fn = "mpiwrapper_" + s.family
            cond = "ierror == MPI_SUCCESS"
            if s.skip:
                cond = f"ierror == MPI_SUCCESS && !{s.skip}"
            body.append(ind + f"if ({cond})")
            body.append(ind + f"  for ({s.extent.ctype} i = 0; "
                              f"i < {s.extent.conv}; ++i)")
            body.append(ind + f"    abi_{s.p.name}[i] = {fn}_toabi({s.name}[i]);")
    for p, name, extent in maps:
        fn = "mpiwrapper_" + scalar_family(p, "array_convert_in")
        body.append(ind + "if (ierror == MPI_SUCCESS)")
        body.append(ind + f"  for ({extent.ctype} i = 0; i < {extent.conv}; ++i)")
        body.append(ind + f"    {name}[i] = {fn}_toabi({name}[i]);")
    return body


def emit_status_field(ep):
    """One of the six accessors that never reach the implementation.

    The ABI status carries its three named fields in the ABI's own encoding --
    mpiwrapper_status_toabi put them there -- so getting or setting one is a
    field access on the caller's own storage. Converting through an
    implementation status and back would be the same value by a longer route,
    and would fail over an implementation too old to have the function.
    """
    field, direction = STATUS_FIELD[ep.name]
    value = [p for p in ep.params if p.kind != "STATUS"]
    assert len(value) == 1, ep.name
    v = "abi_" + value[0].name
    body = ["  {"]
    if direction == "get":
        body.append(f"    *{v} = abi_status->{field};")
    else:
        body.append(f"    abi_status->{field} = {v};")
    body.append("    return MPIABI_SUCCESS;")
    body.append("  }")
    return body


def emit_stub(ep):
    """Decision 6's body: the slot stays present and reports at run time."""
    body = ["  {"]
    for p in ep.params:
        if p.name != "...":
            body.append(f"    (void)abi_{p.name};")
    body += null_out_handles(ep, "    ")
    if ep.ret == "int":
        body.append("    return MPIABI_ERR_UNSUPPORTED_OPERATION;")
    elif ep.ret == "double":
        body.append("    return 0.0;")
    else:
        body.append(f"    return ({abi_type(ep.ret)})0;")
    body.append("  }")
    return body


# ---------------------------------------------------------------------------
# gen/include/mpiwrapper_vtable.h
# ---------------------------------------------------------------------------

VTABLE_PREAMBLE = '''\
/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate.py. The vtable: the only thing libmpi_abi and
 * libmpiwrapper share.
 *
 * Both sides are generated from the same slot list, so they cannot disagree
 * about the layout by accident -- but they are built at different times
 * against different MPIs, so they can disagree by *version*, and
 * MPIWRAPPER_LAYOUT_HASH is what turns that into a clean failure instead of a
 * call through a shifted slot.
 */

#ifndef MPIWRAPPER_VTABLE_H
#define MPIWRAPPER_VTABLE_H

#include <stddef.h>
#include <stdint.h>

/* The MPIABI_ view of the ABI: every typedef, macro and enumerator renamed, no
 * prototypes.
 *
 * The renaming touches typedef names and macro/enumerator names only. Struct
 * *tags* are left alone, so `MPIABI_Comm` and the ABI header's own `MPI_Comm`
 * are both `struct MPI_ABI_Comm *` -- the same type, not two incompatible
 * ones. That is what lets the ABI side forward its arguments into a slot
 * without a single cast (see gen/mpi_abi/entrypoints.c). Struct *members* are
 * left alone too: MPIABI_Status has fields MPI_SOURCE, MPI_TAG, MPI_ERROR,
 * MPI_internal, exactly as the ABI header does.
 */
#include "mpiabi.h"

/* Computed by dev/layout_hash.py's definition over the slot list below:
 * comments removed, whitespace collapsed, FNV-1a/32 of the result. Any edit
 * that changes the struct changes this value, and `ctest -R layout-hash` fails
 * until a regeneration updates it.
 */
#define MPIWRAPPER_LAYOUT_HASH {hash:#010x}u

/* One slot per *forwarded* ABI entry point, so {nslots} of them: MPI_X and
 * PMPI_X get their own, and each leads to a wrapper body that calls the
 * implementation's correspondingly-shifted name. That is fewer than the 1376
 * names libmpi_abi exports, because the entry points MPI-3.0 deleted are
 * answered on the ABI side in terms of their replacements and reach no slot at
 * all (NOTES.md #3); gen/report.txt freezes both counts.
 *
 * Routing MPI_X and PMPI_X to a single slot would be cheaper, but then an
 * application calling PMPI_Send to bypass profiling would still be seen by a
 * tool interposed between the wrapper and the implementation -- it would have
 * bypassed the ABI-level profiling layer only. Keeping them distinct also
 * makes the ledger 1:1 rather than 2:1, so "each forwarded entry point has
 * exactly one slot and one body" is a uniform invariant with no special case.
 *
 * Both names are always available to link against, though not in the shape
 * NOTES.md #2 originally recorded from Linux: on macOS the conda-forge MPICH
 * 4.3.1 build keeps every PMPI_* in a separate libpmpi.dylib (libmpi.dylib has
 * a strong MPI_Send and no PMPI_ symbols at all), and Open MPI 5.0.10 has
 * MPI_Send and PMPI_Send as two distinct definitions rather than an alias
 * pair. Either way both names resolve as long as the wrapper links what mpicc
 * links, which is what src/mpiwrapper/ does.
 */
struct mpiwrapper_vtable {{
'''

VTABLE_EPILOGUE = '''\
};

/* The only symbol libmpiwrapper exports.
 *
 * Returns NULL and sets *diagnostic on any mismatch. A getter rather than an
 * exported struct, because reading a version field out of a struct means
 * trusting the layout you are trying to validate -- and because this is the
 * natural place for the wrapper to build its reverse handle map, and to check
 * its own symbol resolution, before anyone can call a slot.
 *
 * `size` is sizeof(struct mpiwrapper_vtable) as the *caller* understands it,
 * and it must match exactly. What the size catches is the one mismatch the
 * hash cannot: the hash is taken over the *text* of the slot list, so two
 * halves built for different targets (32- against 64-bit) or with
 * incompatible struct-layout settings hash identically and disagree about
 * sizeof.
 *
 * Both `abi_version` and `abi_subversion` are checked, against the header's
 * MPI_ABI_VERSION and MPI_ABI_SUBVERSION. The layout hash alone is not enough:
 * a subversion that added no slot would leave it unchanged.
 *
 * `abi_probe` is the address of any function in libmpi_abi. The wrapper
 * dladdr()s it together with the MPI_Send it actually resolved and refuses if
 * the two share a base object, which would mean the loader bound the wrapper's
 * calls back into libmpi_abi instead of out to the implementation -- infinite
 * recursion, and on ELF the default outcome unless the wrapper is loaded into
 * its own namespace or with RTLD_DEEPBIND. See the long comment in
 * src/mpi_abi/bootstrap.c.
 */
/* The wrapper is compiled -fvisibility=hidden, so this attribute is what makes
 * the single exported symbol single rather than merely intended: everything
 * else in libmpiwrapper is unexported by the language, not by a linker script
 * that has to be kept in step. test/check_exports.cmake confirms it with nm.
 */
#if defined(__GNUC__)
#  define MPIWRAPPER_EXPORT __attribute__((visibility("default")))
#else
#  define MPIWRAPPER_EXPORT
#endif

MPIWRAPPER_EXPORT const struct mpiwrapper_vtable *
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t abi_subversion,
                      uint32_t layout_hash, size_t size, const void *abi_probe,
                      const char **diagnostic);

#endif /* MPIWRAPPER_VTABLE_H */
'''


def slot_declaration(ep):
    ret = abi_type(ep.ret)
    args = [slot_type(p) for p in ep.params] or ["void"]
    return wrap(f"{ret} (*{ep.name})", args, ";", "  ",
                2 + len(ret) + 3 + len(ep.name) + 2)


def emit_vtable_h(entrypoints):
    slots = []
    nslots = 0
    for ep in entrypoints:
        if ep.status == "abi-alias":
            continue
        slots += slot_declaration(ep)
        nslots += 1
    body = "\n".join(slots) + "\n"
    # nslots is the count of *slots*, not of entry points: the abi-alias ones
    # skipped above reach no slot, so len(entrypoints) is the wrong number here
    # even though it is the right one for ENTRYPOINTS_PREAMBLE, which counts
    # exported forwarders. Getting this wrong is invisible -- it only mis-states
    # a comment sitting directly above the struct it miscounts, which is how it
    # survived from the commit that introduced the aliases until a review.
    text = (VTABLE_PREAMBLE.format(hash=0, nslots=nslots) + body +
            VTABLE_EPILOGUE)
    # The hash is over the emitted slot list itself, by dev/layout_hash.py's
    # definition, so it is computed from the text and substituted back. The
    # comment above is not part of that text, so nslots does not affect it.
    value = lh.fnv1a32(lh.slot_list_text(text).encode())
    return (VTABLE_PREAMBLE.format(hash=value, nslots=nslots) + body +
            VTABLE_EPILOGUE)


# ---------------------------------------------------------------------------
# gen/mpi_abi/entrypoints.c
# ---------------------------------------------------------------------------

ENTRYPOINTS_PREAMBLE = '''\
/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate.py. libmpi_abi -- the exported entry points, all
 * {nslots} of them.
 *
 * Note what is *not* here: no conversion, no temporary, no knowledge of any
 * implementation type, and no initialization check. The arguments pass through
 * untouched, and need no cast because the ABI header's MPI_Comm and mpiabi.h's
 * MPIABI_Comm are the same type -- both `struct MPI_ABI_Comm *`. That identity
 * is rule 2 of the renaming (NOTES.md #2) and it is what keeps {nslots}
 * forwarders cast-free; a cast here would silently absorb a genuine type
 * error.
 *
 * MPI_* and PMPI_* are two definitions rather than an alias: macOS aliases
 * need -Wl,-alias or __asm__ labels, and at one line per body an alias saves
 * nothing. They reach *different* slots, so that bypassing a profiling layer
 * at the ABI level also bypasses one interposed at the implementation level.
 */

#include <mpi.h> /* the ABI's */

#include "mpiwrapper_vtable.h"

#include <assert.h>

/* Set by the constructor in bootstrap.c before anything can reach these. */
extern const struct mpiwrapper_vtable *mpi_abi_vt;

/* Cheap insurance in development builds only: in a release build this compiles
 * to nothing and a stuck NULL would be a segfault at a small address, which is
 * what the constructor-ordering argument in bootstrap.c says cannot happen.
 * dev/dispatch-bench measures what the defensive version would cost: 23
 * instructions per entry point instead of 4.
 */
#ifndef NDEBUG
#  define VT() (assert(mpi_abi_vt != NULL), mpi_abi_vt)
#else
#  define VT() mpi_abi_vt
#endif
'''


# The two ABI enum *tags* the renaming does touch, and the callback typedefs
# that embed them. Rule 2 of the renaming leaves struct and enum tags alone
# precisely so that the ABI header's MPI_Comm and mpiabi.h's MPIABI_Comm are
# one type and 1376 forwarders need no cast -- but MPI_T_cb_safety and
# MPI_T_source_order spell their tag exactly like their typedef, and a
# conforming implementation's own <mpi.h> declares the same tag with the same
# enumerators, so leaving them alone would redeclare them in libmpiwrapper's
# translation units (NOTES.md #2). Renaming them is right and makes the two
# spellings genuinely distinct types, which is why these -- and only these --
# forwarders carry a cast.
TAG_RENAMED_TYPES = (
    "MPI_T_cb_safety", "MPI_T_source_order", "MPI_T_event_cb_function",
    "MPI_T_event_free_cb_function", "MPI_T_event_dropped_cb_function",
)


# The three of those that are function typedefs: `typedef void (NAME)(...)`,
# so a parameter of that type is a function type that decays to a pointer, and
# the cast has to name the pointer.
TAG_RENAMED_FUNCTION_TYPES = (
    "MPI_T_event_cb_function", "MPI_T_event_free_cb_function",
    "MPI_T_event_dropped_cb_function",
)


def forwarder_argument(p):
    """What the ABI-side forwarder passes into the slot."""
    if not any(t in p.base for t in TAG_RENAMED_TYPES):
        return p.name
    cast = abi_type(p.base).rstrip()
    if any(t in p.base for t in TAG_RENAMED_FUNCTION_TYPES):
        cast += " *"
    return f"({cast})" + p.name


def emit_entrypoints_c(entrypoints):
    out = [ENTRYPOINTS_PREAMBLE.format(nslots=len(entrypoints))]
    for ep in entrypoints:
        named = [p for p in ep.params if p.name != "..."]
        args = [forwarder_argument(p) for p in named]
        decls = [declare(p.base, p.name, p.suffix) for p in named] or ["void"]
        if len(named) != len(ep.params):
            decls.append("...")
        # An entry point MPI-3.0 deleted reaches the slot of the one that
        # replaced it, with the prefix preserved: MPI_Attr_get calls the
        # implementation's MPI_Comm_get_attr and PMPI_Attr_get calls its
        # PMPI_Comm_get_attr, so the shifted-name rule that keeps a profiling
        # layer honest survives the rename (NOTES.md #2).
        slot = ep.name
        if ep.status == "abi-alias":
            slot = ("P" if ep.name.startswith("PMPI_") else "") + ep.detail
        head = wrap(f"{ep.ret} {ep.name}", decls, "", "",
                    len(ep.ret) + 1 + len(ep.name) + 1)
        call = wrap(f"return VT()->{slot}", args, ";", "  ",
                    2 + len("return VT()->") + len(slot) + 1)
        if len(named) != len(ep.params):
            # MPI_Pcontrol is the only variadic entry point, and C gives a
            # forwarder no way to pass `...` along. Dropping the extra
            # arguments is what MPI-5.0 14.2 permits a profiling layer to do --
            # their meaning is implementation-defined and an implementation may
            # ignore them -- and it is what the hand-written body on the other
            # side of the slot does with them too.
            out.append("/* The extra arguments stop here: C cannot forward\n"
                       " * `...`, and MPI-5.0 14.2 lets a profiling layer\n"
                       " * ignore them.\n"
                       " */")
        one = head[0] + " { " + call[0].strip() + " }"
        if len(head) == 1 and len(call) == 1 and len(one) <= 79:
            out.append(one + "\n")
        else:
            out.append("\n".join(head) + "\n{\n" + "\n".join(call) + "\n}\n")
    return "\n".join(out)


# ---------------------------------------------------------------------------
# gen/mpiwrapper/wrappers.c
# ---------------------------------------------------------------------------

WRAPPERS_PREAMBLE = '''\
/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate.py. libmpiwrapper -- the wrapper bodies, and the
 * vtable they fill. A bug here goes back into the generator (NOTES.md #3).
 *
 * The shape is S1's, which is what the generator was designed against: one
 * `const` local per parameter, in parameter order, named after the parameter
 * with the `abi_` prefix dropped, and one body macro instantiated twice --
 * once calling the implementation's MPI_ name, once its PMPI_ name.
 *
 * That convention is load-bearing rather than cosmetic. The generator asserts
 * over its own emitted text that **no parameter of an ABI-typed signature
 * appears in the argument list of the implementation call** -- only locally
 * declared converted values may. With this convention that assertion is a
 * grep, and a missing conversion is a hard stop at generation time rather than
 * a wrong answer at 4096 ranks.
 *
 * Every body is guarded by MPIWRAPPER_HAVE_<name>, which dev/probe_impl.py
 * writes into mpiwrapper_impl_config.h at configure time by asking the
 * compiler about the implementation's own header. An entry point the implementation
 * does not have gets the stub of decision 6 instead: the slot stays present
 * and reports MPIABI_ERR_UNSUPPORTED_OPERATION at run time, so the ABI surface
 * never shrinks. The hand-written entry points that have no body yet get the
 * same stub, and gen/report.txt names every one of them.
 *
 * Six bodies carry no guard: the status accessors of NOTES.md #5.2 read and
 * write a named field of the caller's own ABI status and never reach the
 * implementation, so a stub there would replace a working answer with an
 * error.
 *
 * The bodies are static: only mpiwrapper_get_vtable is exported, and static
 * enforces that in the language rather than relying on the linker script.
 */

#include "handwritten.h"
#include "internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

'''

WRAPPERS_VTABLE_HEAD = '''
/* Designated initializers, so a slot the generator forgets is a NULL pointer
 * rather than a shifted one -- and mpiwrapper_get_vtable asserts none is NULL
 * before handing the table out.
 */
const struct mpiwrapper_vtable mpiwrapper_vtable_instance = {
'''



def block_comment(text, width=76):
    """A wrapped C block comment, so a long ledger reason stays inside 80."""
    lines = textwrap.wrap(text, width - 3)
    if len(lines) == 1:
        return [f"/* {lines[0]} */"]
    return [f"/* {lines[0]}"] + [f" * {ln}" for ln in lines[1:]] + [" */"]


def banner(name):
    dashes = max(4, 75 - len(name) - len("/* ") - 1)
    return f"/* {'-' * dashes} {name} */"


def emit_wrappers_c(pairs, handwritten_bodies):
    """`pairs` is [(MPI_X, PMPI_X)] in header order."""
    out = [WRAPPERS_PREAMBLE]
    initializers = []

    for ep, pep in pairs:
        if ep.status == "abi-alias":
            continue
        if ep.name in handwritten_bodies:
            for e in (ep, pep):
                initializers.append((e.name, f"mpiwrapper_w_{e.name}"))
            continue

        chunk = [banner(ep.name), ""]
        head = f"#define BODY_{ep.name}(TARGET)"
        stub = macro_lines(head, emit_stub(ep))
        if ep.status == "generated" and ep.unguarded:
            # A body that never reaches the implementation works over one that
            # does not have the entry point at all, so decision 6's stub would
            # be a regression rather than a fallback (NOTES.md #5.2).
            chunk += block_comment(
                "Pure ABI-side: this reads or writes a named field of the "
                "caller's own status, which already holds the ABI's encoding. "
                "The implementation is not involved, so there is no "
                "MPIWRAPPER_HAVE_ guard and no stub -- it answers correctly "
                "over an implementation too old to have the function.")
            chunk += macro_lines(head, emit_body(ep))
        elif ep.status == "generated":
            body = macro_lines(head, emit_body(ep))
            # Decision 6: an entry point the implementation does not have keeps
            # its slot and reports at run time. The probe is what decides, not
            # a version test -- Open MPI 5.0.10 reports MPI-3.1 and has
            # sessions, so MPI_VERSION is a proxy that would stub what is
            # there.
            chunk.append(f"#ifdef MPIWRAPPER_HAVE_{ep.name}")
            chunk += body
            chunk.append("#else")
            chunk += stub
            chunk.append("#endif")
        else:
            chunk += block_comment(ep.detail or "")
            chunk += stub
        chunk.append("")

        # A deprecated entry point still needs a wrapper, and both
        # implementations attach a deprecation attribute to their own
        # declaration of it -- which -Werror turns into a build failure for
        # code whose whole job is to call it.
        if ep.deprecated:
            chunk.insert(1, '#pragma GCC diagnostic push')
            chunk.insert(2, '#pragma GCC diagnostic ignored '
                            '"-Wdeprecated-declarations"')

        for e in (ep, pep):
            head = signature("static " + abi_type(e.ret), "w_" + e.name,
                             e.params)
            tail = f"BODY_{ep.name}({e.name})"
            if len(head[-1]) + 1 + len(tail) <= 79:
                head[-1] += " " + tail
            else:
                head.append("    " + tail)
            chunk += head
            initializers.append((e.name, f"w_{e.name}"))
        if ep.deprecated:
            chunk.append("#pragma GCC diagnostic pop")
        out.append("\n".join(chunk) + "\n")

    width = max(len(n) for n, _ in initializers)
    rows = [f"    .{n.ljust(width)} = {v}," for n, v in initializers]
    out.append(WRAPPERS_VTABLE_HEAD + "\n".join(rows) + "\n};\n")
    return "\n".join(out)


# ---------------------------------------------------------------------------
# gen/mpiwrapper/constants.c
# ---------------------------------------------------------------------------

CONSTANTS_PREAMBLE = '''\
/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate.py. libmpiwrapper -- the ABI <-> implementation
 * constant tables, from the ABI header's own definitions.
 *
 * Two rules govern every table here (NOTES.md #3):
 *
 *  - Case labels over handles are *numeric*, with the ABI's symbolic name in a
 *    comment. MPIABI_INT expands to ((MPIABI_Datatype)0x00000209), and casting
 *    an integer constant to a pointer type and back is not an integer constant
 *    expression in standard C -- gcc and clang accept it, but a case label is
 *    where that extension is not worth relying on. The value came from parsing
 *    the header, so nothing is transcribed either way. The integer families
 *    further down are plain enumerators and are switched by name.
 *
 *  - Every implementation-side value is named by the implementation's own
 *    macro or enumerator, never written out. A name that does not exist is
 *    then a compile error rather than a wrong number -- which is why only the
 *    entries the standard makes optional are guarded at all: the sized Fortran
 *    types, the predefined handles and enumerators newer than the MPI-3.0
 *    floor, and MPI_T.
 *
 *  - Those guards test MPIWRAPPER_HAVE_<name>, from dev/probe_impl.py, and
 *    never `#ifdef <the implementation's own name>`. `#ifdef` sees macros and
 *    not enumerators, and implementations use both: MPICH spells MPI_COMBINER_*
 *    and MPI_CART as enumerators, Open MPI spells MPI_THREAD_SINGLE,
 *    MPI_COMM_TYPE_SHARED and MPI_IDENT that way. An `#ifdef` on one of those
 *    answers *no* for a constant that is right there, and the case then drops
 *    out of the switch, reaches the default arm and passes an unmapped value
 *    through -- silently, which is the one failure mode these tables exist to
 *    prevent. Measured, not hypothetical: MPICH 4.3.1 has
 *    MPI_COMBINER_VALUE_INDEX as `= 20` in an enum, and an S2 draft that used
 *    `#ifdef` on it stopped translating that combiner without failing
 *    anything. The probe asks the compiler, which sees both, and asks about
 *    every name in one translation unit rather than one test per constant.
 *
 * The fromabi default deserves a word. A value inside the ABI's predefined
 * range that reached the default arm is a predefined handle *this*
 * implementation does not provide, and bit-casting it would hand the
 * implementation a fabricated handle -- on MPICH an int whose kind bits it
 * will happily interpret. Returning the class's null handle instead makes the
 * implementation reject the call with its own error, which is the honest
 * outcome. Outside the range it is a handle we produced earlier, and the
 * bit-cast is the identity the toabi direction promised.
 */

#include "internal.h"

#include <string.h>

'''

PREDEF_MACRO = '''
/* One filler per class. Written into caller storage at run time rather than
 * declared as static data, because Open MPI's predefined handles are
 * addresses and a pointer-to-integer cast is not a constant expression: a
 * `static const uint64_t t[] = {(uint64_t)(uintptr_t)MPI_INT}` compiles only
 * against an integer-handle MPI, which is how it slips through review.
 */
#define PREDEF(abi_macro, abi_value, impl_macro)                              \\
  do {                                                                        \\
    if (n < max) {                                                            \\
      out[n].abi  = (abi_value);                                              \\
      out[n].impl = MPIWRAPPER_BITS(impl_macro);                              \\
      out[n].name = #abi_macro;                                               \\
    }                                                                         \\
    ++n;                                                                      \\
  } while (0)
'''


def emit_constants_c(classes, handles, enums):
    out = [CONSTANTS_PREAMBLE]

    def class_guard(cls):
        # MPI_Session is MPI-4.0 and an implementation may simply not have the
        # *type*; nothing can test for a type, so the test is the class's null
        # handle, exactly as internal.h does it.
        return guard("MPI_SESSION_NULL") if cls == "Session" else None

    # --- ABI -> implementation ---------------------------------------------
    for cls in classes:
        low = cls.lower()
        cguard = class_guard(cls)
        rows = []
        if cguard:
            rows.append(f"#ifdef {cguard}")
        rows.append(f"MPI_{cls} mpiwrapper_{low}_fromabi(MPIABI_{cls} abi)")
        rows.append("{")
        rows.append("  switch ((uint64_t)(uintptr_t)abi) {")
        null_macro = handles[cls][0][0]
        for macro, value in handles[cls]:
            g = guard(macro)
            if g == cguard:
                g = None  # the whole class already carries this guard
            if g:
                rows.append(f"#ifdef {g}")
            rows.append(f"  case {value:#010x}: return {macro};"
                        f" /* {gh.rename(macro)} */")
            if g:
                rows.append("#endif")
        rows.append("  default: break;")
        rows.append("  }")
        rows.append(f"  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi)))"
                    f" return {null_macro};")
        rows.append(f"  return MPIWRAPPER_HANDLE(MPI_{cls}, abi);")
        rows.append("}")
        if cguard:
            rows.append("#endif")
        out.append("\n".join(rows) + "\n")

    # --- the predefined-handle tables --------------------------------------
    out.append(PREDEF_MACRO)
    for cls in classes:
        low = cls.lower()
        cguard = class_guard(cls)
        rows = []
        if cguard:
            rows.append(f"#ifdef {cguard}")
        rows.append(f"static size_t predef_{low}(struct mpiwrapper_predef *out,"
                    f" size_t max)")
        rows.append("{")
        rows.append("  size_t n = 0;")
        for macro, value in handles[cls]:
            g = guard(macro)
            if g == cguard:
                g = None  # the whole class already carries this guard
            if g:
                rows.append(f"#ifdef {g}")
            rows.append(f"  PREDEF({gh.rename(macro)}, {value:#010x}, {macro});")
            if g:
                rows.append("#endif")
        rows.append("  return n;")
        rows.append("}")
        if cguard:
            rows.append("#endif")
        out.append("\n".join(rows) + "\n")

    # --- the selftest shims ------------------------------------------------
    out.append("""/* Thin shims so that test/mpiwrapper_selftest.c can walk every class through
 * one loop instead of eleven copies of the same five assertions.
 */
""")
    for cls in classes:
        low = cls.lower()
        cguard = class_guard(cls)
        rows = []
        if cguard:
            rows.append(f"#ifdef {cguard}")
        rows += [
            f"static uint64_t toabi_bits_{low}(uint64_t impl_bits)",
            "{",
            f"  return MPIWRAPPER_BITS(mpiwrapper_{low}_toabi("
            f"MPIWRAPPER_HANDLE(MPI_{cls}, impl_bits)));",
            "}",
            "",
            f"static uint64_t fromabi_bits_{low}(uint64_t abi_bits)",
            "{",
            f"  return MPIWRAPPER_BITS(mpiwrapper_{low}_fromabi("
            f"MPIWRAPPER_HANDLE(MPIABI_{cls}, abi_bits)));",
            "}",
        ]
        if cguard:
            rows.append("#endif")
        out.append("\n".join(rows) + "\n")

    # --- the reverse maps ---------------------------------------------------
    out.append("""/* The reverse maps, built once inside mpiwrapper_get_vtable before any slot
 * can be called. Failure here is fatal and reported through the diagnostic:
 * falling back to a probe loop would reintroduce the only data-dependent
 * branch the perfect hash exists to remove (NOTES.md #5.1).
 *
 * Each table is sized at eight times its key count, rounded up to a power of
 * two and never below eight, which is the headroom the perfect-hash search
 * needs to terminate quickly.
 */""")
    rows, inits, entries = [], [], []
    for cls in classes:
        low, cguard = cls.lower(), class_guard(cls)
        nslots = max(8, 1 << (8 * len(handles[cls]) - 1).bit_length())
        if cguard:
            rows.append(f"#ifdef {cguard}")
            inits.append(f"#ifdef {cguard}")
            entries.append(f"#ifdef {cguard}")
        rows.append(f"static struct mpiwrapper_rmap_entry "
                    f"rmap_slots_{low}[{nslots}];")
        inits.append(f"struct mpiwrapper_rmap mpiwrapper_rmap_{low} = "
                     f"{{rmap_slots_{low}, {nslots}, 0, 0, 0}};")
        entries.append(f'    {{"{low}", predef_{low}, toabi_bits_{low}, '
                       f"fromabi_bits_{low},")
        entries.append(f"     &mpiwrapper_rmap_{low}}},")
        if cguard:
            rows.append("#endif")
            inits.append("#endif")
            entries.append("#endif")
    out.append("\n".join(rows) + "\n")
    out.append("\n".join(inits) + "\n")
    out.append("/* NULL-terminated; walked by the selftest. */\n"
               "const struct mpiwrapper_predef_class "
               "mpiwrapper_predef_classes[] = {\n" +
               "\n".join(entries) + "\n    {NULL, NULL, NULL, NULL, NULL}};\n")
    out.append(INIT_REVERSE_MAPS)

    # --- error codes --------------------------------------------------------
    classes_err = [(n, v, note) for n, (v, note) in enums.items()
                   if (n.startswith("MPI_ERR_") or n.startswith("MPI_T_ERR_"))
                   and n != "MPI_ERR_LASTCODE"]
    out.append(ERRORCODE_COMMENT)
    for direction in ("toabi", "fromabi"):
        rows = []
        if direction == "toabi":
            rows.append("int mpiwrapper_errorcode_toabi(int ierror)")
            rows.append("{")
            rows.append("  if (ierror == MPI_SUCCESS) return MPIABI_SUCCESS;")
            rows.append("  switch (ierror) {")
        else:
            rows.append("int mpiwrapper_errorcode_fromabi(int abi_ierror)")
            rows.append("{")
            rows.append("  if (abi_ierror == MPIABI_SUCCESS) return MPI_SUCCESS;")
            rows.append("  switch (abi_ierror) {")
        for name, value, note in classes_err:
            # MPI_T is optional in full, and the ABI's own `added:` note is
            # what says an error class postdates the floor.
            g = ("MPIWRAPPER_HAVE_" + name
                 if name.startswith("MPI_T_") or added_after_floor(note)
                 else None)
            if g:
                rows.append(f"#ifdef {g}")
            src, dst = ((name, gh.rename(name)) if direction == "toabi"
                        else (gh.rename(name), name))
            rows.append(f"  case {src}: return {dst};")
            if g:
                rows.append("#endif")
        # Not "pass the value through" and not MPI_ERR_OTHER either, for the
        # same reason as the keyval family below: the half of this family that
        # is handed out at run time cannot be a case label, and the two sides'
        # MPI_ERR_LASTCODE differ by five orders of magnitude. The registry in
        # src/mpiwrapper/errorcodes.c answers, and answers *_ERR_OTHER for a
        # code it never issued -- which is what this arm said before S4b.
        rows.append(
            "  default: return "
            + ("mpiwrapper_errorcode_dynamic_toabi(ierror);"
               if direction == "toabi"
               else "mpiwrapper_errorcode_dynamic_fromabi(abi_ierror);"))
        rows.append("  }")
        rows.append("}")
        out.append("\n".join(rows) + "\n")

    # --- ranks and tags -----------------------------------------------------
    out.append(RANKTAG_COMMENT)
    out.append(emit_switch_family("rank", "rank", RANK_MEMBERS, guard_all=False))
    out.append(emit_switch_family("tag", "tag", TAG_MEMBERS, guard_all=False))

    # --- the mode bitmasks --------------------------------------------------
    out.append(BITMASK_COMMENT)
    out.append(emit_bitmask("filemode", "mode", FILEMODE_MEMBERS))
    out.append(emit_bitmask("winassert", "mode", WINASSERT_MEMBERS))

    # --- the remaining integer families -------------------------------------
    out.append(INTFAMILY_COMMENT)
    deprecated = {n for n, (_, note) in enums.items() if "deprecated:" in note}
    for family in sorted(set(SWITCH_KIND.values())
                         | set(ATTRIBUTE_VALUE_FAMILIES)):
        members = [n for n in enums
                   if re.match(SWITCH_FAMILY_MEMBERS[family], n)]
        if not members:
            raise SystemExit(f"no enumerators matched the {family} family")
        out.append(emit_switch_family(family, family, members, guard_all=True,
                                      deprecated=deprecated))

    out.append(SENTINEL_BODIES)
    out.append(emit_tool_handles())
    out.append(emit_tool_obj())
    return "\n".join(out)


TOOL_HANDLE_COMMENT = '''
/* MPI_T's six handle classes.
 *
 * Not the eleven of NOTES.md #5.1 and deliberately not their machinery: those
 * have up to 103 predefined values apiece, spelled in the implementation as
 * addresses that are not compile-time constants, which is what the perfect-hash
 * reverse map exists for. These have at most two apiece, so each direction is
 * one or two compares -- #5.3's sentinel shape rather than #5.1's map shape.
 *
 * The sentinels genuinely differ, which is why this is not a bit-cast: the ABI
 * fixes MPI_T_PVAR_ALL_HANDLES at 1, Open MPI 5.0.6 spells it -1, and MPICH
 * 4.3.1 makes it an `extern ... * const` whose value is not a constant
 * expression at all -- so it could not be a case label even if one were wanted,
 * and both directions have to be run-time compares.
 *
 * A dynamic implementation handle whose bits land on 0 or 1 would be read back
 * as a sentinel, so the toabi direction rejects it exactly as #5.1 does: the
 * class's null handle plus the flag mpiwrapper_take_handle_error() reports. No
 * implementation can produce one -- these are object addresses on both -- and
 * the check is one compare on a path that allocates a tool handle.
 */
'''


def emit_tool_handles():
    rows = [TOOL_HANDLE_COMMENT]
    for fam, (impl, sentinels, abinull) in TOOL_HANDLE.items():
        abi = gh.rename(impl)
        g = "MPIWRAPPER_HAVE_" + impl
        body = [f"#ifdef {g}",
                f"{impl} mpiwrapper_{fam}_fromabi({abi} abi)",
                "{"]
        for s in sentinels:
            body.append(f"  if (abi == {gh.rename(s)}) return {s};")
        body += [f"  return MPIWRAPPER_HANDLE({impl}, MPIWRAPPER_BITS(abi));",
                 "}",
                 "",
                 f"{abi} mpiwrapper_{fam}_toabi({impl} h)",
                 "{"]
        for s in sentinels:
            body.append(f"  if (h == {s}) return {gh.rename(s)};")
        if sentinels:
            # Only a class with a sentinel has a value to collide with; the
            # two event classes name none, so every bit pattern is its own.
            body += ["  if (MPIWRAPPER_BITS(h) <= MPIWRAPPER_TOOL_PREDEF_LAST) {",
                     "    mpiwrapper_set_handle_error();",
                     f"    return {abinull};",
                     "  }"]
        body += [f"  return MPIWRAPPER_HANDLE({abi}, MPIWRAPPER_BITS(h));",
                 "}",
                 "#endif"]
        rows.append("\n".join(body) + "\n")
    return "\n".join(rows)


TOOL_OBJ_COMMENT = '''
/* MPI_T's obj_handle: the one parameter whose *class* is not in the argument
 * list. MPI-5.0 15.3.6 makes it "an address to a local variable that stores the
 * object's handle", and which kind of handle that is comes from the `bind` a
 * prior get_info reported -- so src/mpiwrapper/toolobj.c asks the
 * implementation for `bind` first and this switch converts accordingly.
 *
 * `bind` here is the *implementation's* own value, straight from its own
 * get_info, so the labels are its names and no conversion intervenes. A null
 * obj_handle and MPI_T_BIND_NO_OBJECT both mean the same thing -- the argument
 * is ignored -- and both produce a null pointer, which is what the
 * implementation is then given.
 */
'''


def emit_tool_obj():
    rows = [TOOL_OBJ_COMMENT,
            "void *mpiwrapper_tool_obj_fromabi(int bind, void *abi_obj,",
            "                                  union mpiwrapper_tool_obj *out)",
            "{",
            "  if (!abi_obj) return NULL;",
            "  switch (bind) {"]
    for member, cls in TOOL_OBJ_CLASSES.items():
        abitype = {"comm": "MPIABI_Comm", "datatype": "MPIABI_Datatype",
                   "errhandler": "MPIABI_Errhandler", "file": "MPIABI_File",
                   "group": "MPIABI_Group", "info": "MPIABI_Info",
                   "message": "MPIABI_Message", "op": "MPIABI_Op",
                   "request": "MPIABI_Request", "session": "MPIABI_Session",
                   "win": "MPIABI_Win"}[cls]
        # Two independent reasons a case can be absent: MPI_T may not name the
        # binding, and the class itself may not exist (sessions are MPI-4.0).
        guards = ["MPIWRAPPER_HAVE_" + member]
        if cls == "session":
            guards.append("MPIWRAPPER_HAVE_MPI_SESSION_NULL")
        rows.append("#if " + " && ".join(f"defined({g})" for g in guards))
        rows.append(f"  case {member}:")
        rows.append(f"    out->{cls} = mpiwrapper_{cls}_fromabi("
                    f"*({abitype} *)abi_obj);")
        rows.append(f"    return &out->{cls};")
        rows.append("#endif")
    rows += ["  default: break;",
             "  }",
             "  return NULL;",
             "}"]
    return "\n".join(rows) + "\n"


def emit_switch_family(family, argname, members, guard_all, deprecated=()):
    """`mpiwrapper_<family>_fromabi`/`_toabi`, a switch each way.

    A family with a deprecated member carries the same pragma pair the
    deprecated *entry points* do. The ABI header marks MPI_HOST as deprecated
    in MPI-4.1 and Open MPI main attaches an attribute to its own declaration
    of it, which -Werror turns into a build failure for a table whose whole job
    is to name it. Driven by the header's own `deprecated:` note rather than by
    a list here, so the next one costs nothing.
    """
    rows = []
    stale = [m for m in members if m in deprecated]
    if stale:
        rows.append("/* " + ", ".join(stale) + ": deprecated, and an "
                    "implementation may say so on its own")
        rows.append(" * declaration. A conversion table still has to name it.")
        rows.append(" */")
        rows.append("#pragma GCC diagnostic push")
        rows.append('#pragma GCC diagnostic ignored "-Wdeprecated-declarations"')
    for i, direction in enumerate(("fromabi", "toabi")):
        arg = ("abi_" + argname) if direction == "fromabi" else argname
        rows.append(f"int mpiwrapper_{family}_{direction}(int {arg})")
        rows.append("{")
        rows.append(f"  switch ({arg}) {{")
        labels = [(gh.rename(m), m) if direction == "fromabi" else (m, gh.rename(m))
                  for m in members]
        width = max(len(src) for src, _ in labels)
        for (src, dst), m in zip(labels, members):
            g = guard(m) if guard_all else None
            if g:
                rows.append(f"#ifdef {g} /* {guard_reason(m)} */")
            rows.append(f"  case {(src + ':').ljust(width + 1)} return {dst};")
            if g:
                rows.append("#endif")
        fallback = SWITCH_DEFAULT.get(family, (arg, arg))[i]
        rows.append(f"  default:{' ' * (width - 7)} return {fallback};")
        rows.append("  }")
        rows.append("}")
        rows.append("")
    if stale:
        rows.append("#pragma GCC diagnostic pop")
        rows.append("")
    return "\n".join(rows)


def emit_bitmask(family, argname, members):
    rows = []
    for direction in ("fromabi", "toabi"):
        if direction == "fromabi":
            arg, res = "abi_" + argname, argname
        else:
            arg, res = argname, "abi_" + argname
        rows.append(f"int mpiwrapper_{family}_{direction}(int {arg})")
        rows.append("{")
        rows.append(f"  int {res} = 0;")
        for m in members:
            src, dst = ((gh.rename(m), m) if direction == "fromabi"
                        else (m, gh.rename(m)))
            rows.append(f"  if ({arg} & {src}) {res} |= {dst};")
        rows.append(f"  return {res};")
        rows.append("}")
        rows.append("")
    return "\n".join(rows)


INIT_REVERSE_MAPS = '''
int mpiwrapper_init_reverse_maps(const char **diagnostic)
{
  struct mpiwrapper_predef predef[128];
  uint64_t                 keys[128], abis[128];

  for (const struct mpiwrapper_predef_class *c = mpiwrapper_predef_classes;
       c->name; ++c) {
    const size_t n = c->fill(predef, sizeof predef / sizeof *predef);
    if (n > sizeof predef / sizeof *predef) {
      *diagnostic = "predefined-handle table larger than the build-time bound";
      return 0;
    }
    for (size_t i = 0; i < n; ++i) {
      keys[i] = predef[i].impl;
      abis[i] = predef[i].abi;
    }
    if (!mpiwrapper_rmap_build(c->map, keys, abis, n)) {
      *diagnostic = "could not construct a collision-free predefined-handle map";
      return 0;
    }
  }
  return 1;
}
'''

ERRORCODE_COMMENT = '''
/* Error codes. The common case is MPI_SUCCESS, which is 0 everywhere, so it
 * costs one compare. Codes handed out at run time by MPI_Add_error_class/
 * _code need renumbering rather than passing through (the ABI caps
 * MPI_ERR_LASTCODE at 16383 against MPICH's 0x3fffffff), so the default arm
 * hands off to the registry in src/mpiwrapper/errorcodes.c, whose only writers
 * are those two entry points. A code that registry never issued still answers
 * *_ERR_OTHER, which is a legal class for an error this ABI cannot name.
 */'''

RANKTAG_COMMENT = '''
/* Ranks and tags are separate classes, and this is the reason: in the ABI,
 * MPI_ANY_TAG is -2 and MPI_PROC_NULL is -3; in MPICH both MPI_ANY_TAG and
 * MPI_PROC_NULL are -1. An int argument cannot be translated without knowing
 * its role, which is why the generator needs each parameter's kind from
 * apis.json and the header alone is insufficient (NOTES.md #5.4).
 *
 * A switch is fine *within* a role: the implementation's magic values are
 * distinct there (MPICH ranks are -1, -2, -3, -32766), so the case labels are
 * unique. It is only a combined rank-and-tag conversion that could not be
 * written as one.
 */'''

BITMASK_COMMENT = '''
/* The mode bitmasks. MPI_MODE_* are OR-combined and the bit assignments are
 * unrelated to the ABI's: the ABI's RDONLY is 16 where both implementations
 * use 2, so this is a decomposition rather than a switch (NOTES.md #5.5).
 *
 * There are *two* mappers rather than the one NOTES.md #5.5 first described,
 * and Open MPI is why. The ABI puts file modes (1..256) and window asserts
 * (1024..16384) in one enum with disjoint bits, so on the way in one mapper
 * would do. On the way out it would not: Open MPI numbers its window asserts
 * MPI_MODE_NOCHECK=1, NOPRECEDE=2, NOPUT=4, NOSTORE=8, NOSUCCEED=16 -- exactly
 * the bits it also uses for MPI_MODE_CREATE, RDONLY, WRONLY, RDWR and
 * DELETE_ON_CLOSE. An implementation amode of 0x1 is CREATE or NOCHECK
 * depending on which parameter it came from, and nothing in the value says
 * which. (MPICH keeps the two families disjoint, so a single mapper
 * round-trips there and the bug would have shipped.) This is the same shape as
 * ranks and tags: the role is a property of the parameter, so it belongs in
 * the function name, and apis.json is what tells the generator which to emit.
 */'''

INTFAMILY_COMMENT = '''
/* The remaining mapped integer constants, one family per apis.json kind.
 *
 * These are ordinary ints on both sides, so the case labels are symbolic; the
 * numeric-label rule applies to handles, where the ABI's spelling is a cast to
 * a pointer type.
 *
 * Only the members a conforming implementation may genuinely lack are guarded.
 * Everything else here is MPI-3.0 or older, so an implementation that really
 * lacks one fails the build naming it -- which is where these families would
 * otherwise be at their most dangerous, since a dropped case reaches the
 * default arm and passes an unmapped value through.
 */'''

SENTINEL_BODIES = '''
/* Sentinels. MPI_BOTTOM is (void *)0 in the ABI and in both implementations,
 * so that arm is an identity today -- it is emitted anyway, because the ABI
 * fixes the value and an implementation is free not to, and a site that omits
 * the test is invisible until that implementation appears. MPI_IN_PLACE is not
 * identity: MPICH's is (void *)-1.
 */

const void *mpiwrapper_sendbuf_fromabi(const void *abi_buf)
{
  if (abi_buf == MPIABI_BOTTOM) return MPI_BOTTOM;
  return abi_buf;
}

const void *mpiwrapper_sendbuf_inplace_fromabi(const void *abi_buf)
{
  if (abi_buf == MPIABI_BOTTOM) return MPI_BOTTOM;
  if (abi_buf == MPIABI_IN_PLACE) return MPI_IN_PLACE;
  return abi_buf;
}

void *mpiwrapper_recvbuf_fromabi(void *abi_buf)
{
  if (abi_buf == MPIABI_BOTTOM) return MPI_BOTTOM;
  return abi_buf;
}

/* The in-place receive buffer is MPI_Scatter's and MPI_Scatterv's, where the
 * root passes MPI_IN_PLACE for what it would otherwise receive into. S1 had
 * only three of these because its prototype had no such call.
 */
void *mpiwrapper_recvbuf_inplace_fromabi(void *abi_buf)
{
  if (abi_buf == MPIABI_BOTTOM) return MPI_BOTTOM;
  if (abi_buf == MPIABI_IN_PLACE) return MPI_IN_PLACE;
  return abi_buf;
}

/* The graph-topology weights. The ABI fixes MPI_UNWEIGHTED at (int *)10 and
 * MPI_WEIGHTS_EMPTY at (int *)11; both implementations spell them as objects
 * whose address is not a build-time constant, which is why this is a run-time
 * test like every other sentinel and not a table. The weights themselves are
 * plain ints and cross unconverted -- only the two pointers mean anything.
 *
 * The out form is MPI_Dist_graph_neighbors', where the caller passes
 * MPI_UNWEIGHTED to say it does not want the weights back.
 */
/* The one sentinel that is an integer rather than a pointer (S7).
 *
 * MPI_File_set_view's `disp` is a byte displacement -- an open numeric domain
 * that crosses unconverted -- with one distinguished value in it. The ABI
 * fixes MPI_DISPLACEMENT_CURRENT at (MPI_Offset)-1; ROMIO, which is the MPI-IO
 * of both implementations here, spells it -54278278. So this is the same
 * one-compare shape as the pointer sentinels above and not the switch shape of
 * a mapped family: nothing else about a displacement means anything to us, and
 * MPI_File_get_view's outgoing `disp` is a real displacement rather than this
 * value, so there is no reverse direction to emit.
 *
 * Guarded because the constant is MPI-IO's: an implementation built without it
 * has neither this name nor MPI_File_set_view, and dev/probe_impl.py is what
 * decides that per build rather than an #ifdef on the implementation's own
 * spelling (decision 6).
 */
MPI_Offset mpiwrapper_displacement_fromabi(MPIABI_Offset abi_disp)
{
#ifdef MPIWRAPPER_HAVE_MPI_DISPLACEMENT_CURRENT
  if (abi_disp == MPIABI_DISPLACEMENT_CURRENT) return MPI_DISPLACEMENT_CURRENT;
#endif
  return abi_disp;
}

const int *mpiwrapper_weights_fromabi(const int *abi_weights)
{
  if (abi_weights == MPIABI_UNWEIGHTED) return MPI_UNWEIGHTED;
  if (abi_weights == MPIABI_WEIGHTS_EMPTY) return MPI_WEIGHTS_EMPTY;
  return abi_weights;
}

int *mpiwrapper_weights_out_fromabi(int *abi_weights)
{
  if (abi_weights == MPIABI_UNWEIGHTED) return MPI_UNWEIGHTED;
  if (abi_weights == MPIABI_WEIGHTS_EMPTY) return MPI_WEIGHTS_EMPTY;
  return abi_weights;
}
'''


# ---------------------------------------------------------------------------
# Loading and joining the two inputs
# ---------------------------------------------------------------------------

def load(mpi_h_text):
    """The 1376 prototypes, joined with apis.json and classified."""
    protos = parse_prototypes(mpi_h_text)
    apis = json.load(open(APIS_JSON))

    for name, ep in protos.items():
        base = ep.name if not name.startswith("PMPI_") else "MPI_" + ep.base
        key = base.lower()
        large = False
        if key not in apis:
            if not key.endswith("_c") or key[:-2] not in apis:
                raise SystemExit(f"{base}: no apis.json entry")
            key, large = key[:-2], True
        fn = apis[key]
        # The Fortran-only ierror is not a C parameter, MPI_Pcontrol's varargs
        # are not in the header's parameter list, and a `large_only` parameter
        # exists in the _c form alone -- which is what pairs the two forms
        # (NOTES.md #3).
        aparams = [p for p in fn["parameters"]
                   if "c_parameter" not in p["suppress"]
                   and p["kind"] != "VARARGS"
                   and (large or not p["large_only"])]
        cparams = [p for p in ep.params if p.name != "..."]
        if len(aparams) != len(cparams):
            raise SystemExit(
                f"{base}: header has {len(cparams)} parameters, apis.json "
                f"{len(aparams)}")
        for p, ap in zip(cparams, aparams):
            if p.name != ap["name"]:
                expected = NAME_DISAGREEMENTS.get((base, p.name))
                if expected != ap["name"]:
                    raise SystemExit(
                        f"{base}: header calls parameter {p.name!r} what "
                        f"apis.json calls {ap['name']!r}; add it to "
                        f"NAME_DISAGREEMENTS with a reason if that is right")
            p.kind = ap["kind"]
            p.direction = ap["param_direction"]
            p.length = ap["length"]
            p.constant = ap["constant"]
        ep.ret_kind = fn["return_kind"]
    return protos


def assign_status(protos, handwritten_bodies):
    """Fill ep.status/ep.detail for every MPI_ entry point (the PMPI_ twin
    follows its MPI_ name), and fail if the ledger does not cover the rest."""
    unknown = set(HAND_WRITTEN) - set(protos)
    if unknown:
        raise SystemExit("HAND_WRITTEN names entry points the ABI header does "
                         "not have: " + ", ".join(sorted(unknown)))
    stray = set(handwritten_bodies) - set(HAND_WRITTEN)
    if stray:
        raise SystemExit("src/mpiwrapper/handwritten.h defines bodies that the "
                         "ledger does not name: " + ", ".join(sorted(stray)))

    for name, ep in protos.items():
        if name.startswith("PMPI_"):
            continue
        if name in ABI_ALIAS:
            # No slot, no wrapper body: libmpi_abi answers this one itself, by
            # calling the slot of the entry point that replaced it.
            ep.status = "abi-alias"
            ep.detail = ABI_ALIAS[name]
            twin = protos["P" + name]
            twin.status, twin.detail = ep.status, ep.detail
            continue
        if name in HAND_WRITTEN:
            ep.status = "hand-written"
            ep.detail = HAND_WRITTEN[name]
            if name not in handwritten_bodies:
                # Unreachable since S4b, which is what the frozen
                # "hand-written bodies" tally above says. It stays because a
                # *new* ledger entry starts here, and this is what the report
                # would say about it.
                ep.detail += "; no body yet in src/mpiwrapper/"
            continue
        blocked = []
        for p in ep.params:
            if p.name == "...":
                blocked.append("varargs")
                continue
            p.cls = classify(ep, p)
            if p.cls.startswith("S3:"):
                blocked.append(p.cls[len("S3:"):] + f" ({p.name})")
        if ep.ret_kind not in {"ERROR_CODE"} | RET_PASSTHROUGH:
            blocked.append(f"return kind {ep.ret_kind}")
        if name in STATUS_FIELD:
            # Pure ABI-side: the body reads or writes a named field of the
            # caller's own status and never reaches the implementation, so the
            # `status in` class that blocks the other six does not arise, and
            # the body needs no MPIWRAPPER_HAVE_ guard either.
            blocked = []
            ep.unguarded = True
        if blocked:
            ep.status = "deferred"
            ep.detail = "S3: " + "; ".join(sorted(set(blocked)))
        else:
            ep.status = "generated"
            ep.detail = None
        # The PMPI_ twin shares the classification, since it shares the body.
        twin = protos["P" + name]
        twin.status, twin.detail = ep.status, ep.detail
        twin.unguarded = ep.unguarded
        for p, tp in zip(ep.params, twin.params):
            tp.cls = p.cls


def parse_handwritten_h():
    """The set of MPI_ names src/mpiwrapper/handwritten.h has a body for."""
    text = HANDWRITTEN_H.read_text()
    names = set(re.findall(r"\bmpiwrapper_w_(MPI_[A-Za-z0-9_]+)\s*\(", text))
    pnames = set(re.findall(r"\bmpiwrapper_w_P(MPI_[A-Za-z0-9_]+)\s*\(", text))
    if names != pnames:
        raise SystemExit(
            "handwritten.h declares an MPI_/PMPI_ body without its twin: "
            + ", ".join(sorted(names ^ pnames)))
    return names


# ---------------------------------------------------------------------------
# Post-hoc assertions over the emitted text (NOTES.md #3)
# ---------------------------------------------------------------------------

_TARGET_CALL_RE = re.compile(r"\bTARGET\((?P<args>[^;]*?)\)\s*;", re.DOTALL)


def assert_no_abi_argument_reaches_the_call(wrappers_text):
    """The load-bearing assertion: no parameter of an ABI-typed signature may
    appear in the argument list of the implementation call. Only locally
    declared, converted values may."""
    bad = []
    for m in _TARGET_CALL_RE.finditer(wrappers_text):
        args = re.sub(r"\\\s*\n", " ", m.group("args"))
        for ident in re.findall(r"\babi_[A-Za-z0-9_]+\b", args):
            bad.append((ident, args.strip()[:70]))
    if bad:
        raise SystemExit(
            "an ABI-typed parameter reaches the implementation call:\n" +
            "\n".join(f"  {i}: TARGET({a}...)" for i, a in bad[:20]))


def assert_slots_complete(vtable_text, wrappers_text, names):
    """One slot per entry point, in header order -- less the ABI-side aliases,
    which libmpi_abi answers itself and which therefore have nothing on the
    other side of the vtable to point at."""
    slots = re.findall(r"\(\*(P?MPI_[A-Za-z0-9_]+)\)", vtable_text)
    expected = [n for n in names
                if n.split("MPI_", 1)[1] not in
                {a.split("MPI_", 1)[1] for a in ABI_ALIAS}]
    if slots != expected:
        raise SystemExit("the vtable slot list is not the entry-point list "
                         "less the ABI-side aliases")
    names = expected
    filled = re.findall(r"^\s*\.(P?MPI_[A-Za-z0-9_]+)\s*=", wrappers_text,
                        re.MULTILINE)
    if sorted(filled) != sorted(names):
        missing = set(names) - set(filled)
        raise SystemExit("vtable initializer misses: " +
                         ", ".join(sorted(missing)[:20]))


# ---------------------------------------------------------------------------
# gen/report.txt
# ---------------------------------------------------------------------------

def emit_report(protos, tallies, handwritten_bodies):
    mpi = [ep for n, ep in protos.items() if not n.startswith("PMPI_")]
    out = []
    w = out.append
    w("The generator's ledger -- GENERATED FILE, do not edit by hand.")
    w("")
    w("Produced by dev/generate.py. Every ABI entry point appears exactly once")
    w("below: generated, hand-written, implemented on the ABI side in terms of")
    w("another, or deferred with the argument class that blocks it. The")
    w("generator fails if one appears in none of the four, which is what makes")
    w("'nothing was silently dropped' a checked property.")
    w("")
    w("Frozen tallies")
    w("--------------")
    for key, value in tallies.items():
        w(f"  {key:<22} {value}")
    w("")

    w("Generated (%d)" % sum(1 for e in mpi if e.status == "generated"))
    w("-" * 40)
    for ep in sorted(mpi, key=lambda e: e.name):
        if ep.status == "generated":
            w(f"  {ep.name}")
    w("")

    w("Hand-written (%d)" % sum(1 for e in mpi if e.status == "hand-written"))
    w("-" * 40)
    w("  An entry point with a body in src/mpiwrapper/ is marked [done]. All")
    w("  of them are: S1 wrote eight, S4a the converter face (the handle and")
    w("  status converters, the status-consuming functions, the output-string")
    w("  buffers and MPI_Abi_*), S4b the 40 that need state the wrapper owns")
    w("  -- the lifecycle, the thirteen callback registrars S1 had not")
    w("  already written, the buffer attach and detach forms, the dynamic")
    w("  error-code registry, spawn and MPI_Pcontrol -- and S7 the two")
    w("  attribute getters, whose class no signature carries. The count of")
    w("  bodies is a frozen tally above, so one going missing fails")
    w("  generation rather than becoming a stub.")
    w("")
    w("  What still answers MPI_ERR_UNSUPPORTED_OPERATION is decided per")
    w("  build by dev/probe_impl.py: an entry point the implementation does")
    w("  not declare, hand-written or generated alike (decision 6).")
    w("")
    w("  One limitation is this library's rather than the implementation's,")
    w("  and belongs here because no probe reports it: where the")
    w("  implementation has no MPI_BUFFER_AUTOMATIC (it is MPI-4.1), the")
    w("  wrapper emulates it with a fixed buffer of its own")
    w("  (MPIWRAPPER_AUTOBUF_BYTES, 8 MiB). The standard's 'buffer of")
    w("  sufficient size' is unbounded, so a program that would have run")
    w("  against a real automatic buffer can still see MPI_ERR_BUFFER.")
    w("")
    for reason in sorted(set(HAND_WRITTEN.values())):
        w(f"  {reason}")
        for ep in sorted(mpi, key=lambda e: e.name):
            if ep.status == "hand-written" and HAND_WRITTEN[ep.name] == reason:
                mark = "[done]" if ep.name in handwritten_bodies else "     -"
                w(f"    {mark} {ep.name}")
        w("")

    aliases = [e for e in mpi if e.status == "abi-alias"]
    w("Implemented on the ABI side (%d)" % len(aliases))
    w("-" * 40)
    w("  The entry points MPI-3.0 deleted from the standard. The ABI header")
    w("  still declares them, because an ABI is a promise about symbols -- but")
    w("  an implementation need not define them any more, and Open MPI main's")
    w("  libmpi_abi does not. Forwarding through a slot would make that a")
    w("  *link* failure of the whole wrapper rather than the run-time report")
    w("  decision 6 promises, because the probe asks the compiler and the")
    w("  compiler sees the declaration.")
    w("")
    w("  So libmpi_abi answers these itself, by calling the slot of the entry")
    w("  point that replaced each. They have no slot and no wrapper body, and")
    w("  they now work over any implementation with the MPI-2 attribute")
    w("  interface rather than only over one that kept the MPI-1 spelling.")
    w("  The generator checks each pair's return type, arity and parameter")
    w("  types, so a signature that drifts fails generation.")
    w("")
    for ep in sorted(aliases, key=lambda e: e.name):
        w(f"    {ep.name} -> {ep.detail}")
    w("")

    outliving = [e for e in mpi if stages_past_return(e)]
    w("Staged past return (%d)" % len(outliving))
    w("-" * 40)
    w("  These convert an in-direction array *and* hand back a request, so the")
    w("  implementation may still be reading the temporary after the call")
    w("  returns: the block is owned by the request and freed when the")
    w("  implementation nulls the handle -- at completion for a nonblocking")
    w("  operation, at MPI_Request_free for a persistent one (NOTES.md #5.7).")
    w("  This is the one property no assertion here can check, which is why")
    w("  the set is a frozen tally and why test/abi_arrays_test.c exercises")
    w("  both lifetimes.")
    w("")
    for ep in sorted(outliving, key=lambda e: e.name):
        w(f"    {ep.name}")
    w("")

    deferred = [e for e in mpi if e.status == "deferred"]
    w("Deferred to S3 (%d)" % len(deferred))
    w("-" * 40)
    if not deferred:
        w("  None: S3 is complete, and every one of the entry points above is")
        w("  either generated or named in the ledger. The section stays, and")
        w("  so does its frozen tally of zero, because that is what makes a")
        w("  future apis.json or ABI header introducing an argument class the")
        w("  generator cannot place fail here -- rather than quietly emitting")
        w("  one more run-time-reporting stub.")
        w("")
    else:
        w("  The slot exists and reports MPI_ERR_UNSUPPORTED_OPERATION at run "
          "time")
        w("  (decision 6), and the class named is what has to be added for it")
        w("  to become a real body.")
        w("")
    by_reason = {}
    for ep in deferred:
        by_reason.setdefault(ep.detail, []).append(ep.name)
    for reason in sorted(by_reason):
        w(f"  {reason}")
        for name in sorted(by_reason[reason]):
            w(f"    {name}")
        w("")

    w("Returns MPI_ERR_UNSUPPORTED_OPERATION at run time")
    w("-" * 40)
    w("  ...for every entry point above that is deferred or hand-written")
    w("  without a body yet, and -- per implementation, decided by")
    w("  dev/probe_impl.py at configure time -- for every generated one")
    w("  the implementation does not define. The second set is a property of")
    w("  the build, not of the generator, so it is not listed here.")
    w("")
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    check = "--check" in sys.argv

    # The S0 step, so that one command produces all seven artifacts.
    patched = gh.apply_patch()
    mpi_names, pmpi_names = gh.extract_entrypoints(patched)
    gh.check_symmetry(mpi_names, pmpi_names)
    mpi_h_text = gh.GENERATED_NOTICE_MPI_H + "\n" + patched
    mpiabi_h_text = gh.build_mpiabi_h(patched)

    protos = load(patched)

    classes = parse_handle_classes(patched)
    if sorted(c.lower() for c in classes) != sorted(HANDLE_KIND.values()):
        raise SystemExit(
            "the ABI header's handle classes are not the ones HANDLE_KIND "
            f"maps: {sorted(classes)}")
    handles = parse_handle_constants(patched, classes)
    enums = parse_enum_members(patched)

    # The deleted-in-MPI-3.0 set, taken from the header's own marker rather
    # than from a list here, so a sixth cannot appear unnoticed.
    deprecated_mpi2 = {n for n, ep in protos.items()
                       if not n.startswith("PMPI_")
                       and ep.deprecated_in == "MPI-2.0"}
    check_aliases(protos, parse_typedefs(patched), deprecated_mpi2)

    handwritten_bodies = parse_handwritten_h()
    assign_status(protos, handwritten_bodies)

    names = list(protos)
    mpi_eps = [ep for n, ep in protos.items() if not n.startswith("PMPI_")]
    pairs = [(ep, protos["P" + ep.name]) for ep in mpi_eps]

    vtable_h = emit_vtable_h(list(protos.values()))
    entrypoints_c = emit_entrypoints_c(list(protos.values()))
    wrappers_c = emit_wrappers_c(pairs, handwritten_bodies)
    constants_c = emit_constants_c(classes, handles, enums)

    assert_no_abi_argument_reaches_the_call(wrappers_c)
    assert_slots_complete(vtable_h, wrappers_c, names)

    tallies = {
        "entry points": len(mpi_eps),
        "vtable slots": len(protos) - 2 * len(ABI_ALIAS),
        "handle classes": len(classes),
        "predefined handles": sum(len(v) for v in handles.values()),
        "error classes": sum(1 for n in enums
                             if (n.startswith("MPI_ERR_") or
                                 n.startswith("MPI_T_ERR_"))
                             and n != "MPI_ERR_LASTCODE"),
        "generated": sum(1 for e in mpi_eps if e.status == "generated"),
        "hand-written": sum(1 for e in mpi_eps if e.status == "hand-written"),
        "hand-written bodies": len(handwritten_bodies),
        "deferred to S3": sum(1 for e in mpi_eps if e.status == "deferred"),
        "ABI-side aliases": sum(1 for e in mpi_eps if e.status == "abi-alias"),
        "staged past return": sum(1 for e in mpi_eps if stages_past_return(e)),
    }
    drift = {k: (v, FROZEN[k]) for k, v in tallies.items() if v != FROZEN.get(k)}
    if drift:
        raise SystemExit(
            "frozen tallies changed -- reclassify deliberately, then update "
            "FROZEN:\n" + "\n".join(f"  {k}: {got} (frozen: {want})"
                                    for k, (got, want) in drift.items()))

    report = emit_report(protos, tallies, handwritten_bodies)

    targets = {
        gh.OUT_MPI_H: mpi_h_text,
        gh.OUT_MPIABI_H: mpiabi_h_text,
        gh.OUT_ENTRYPOINTS: "\n".join(mpi_names) + "\n",
        OUT_VTABLE_H: vtable_h,
        OUT_ENTRYPOINTS_C: entrypoints_c,
        OUT_WRAPPERS_C: wrappers_c,
        OUT_CONSTANTS_C: constants_c,
        OUT_REPORT: report,
    }

    if check:
        stale = [str(p.relative_to(ROOT)) for p, text in targets.items()
                 if not p.exists() or p.read_text() != text]
        if stale:
            raise SystemExit("regeneration differs from committed output: " +
                             ", ".join(stale))
        print(f"OK: {tallies['entry points']} entry points, "
              f"{tallies['generated']} generated, "
              f"{tallies['hand-written']} hand-written, "
              f"{tallies['deferred to S3']} deferred; "
              "regeneration matches committed output")
        return

    for path, text in targets.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        print(f"wrote {path.relative_to(ROOT)}")
    print(f"{tallies['entry points']} entry points: {tallies['generated']} "
          f"generated, {tallies['hand-written']} hand-written, "
          f"{tallies['deferred to S3']} deferred to S3")


if __name__ == "__main__":
    main()
