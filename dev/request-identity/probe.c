#include <mpi.h>
#include <stdio.h>
#include <string.h>
static void show(const char *what, MPI_Request *r, int n)
{
  printf("%-38s", what);
  for (int i = 0; i < n; ++i) printf(" %#llx", (unsigned long long)(uintptr_t)r[i]);
  int same = 1;
  for (int i = 1; i < n; ++i) if (r[i] != r[0]) same = 0;
  printf("   %s\n", same ? "<-- ALL THE SAME" : "distinct");
}
int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  MPI_Request r[4];
  int buf[8] = {0}, counts[1] = {1}, displs[1] = {0};
  MPI_Datatype types[1] = {MPI_INT};

  for (int i = 0; i < 4; ++i)
    MPI_Isend(NULL, 0, MPI_INT, MPI_PROC_NULL, 0, MPI_COMM_SELF, &r[i]);
  show("Isend to MPI_PROC_NULL x4", r, 4);
  for (int i = 0; i < 4; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);

  for (int i = 0; i < 4; ++i)
    MPI_Irecv(NULL, 0, MPI_INT, MPI_PROC_NULL, 0, MPI_COMM_SELF, &r[i]);
  show("Irecv from MPI_PROC_NULL x4", r, 4);
  for (int i = 0; i < 4; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);

  for (int i = 0; i < 4; ++i) MPI_Ibarrier(MPI_COMM_SELF, &r[i]);
  show("Ibarrier on MPI_COMM_SELF x4", r, 4);
  for (int i = 0; i < 4; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);

  for (int i = 0; i < 4; ++i)
    MPI_Ialltoallw(buf, counts, displs, types, buf + 4, counts, displs, types,
                   MPI_COMM_SELF, &r[i]);
  show("Ialltoallw on MPI_COMM_SELF x4", r, 4);
  for (int i = 0; i < 4; ++i) MPI_Wait(&r[i], MPI_STATUS_IGNORE);

  /* recycling: complete one, then post another and see if the value returns */
  MPI_Request a, b;
  MPI_Ialltoallw(buf, counts, displs, types, buf + 4, counts, displs, types,
                 MPI_COMM_SELF, &a);
  MPI_Request a_saved = a;
  MPI_Wait(&a, MPI_STATUS_IGNORE);
  MPI_Ialltoallw(buf, counts, displs, types, buf + 4, counts, displs, types,
                 MPI_COMM_SELF, &b);
  printf("%-38s %#llx then %#llx   %s\n", "Ialltoallw, complete, Ialltoallw",
         (unsigned long long)(uintptr_t)a_saved, (unsigned long long)(uintptr_t)b,
         a_saved == b ? "<-- VALUE REUSED" : "distinct");
  MPI_Wait(&b, MPI_STATUS_IGNORE);
  MPI_Finalize();
  return 0;
}
