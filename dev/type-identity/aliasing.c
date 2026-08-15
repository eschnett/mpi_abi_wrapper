/* If a cast *is* needed, is it safe? The worry is type-based alias analysis:
 * an array written as one type and read as another of the same representation
 * is a strict-aliasing violation on paper even when the layout is identical.
 *
 * This is the worst case for us, not the realistic one: everything is in a
 * single translation unit at -O3, so the optimizer sees both the write and the
 * read and may apply TBAA freely. In the real system the implementation is a
 * separate shared library and cannot see our stores at all.
 */
#include <mpi.h>

#include "mpiabi.h"

#include <stdio.h>

#define N 8

static long long sum_as_impl(const MPI_Count *a, int n)
{
  long long s = 0;
  for (int i = 0; i < n; ++i) s += a[i];
  return s;
}

static MPI_Aint max_as_impl(const MPI_Aint *a, int n)
{
  MPI_Aint m = a[0];
  for (int i = 1; i < n; ++i)
    if (a[i] > m) m = a[i];
  return m;
}

int main(void)
{
  MPIABI_Count counts[N];
  MPIABI_Aint  displs[N];
  long long    expect_sum = 0;
  MPIABI_Aint  expect_max = 0;

  for (int i = 0; i < N; ++i) {
    counts[i] = (MPIABI_Count)1 << (i + 40); /* values that need all 64 bits */
    displs[i] = (MPIABI_Aint)i * 1000003;
    expect_sum += counts[i];
    if (displs[i] > expect_max) expect_max = displs[i];
  }

  const long long   got_sum = sum_as_impl((const MPI_Count *)counts, N);
  const MPI_Aint    got_max = max_as_impl((const MPI_Aint *)displs, N);

  printf("  sum through a cast pointer: %s (%lld vs %lld)\n",
         got_sum == expect_sum ? "correct" : "WRONG", got_sum, expect_sum);
  printf("  max through a cast pointer: %s (%lld vs %lld)\n",
         (long long)got_max == (long long)expect_max ? "correct" : "WRONG",
         (long long)got_max, (long long)expect_max);
  return !(got_sum == expect_sum && (long long)got_max == (long long)expect_max);
}
