/* clock_gettime needs a POSIX feature macro under -std=c11 on glibc. */
#define _POSIX_C_SOURCE 200809L

/* How much does the ABI side's dispatch cost per call, and does it matter?
 *
 * Reports ns/call for five dispatch shapes against two callees: a trivial one
 * (models MPI_Wtime / MPI_Comm_rank) and one calibrated to roughly a small
 * MPI_Send. The second is what decides whether any of this is worth doing.
 */

#include "bench.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern int callee_spin_iters;
extern int          callee_real(int);

static double now_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static volatile int sink;

/* One timed burst of a single shape. */
static double burst_ns_per_call(int (*fn)(int), long iters)
{
  const double t0 = now_ns();
  int          acc = 0;
  for (long i = 0; i < iters; ++i) acc = fn(acc);
  const double t1 = now_ns();
  sink             = acc;
  return (t1 - t0) / (double)iters;
}

/* Shapes are measured round-robin, one burst each per rep, and the minimum is
 * kept. Measuring each shape to completion in turn lets frequency and thermal
 * drift land entirely on whichever shape happened to be running -- the first
 * version of this benchmark did that and reported a 213% overhead for one extra
 * load, plus a dispatch shape that beat a direct call.
 */
static void measure_all(int (*const *fns)(int), int n, long iters, int reps,
                        double *out)
{
  for (int i = 0; i < n; ++i) out[i] = 1e300;
  for (int r = 0; r < reps; ++r)
    for (int i = 0; i < n; ++i) {
      const double t = burst_ns_per_call(fns[i], iters);
      if (t < out[i]) out[i] = t;
    }
}

struct row {
  const char *name;
  int (*cheap)(int);
  int (*real)(int);
};

int main(int argc, char **argv)
{
  const long iters = argc > 1 ? atol(argv[1]) : 20000000L;
  const int  reps  = argc > 2 ? atoi(argv[2]) : 5;

  dispatch_init();

  /* Calibrate the "real" callee to about 250 ns, a plausible small MPI_Send. */
  callee_spin_iters = 1;
  for (int i = 0; i < 60; ++i) {
    double t = 1e300;
    for (int r = 0; r < 3; ++r) {
      const double u = burst_ns_per_call(callee_real, 200000);
      if (u < t) t = u;
    }
    if (t >= 250.0) break;
    callee_spin_iters = (int)((double)callee_spin_iters * 1.5) + 1;
  }
  double real_cost = 1e300;
  for (int r = 0; r < 5; ++r) {
    const double u = burst_ns_per_call(callee_real, 200000);
    if (u < real_cost) real_cost = u;
  }

  const struct row rows[] = {
      {"direct call (linked)", disp_direct_cheap, disp_direct_real},
      {"static fn pointers", disp_ptrs_cheap, disp_ptrs_real},
      {"vtable copied in", disp_copy_cheap, disp_copy_real},
      {"vtable via pointer", disp_plain_cheap, disp_plain_real},
      {"pointer + atomic+branch", disp_atomic_cheap, disp_atomic_real},
  };
  const int n = (int)(sizeof rows / sizeof *rows);

  printf("iters=%ld reps=%d   callee_real ~ %.0f ns (spin=%d)\n\n", iters, reps,
         real_cost, callee_spin_iters);
  printf("%-26s %12s %10s   %12s %10s\n", "dispatch shape", "cheap ns", "vs floor",
         "real ns", "overhead");
  printf("%-26s %12s %10s   %12s %10s\n", "--------------------------",
         "------------", "----------", "------------", "----------");

  int (*cheap_fns[8])(int);
  int (*real_fns[8])(int);
  for (int i = 0; i < n; ++i) { cheap_fns[i] = rows[i].cheap; real_fns[i] = rows[i].real; }

  double c[8], rr[8];
  measure_all(cheap_fns, n, iters, reps, c);
  measure_all(real_fns, n, iters / 200, reps, rr);

  for (int i = 0; i < n; ++i)
    printf("%-26s %12.3f %+10.3f   %12.1f %+9.2f%%\n", rows[i].name, c[i],
           c[i] - c[0], rr[i], 100.0 * (rr[i] - rr[0]) / rr[0]);

  printf("\n'cheap ns' isolates the dispatch. 'overhead' is what it costs on a call\n"
         "that actually does something -- which is the number that decides this.\n");
  return 0;
}
