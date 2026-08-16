/* Refuse a launcher that silently delivered fewer ranks than the build asked
 * for.
 *
 * The five black-box tests all support one rank as well as two, which is what
 * makes -DMPI_ABI_TEST_USE_LAUNCHER=OFF a usable configuration (CODE.md 10).
 * That tolerance has a failure mode: a launcher that accepts `-n 2` and starts
 * two *singletons* produces two passing one-rank runs, and `ctest` reports
 * green for a configuration that never exercised a second rank at all. It is
 * the exact shape of the mistake test/README.md warns about for a mismatched
 * `mpiexec`, and it is not hypothetical -- the MPICH row of the Linux container
 * had been running that way, and nothing in a green run said so (HISTORY.md
 * 2.14).
 *
 * So the build states its expectation instead of leaving it implicit. CMake
 * puts MPI_ABI_EXPECT_RANKS in each test's environment: the number it passed to
 * the launcher, or 1 when there is no launcher. A test that gets a different
 * number fails and says what it was promised. Running a test binary by hand
 * sets nothing and keeps the old tolerance, which is what makes a one-off
 * `mpiexec -n 1 ./abi_state_test` still useful while debugging.
 *
 * The check is on the *count*, not on which count: it is equally an error to be
 * given two ranks by a build that asked for one, since that build's tests are
 * written for the singleton path.
 */
#ifndef MPIABI_TEST_EXPECT_RANKS_H
#define MPIABI_TEST_EXPECT_RANKS_H

#include <stdio.h>
#include <stdlib.h>

/* 0 if the rank count is the one the build asked the launcher for -- or if no
 * expectation was stated. Nonzero, after printing why, if it is not.
 */
static inline int mpiabi_expect_ranks(const char *test, int size, int rank)
{
  const char *want = getenv("MPI_ABI_EXPECT_RANKS");
  if (want == NULL || *want == '\0')
    return 0;

  int expected = atoi(want);
  if (expected <= 0 || expected == size)
    return 0;

  /* Every process says this: if the job did become N singletons, each of them
   * is rank 0 of its own MPI_COMM_WORLD, and N copies of the line is a truer
   * picture of what happened than one would be.
   */
  if (rank == 0)
    printf("%s: this build asked the launcher for %d rank%s and got %d. "
           "A job that silently became separate singletons passes every test "
           "here without ever crossing a rank boundary, so it is a failure "
           "rather than a weaker pass. Configure with "
           "-DMPI_ABI_TEST_USE_LAUNCHER=OFF to run singletons on purpose.\n",
           test, expected, expected == 1 ? "" : "s", size);
  return 1;
}

#endif /* MPIABI_TEST_EXPECT_RANKS_H */
