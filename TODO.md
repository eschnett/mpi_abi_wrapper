# Erik's open review questions

- test the dlopen/dlmopen mechanism in a mock-up, both on linux
  (docker) and macos. (on macos a two-level namespace is probably
  necessary.)

- the calls to `vt()` in the wrappers are expensive. would it be
  cheaper to initialize a global pointer to `NULL` and crash if MPI
  functions are called before initialization? initialization happens
  when the shared library is loaded, i.e. this would never happen.
  
  also, would it be cheaper to copy the vtable to save one pointer
  dereference? i want to reduce the per-call overhead -- maybe this is
  over-optimizing things since C++ makes sure that such vtable
  accesses are efficient on modern hardware anyway?

- should both `MPI_*` and `PMPI_*` functions call the wrapped `MPI_*`
  functions? this sounds weird. the cost of wrapping the `PMPI_*`
  functions is small, why not do that as well?

- the handle maps use hash tables. would sorted arrays be faster? they
  would allow a binary search, or possible arithmetic search (O(log
  log n)).

- the argument for "not a switch" in `mpiwrapper_rank_toabi` is wrong.
  a switch would totally work there.

- the error handling logic in the example `w_MPI_Waitall` is
  complicated. consider using `goto` to simplify.

- the distinction between `libmpi_abi` and `libmpiwrapper` should not
  be user visible. by default, build both, for a given MPI
  implementation. the distinction "needs MPI" is interesting, but keep
  it internal.

- consumers. other large projects (e.g. HDF5, PETSc) are also
  important consumers. do not focus only on mpif.

- MPI 4.0: "Every ABI function then maps 1:1 to one implementation
  call". to clarify: implement the full MPI ABI, which is MPI 5.0
  (plus the Fortran extension). missing functions should be reported
  at run time, not omitted from the ABI. however, you can expect the
  MPI 4.0 API to be available.

- `mpiwrapper_get_vtable`: there not only `MPI_ABI_VERSION` but also
  `MPI_ABI_SUBVERSION`; report and check both.

- "The wrapper's installed name encodes its MPI": no, this probably
  won't work because many HPC systems have additional ABI differences
  (loaded modules, compilers, etc.). this mechanism introduces
  complexity and probably won't find many errors. all wrappers should
  be installed into separate prefixes, possibly next to the MPI
  library they are wrapping.

- define a set of independent (but sequential) stages that implement
  this project.

  each stage should roughly correspond to a single AI session. suggest
  which model to use.

  the first stage should be the prototype.
