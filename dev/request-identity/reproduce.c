/* The built-in-request refusal, from the ABI side.
 *
 * probe-staged.c shows the implementations handing one shared request value to
 * two staged operations. This shows what the wrapper then does with it: the
 * second MPI_Ialltoallw meets its own key in the request-keyed table and is
 * answered MPI_ERR_INTERN, though the call was legal (NOTES.md #13.2). The
 * default error handler turns that into an aborted job; this program installs
 * MPI_ERRORS_RETURN so the refusal is visible rather than fatal.
 *
 * Unlike the other probes here, this one links against the *ABI*, not against
 * an implementation:
 *
 *   cc -I<abi-include> -o reproduce reproduce.c -L<prefix>/lib -lmpi_abi \
 *      -Wl,-rpath,<prefix>/lib
 *   MPI_ABI_WRAPPER_LIB=<prefix>/lib/libmpiwrapper.so ./reproduce
 *
 * where <abi-include> is the ABI mpi.h (dev/vendor/mpi-abi-stubs/ will do).
 *
 * Two cases, because they fail for different reasons:
 *
 *   1. MPI_Ialltoallw with all counts zero. The wrapper has staged a datatype
 *      array that must outlive the call, and the shared request is the only
 *      key it has to hang it on.
 *   2. MPI_Ineighbor_alltoallw on a degree-0 topology. Here both extents are
 *      zero, so there is nothing to keep alive at all -- the wrapper attaches
 *      a zero-length block because the attach is unconditional.
 */
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>

static int failures;

static void report(const char *what, int e1, int e2, MPI_Request a,
                   MPI_Request b)
{
  printf("%s\n", what);
  printf("  first:  rc=%-3d request=%#llx\n", e1,
         (unsigned long long)(uintptr_t)a);
  printf("  second: rc=%-3d request=%#llx\n", e2,
         (unsigned long long)(uintptr_t)b);
  if (e2 != MPI_SUCCESS) {
    char s[MPI_MAX_ERROR_STRING];
    int  n = 0;
    MPI_Error_string(e2, s, &n);
    printf("          %.*s\n", n, s);
  }
  if (e1 == MPI_SUCCESS && e2 == MPI_SUCCESS) {
    printf("  both accepted\n\n");
  } else {
    printf("  *** a legal call was refused ***\n\n");
    ++failures;
  }
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
  MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);

  int          buf[8]   = {0};
  int          zero[1]  = {0};
  MPI_Datatype types[1] = {MPI_INT};
  MPI_Request  a = MPI_REQUEST_NULL, b = MPI_REQUEST_NULL;

  int e1 = MPI_Ialltoallw(buf, zero, zero, types, buf + 4, zero, zero, types,
                          MPI_COMM_SELF, &a);
  int e2 = MPI_Ialltoallw(buf, zero, zero, types, buf + 4, zero, zero, types,
                          MPI_COMM_SELF, &b);
  report("two zero-work MPI_Ialltoallw, posted before either is waited on", e1,
         e2, a, b);
  if (a != MPI_REQUEST_NULL) MPI_Wait(&a, MPI_STATUS_IGNORE);
  if (b != MPI_REQUEST_NULL) MPI_Wait(&b, MPI_STATUS_IGNORE);

  MPI_Comm g;
  MPI_Dist_graph_create_adjacent(MPI_COMM_SELF, 0, NULL, NULL, 0, NULL, NULL,
                                 MPI_INFO_NULL, 0, &g);
  MPI_Comm_set_errhandler(g, MPI_ERRORS_RETURN);
  a = b = MPI_REQUEST_NULL;
  e1 = MPI_Ineighbor_alltoallw(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               g, &a);
  e2 = MPI_Ineighbor_alltoallw(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               g, &b);
  report("two degree-0 MPI_Ineighbor_alltoallw, same", e1, e2, a, b);
  if (a != MPI_REQUEST_NULL) MPI_Wait(&a, MPI_STATUS_IGNORE);
  if (b != MPI_REQUEST_NULL) MPI_Wait(&b, MPI_STATUS_IGNORE);
  MPI_Comm_free(&g);

  MPI_Finalize();
  return failures != 0;
}
