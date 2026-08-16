/* Does a *staged* entry point ever get a shared built-in request, and can
 * MPI_Request_get_status tell?
 *
 * probe.c asks the identity question of MPI_Isend and MPI_Ibarrier, neither of
 * which stages anything, and of one shape of MPI_Ialltoallw that turns out to
 * be the shape that does not share. This asks it of the four operations that
 * stage -- MPI_Ialltoallw, MPI_Alltoallw_init, MPI_Ineighbor_alltoallw,
 * MPI_Neighbor_alltoallw_init -- in the zero-work forms both implementations
 * have a shortcut for, and then asks whether MPI_Request_get_status separates
 * the cases NOTES.md #13.2's fix needs it to separate.
 *
 *   cc -I$MPI/include -L$MPI/lib -o probe-staged probe-staged.c -lmpi
 *   ./probe-staged
 *   MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM=tsp_inplace ./probe-staged   # MPICH
 *
 * One rank is enough: the shortcut is a property of this rank's own schedule.
 */
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>

static unsigned long long h(MPI_Request r)
{
  return (unsigned long long)(uintptr_t)r;
}

static void show(const char *what, MPI_Request *r, int n)
{
  printf("  %-46s", what);
  for (int i = 0; i < n; ++i) printf(" %#llx", h(r[i]));
  int same = 1;
  for (int i = 1; i < n; ++i)
    if (r[i] != r[0]) same = 0;
  printf("   %s\n", same ? "<-- ALL THE SAME" : "distinct");
}

static void ask(const char *what, MPI_Request r)
{
  int flag = -1;
  int rc   = MPI_Request_get_status(r, &flag, MPI_STATUS_IGNORE);
  printf("  %-46s handle=%#-14llx rc=%d flag=%d\n", what, h(r), rc, flag);
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  MPI_Request  r[3];
  int          sbuf[512] = {0}, rbuf[512] = {0};
  MPI_Aint     zero_a[1] = {0};
  int          zero_i[1] = {0};
  int          one_i[1]  = {1};
  MPI_Datatype types[1]  = {MPI_INT};

  printf("Which entry points hand back one shared value?\n");

  for (int i = 0; i < 3; ++i)
    MPI_Isend(NULL, 0, MPI_INT, MPI_PROC_NULL, 0, MPI_COMM_SELF, &r[i]);
  MPI_Request procnull_send = r[0];
  show("Isend to MPI_PROC_NULL x3", r, 3);
  for (int i = 0; i < 3; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);

  /* an ordinary short send, not a MPI_PROC_NULL one: both implementations
   * answer an eagerly completed send with the built-in too */
  MPI_Request rq;
  MPI_Irecv(rbuf, 1, MPI_INT, 0, 7, MPI_COMM_SELF, &rq);
  MPI_Isend(sbuf, 1, MPI_INT, 0, 7, MPI_COMM_SELF, &r[0]);
  printf("  %-46s %#llx   %s\n", "ordinary 1-int Isend on COMM_SELF", h(r[0]),
         r[0] == procnull_send ? "<-- the same shared built-in" : "own request");
  MPI_Wait(&r[0], MPI_STATUS_IGNORE);
  MPI_Wait(&rq, MPI_STATUS_IGNORE);

  for (int i = 0; i < 3; ++i)
    MPI_Ialltoallw(MPI_IN_PLACE, NULL, NULL, NULL, rbuf, zero_i, zero_i, types,
                   MPI_COMM_SELF, &r[i]);
  MPI_Request staged = r[0];
  show("Ialltoallw IN_PLACE, counts 0, COMM_SELF x3", r, 3);
  for (int i = 0; i < 3; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);
  printf("  %-46s %s\n", "  ... same value as the PROC_NULL Isend?",
         staged == procnull_send ? "YES -- shared across kinds" : "no");

  for (int i = 0; i < 3; ++i)
    MPI_Ialltoallw(MPI_IN_PLACE, NULL, NULL, NULL, rbuf, one_i, zero_i, types,
                   MPI_COMM_SELF, &r[i]);
  show("Ialltoallw IN_PLACE, count 1, COMM_SELF x3", r, 3);
  for (int i = 0; i < 3; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);

  for (int i = 0; i < 3; ++i)
    MPI_Ialltoallw(sbuf, zero_i, zero_i, types, rbuf, zero_i, zero_i, types,
                   MPI_COMM_SELF, &r[i]);
  show("Ialltoallw, counts 0, COMM_SELF x3", r, 3);
  for (int i = 0; i < 3; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);

  for (int i = 0; i < 3; ++i)
    MPI_Alltoallw_init(MPI_IN_PLACE, NULL, NULL, NULL, rbuf, zero_i, zero_i,
                       types, MPI_COMM_SELF, MPI_INFO_NULL, &r[i]);
  show("Alltoallw_init IN_PLACE, counts 0 x3", r, 3);
  for (int i = 0; i < 3; ++i) MPI_Request_free(&r[i]);

  MPI_Comm g;
  MPI_Dist_graph_create_adjacent(MPI_COMM_SELF, 0, NULL, NULL, 0, NULL, NULL,
                                 MPI_INFO_NULL, 0, &g);
  for (int i = 0; i < 3; ++i)
    MPI_Ineighbor_alltoallw(sbuf, zero_i, zero_a, types, rbuf, zero_i, zero_a,
                            types, g, &r[i]);
  show("Ineighbor_alltoallw, degree 0 x3", r, 3);
  for (int i = 0; i < 3; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);

  for (int i = 0; i < 3; ++i)
    MPI_Neighbor_alltoallw_init(sbuf, zero_i, zero_a, types, rbuf, zero_i,
                                zero_a, types, g, MPI_INFO_NULL, &r[i]);
  show("Neighbor_alltoallw_init, degree 0 x3", r, 3);
  for (int i = 0; i < 3; ++i) MPI_Request_free(&r[i]);
  MPI_Comm_free(&g);

  printf("\nWhat does MPI_Request_get_status answer?\n");

  MPI_Isend(NULL, 0, MPI_INT, MPI_PROC_NULL, 0, MPI_COMM_SELF, &r[0]);
  ask("shared built-in (PROC_NULL Isend)", r[0]);
  MPI_Wait(&r[0], MPI_STATUS_IGNORE);

  MPI_Ialltoallw(MPI_IN_PLACE, NULL, NULL, NULL, rbuf, zero_i, zero_i, types,
                 MPI_COMM_SELF, &r[0]);
  ask("zero-work Ialltoallw", r[0]);
  MPI_Wait(&r[0], MPI_STATUS_IGNORE);

  MPI_Request pending;
  MPI_Irecv(rbuf, 1, MPI_INT, 0, 99, MPI_COMM_SELF, &pending);
  ask("in flight (Irecv, no matching send yet)", pending);
  MPI_Isend(sbuf, 1, MPI_INT, 0, 99, MPI_COMM_SELF, &r[0]);
  MPI_Wait(&r[0], MPI_STATUS_IGNORE);
  MPI_Wait(&pending, MPI_STATUS_IGNORE);

  MPI_Alltoallw_init(sbuf, one_i, zero_i, types, rbuf, one_i, zero_i, types,
                     MPI_COMM_SELF, MPI_INFO_NULL, &r[0]);
  ask("persistent, fresh -- the trap (MPI-5.0 3.7.6)", r[0]);
  MPI_Start(&r[0]);
  ask("persistent, after MPI_Start", r[0]);
  MPI_Wait(&r[0], MPI_STATUS_IGNORE);
  MPI_Request_free(&r[0]);

  enum { N = 200000 };
  MPI_Isend(NULL, 0, MPI_INT, MPI_PROC_NULL, 0, MPI_COMM_SELF, &r[0]);
  double t0 = MPI_Wtime();
  for (int i = 0; i < N; ++i) {
    int flag;
    MPI_Request_get_status(r[0], &flag, MPI_STATUS_IGNORE);
  }
  double t1 = MPI_Wtime();
  MPI_Wait(&r[0], MPI_STATUS_IGNORE);
  printf("  %-46s %.1f ns/call\n", "cost, on a complete request",
         (t1 - t0) * 1e9 / N);

  MPI_Finalize();
  return 0;
}
