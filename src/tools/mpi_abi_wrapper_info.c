/* mpi_abi_wrapper_info -- what this prefix was built to do, and what it
 * actually resolved to at run time. The ompi_info / mpichversion analogue, and
 * the first thing to ask when a program built against this prefix misbehaves
 * (NOTES.md #9, decision 27).
 *
 * It reports decision 5's resolution *from the outside*. The obvious design --
 * ask libmpi_abi which wrapper it loaded -- is ruled out: that library exports
 * exactly the 1376 MPI_ and PMPI_ names of the ABI and nothing else, a tally
 * test/check_exports.cmake checks in both directions, so there is no accessor
 * to add. So the two paths below come from the same CMake variables the
 * library and bin/mpiexec bake in, with the environment variable that
 * overrides each shown beside it.
 *
 * One limitation, stated rather than papered over: libmpi_abi resolves and
 * dlopens the wrapper from a *constructor*, so when that fails the process
 * aborts before main() and this tool prints nothing. That is not a gap in the
 * diagnostic -- bootstrap.c's vt_fail names the path it tried and the variable
 * to set, which is the same information the "wrapper:" line below would carry.
 * The launcher line is the part that is then unavailable, and
 * `mpiexec -showme:launcher` answers it without loading anything.
 *
 * The default mode calls no MPI function that reaches the implementation
 * except MPI_Get_library_version, which is legal before MPI_Init and which is
 * the run-time proof that the whole chain resolved. --full adds the calls that
 * need MPI_Init: MPI_Abi_get_info creates an MPI_Info (hw_abi.c), so it cannot
 * live in a mode that ci-scripts/check-install.sh runs with no launcher.
 */

#include <mpi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Every one of these is baked in by CMakeLists.txt from the same variable the
 * artifact it describes uses, so the two cannot disagree.
 */
#ifndef MPI_ABI_WRAPPER_INFO_VERSION
#  define MPI_ABI_WRAPPER_INFO_VERSION "unknown"
#endif
#ifndef MPI_ABI_WRAPPER_INFO_PREFIX
#  define MPI_ABI_WRAPPER_INFO_PREFIX ""
#endif
#ifndef MPI_ABI_WRAPPER_INFO_INCLUDEDIR
#  define MPI_ABI_WRAPPER_INFO_INCLUDEDIR ""
#endif
#ifndef MPI_ABI_WRAPPER_INFO_LIBDIR
#  define MPI_ABI_WRAPPER_INFO_LIBDIR ""
#endif
#ifndef MPI_ABI_WRAPPER_INFO_LIB_DEFAULT
#  define MPI_ABI_WRAPPER_INFO_LIB_DEFAULT ""
#endif
#ifndef MPI_ABI_WRAPPER_INFO_MPIEXEC_DEFAULT
#  define MPI_ABI_WRAPPER_INFO_MPIEXEC_DEFAULT ""
#endif
#ifndef MPI_ABI_WRAPPER_INFO_CXX
#  define MPI_ABI_WRAPPER_INFO_CXX ""
#endif
#ifndef MPI_ABI_WRAPPER_INFO_FORTRAN
#  define MPI_ABI_WRAPPER_INFO_FORTRAN "no"
#endif

/* A path, whether the environment overrode the baked value, and whether what
 * the answer names is actually there. The last column is the point: both of
 * this design's new failure modes -- a wrapper or a launcher that has moved --
 * are one word of output rather than a puzzle.
 */
static void print_resolved(const char *label, const char *envname,
                           const char *baked, int want_executable)
{
  const char *env      = getenv(envname);
  const int   from_env = env && *env;
  const char *value    = from_env ? env : baked;

  printf("%-24s %s\n", label, *value ? value : "(none)");
  printf("%-24s %s\n", "  from", from_env ? envname : "the build");
  if (*value)
    printf("%-24s %s\n", "  present",
           access(value, want_executable ? X_OK : R_OK) == 0 ? "yes" : "NO");
}

int main(int argc, char **argv)
{
  int full = 0;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--full") == 0) {
      full = 1;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      printf("usage: %s [--full]\n", argv[0]);
      printf("  --full  also call MPI_Init and report what needs it\n");
      return 0;
    } else {
      fprintf(stderr, "%s: unknown argument: %s\n", argv[0], argv[i]);
      return 2;
    }
  }

  printf("%-24s %s\n", "package:", "mpi_abi_wrapper " MPI_ABI_WRAPPER_INFO_VERSION);

  /* From the header this tool compiled against, so it is the number a
   * consumer of this prefix would also see, not a second copy of it.
   */
  printf("%-24s %d.%d\n", "mpi abi (header):", MPI_ABI_VERSION, MPI_ABI_SUBVERSION);

  /* And from the library, through the ABI's own query. hw_abi.c answers it
   * from the ABI header's macros without reaching the implementation, so it
   * is safe here and it cross-checks the line above.
   */
  {
    int major = -1, minor = -1;
    if (MPI_Abi_get_version(&major, &minor) == MPI_SUCCESS)
      printf("%-24s %d.%d\n", "mpi abi (library):", major, minor);
    else
      printf("%-24s %s\n", "mpi abi (library):", "(query failed)");
  }

  printf("%-24s %s\n", "prefix:", MPI_ABI_WRAPPER_INFO_PREFIX);
  printf("%-24s %s\n", "includedir:", MPI_ABI_WRAPPER_INFO_INCLUDEDIR);
  printf("%-24s %s\n", "libdir:", MPI_ABI_WRAPPER_INFO_LIBDIR);
  printf("%-24s %s\n", "mpicxx:",
         *MPI_ABI_WRAPPER_INFO_CXX ? MPI_ABI_WRAPPER_INFO_CXX : "(not built)");
  printf("%-24s %s\n", "fortran probe:", MPI_ABI_WRAPPER_INFO_FORTRAN);

  print_resolved("wrapper:", "MPI_ABI_WRAPPER_LIB",
                 MPI_ABI_WRAPPER_INFO_LIB_DEFAULT, 0);
  print_resolved("launcher:", "MPI_ABI_WRAPPER_MPIEXEC",
                 MPI_ABI_WRAPPER_INFO_MPIEXEC_DEFAULT, 1);

  /* Legal before MPI_Init, and the only line here that proves the chain
   * end to end: it is answered by the wrapped implementation, through the
   * wrapper libmpi_abi actually loaded. Decision 26 prepends this library's
   * own banner, so the string carries both halves.
   */
  {
    static char version[MPI_MAX_LIBRARY_VERSION_STRING];
    int         len = 0;

    if (MPI_Get_library_version(version, &len) == MPI_SUCCESS && len > 0) {
      /* Multi-line for both implementations -- MPICH's is its whole configure
       * line -- so this one field gets a block of its own rather than being
       * forced into the key/value column the rest of the report uses.
       */
      printf("%s\n", "library version:");
      for (const char *line = version; *line;) {
        const char *end = strchr(line, '\n');
        const int   n   = end ? (int)(end - line) : (int)strlen(line);
        printf("  %.*s\n", n, line);
        line = end ? end + 1 : line + n;
      }
    } else {
      printf("%-24s %s\n", "library version:", "(query failed)");
    }
  }

  if (!full) return 0;

  /* --full: everything that needs an initialized MPI. Under a launcher this
   * reports the job as well, which makes the tool usable as
   * `mpiexec -n 2 mpi_abi_wrapper_info --full`.
   */
  if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
    fprintf(stderr, "%s: MPI_Init failed\n", argv[0]);
    return 1;
  }

  {
    int size = -1, rank = -1;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("%-24s rank %d of %d\n", "comm world:", rank, size);
  }

  {
    MPI_Info info = MPI_INFO_NULL;
    if (MPI_Abi_get_info(&info) == MPI_SUCCESS && info != MPI_INFO_NULL) {
      static const char *const keys[] = {"mpi_aint_size", "mpi_count_size",
                                         "mpi_offset_size"};
      for (size_t i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        char value[MPI_MAX_INFO_VAL];
        int  buflen = MPI_MAX_INFO_VAL;
        int  flag   = 0;
        if (MPI_Info_get_string(info, keys[i], &buflen, value, &flag) ==
                MPI_SUCCESS &&
            flag)
          printf("%-24s %s\n", keys[i], value);
      }
      MPI_Info_free(&info);
    } else {
      printf("%-24s %s\n", "abi info:", "(not available)");
    }
  }

  {
    int is_set = 0;
    int logical_true = 0, logical_false = 0;
    if (MPI_Abi_get_fortran_booleans(sizeof(int), &logical_true, &logical_false,
                                     &is_set) == MPI_SUCCESS)
      printf("%-24s %s\n", "fortran booleans:", is_set ? "set" : "not set");
  }

  MPI_Finalize();
  return 0;
}
