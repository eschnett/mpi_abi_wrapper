/* Is a sorted array with binary or interpolation search faster than a hash table
 * for the implementation -> ABI predefined handle map?
 *
 * This map is on the hot path: it runs on every out-handle and inside every
 * user-op trampoline, and the datatype class is the big one. So the question is
 * worth answering rather than assuming.
 *
 * Two realistic key distributions, because they behave very differently:
 *   mpich  -- the 77 real predefined MPI_Datatype values out of MPICH's header,
 *             which are 0x0c000000..0x8c000004: a kind field in the high bits and
 *             dense low bits, i.e. severely non-uniform.
 *   ompi   -- Open MPI's are addresses of static objects, modelled as a base plus a
 *             fixed stride, i.e. nearly uniform but hugely offset.
 *
 * Two access patterns, because branch prediction dominates one of the candidates:
 *   hot    -- the same key every time (what MPI_Send with one datatype does)
 *   sweep  -- round-robin over all keys (worst case for a predictor)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NKEYS 77

static uint64_t keys_mpich[NKEYS] = {
#include "mpich_keys.inc"
};
static uint64_t keys_ompi[NKEYS];

static uint32_t values[NKEYS]; /* the ABI handle each key maps to */

/* ------------------------------------------------------------ open addressing */

#define HSLOTS 256 /* 3.3x load factor headroom */
static struct {
  uint64_t key;
  uint32_t val;
  uint32_t used;
} htab[HSLOTS];

static size_t hmix(uint64_t k)
{
  k *= 0x9e3779b97f4a7c15u;
  return (size_t)(k >> 56); /* top 8 bits -> 256 slots */
}

static void h_build(const uint64_t *keys)
{
  memset(htab, 0, sizeof htab);
  for (int i = 0; i < NKEYS; ++i) {
    size_t s = hmix(keys[i]) & (HSLOTS - 1);
    while (htab[s].used) s = (s + 1) & (HSLOTS - 1);
    htab[s].key = keys[i];
    htab[s].val = values[i];
    htab[s].used = 1;
  }
}

static uint32_t h_lookup(uint64_t k)
{
  size_t s = hmix(k) & (HSLOTS - 1);
  while (htab[s].used) {
    if (htab[s].key == k) return htab[s].val;
    s = (s + 1) & (HSLOTS - 1);
  }
  return 0;
}

/* --------------------------------------------------------------- perfect hash */

/* The key set is fully known at initialization, so a multiplier can simply be
 * searched for until no two keys collide. Then a lookup is one load and one
 * compare, with no probe loop and no loop-carried branch at all.
 */
static struct {
  uint64_t key;
  uint32_t val;
} ptab[HSLOTS];
static uint64_t pmul;
static int      pshift;

static int p_build(const uint64_t *keys)
{
  for (uint64_t m = 0x9e3779b97f4a7c15u; m != 0; m += 0x2545f4914f6cdd1du) {
    memset(ptab, 0, sizeof ptab);
    int ok = 1;
    for (int i = 0; i < NKEYS && ok; ++i) {
      size_t s = (size_t)((keys[i] * m) >> 56);
      if (ptab[s].key) ok = 0;
      else { ptab[s].key = keys[i]; ptab[s].val = values[i]; }
    }
    if (ok) { pmul = m; pshift = 56; return 1; }
  }
  return 0;
}

static uint32_t p_lookup(uint64_t k)
{
  const size_t s = (size_t)((k * pmul) >> pshift);
  return ptab[s].key == k ? ptab[s].val : 0;
}

/* --------------------------------------------------------------- sorted array */

static uint64_t sk[NKEYS];
static uint32_t sv[NKEYS];

static int cmp64(const void *a, const void *b)
{
  const uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
  return x < y ? -1 : x > y ? 1 : 0;
}

static void s_build(const uint64_t *keys)
{
  uint64_t idx[NKEYS];
  memcpy(idx, keys, sizeof idx);
  qsort(idx, NKEYS, sizeof idx[0], cmp64);
  for (int i = 0; i < NKEYS; ++i) {
    sk[i] = idx[i];
    for (int j = 0; j < NKEYS; ++j)
      if (keys[j] == idx[i]) { sv[i] = values[j]; break; }
  }
}

static uint32_t bsearch_lookup(uint64_t k)
{
  int lo = 0, hi = NKEYS - 1;
  while (lo <= hi) {
    const int mid = (lo + hi) >> 1;
    if (sk[mid] == k) return sv[mid];
    if (sk[mid] < k) lo = mid + 1;
    else hi = mid - 1;
  }
  return 0;
}

/* Interpolation search: O(log log n) *if the keys are uniformly distributed*. */
static uint32_t isearch_lookup(uint64_t k)
{
  int lo = 0, hi = NKEYS - 1;
  while (lo <= hi && k >= sk[lo] && k <= sk[hi]) {
    if (sk[hi] == sk[lo]) break;
    const int pos =
        lo + (int)(((double)(k - sk[lo]) / (double)(sk[hi] - sk[lo])) * (hi - lo));
    if (sk[pos] == k) return sv[pos];
    if (sk[pos] < k) lo = pos + 1;
    else hi = pos - 1;
  }
  return 0;
}

/* ------------------------------------------------------------------- harness */

static double now_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static volatile uint32_t sink;

static double burst(uint32_t (*fn)(uint64_t), const uint64_t *probe, long n)
{
  const double t0 = now_ns();
  uint32_t     acc = 0;
  for (long i = 0; i < n; ++i) acc += fn(probe[i & 1023]);
  const double t1 = now_ns();
  sink             = acc;
  return (t1 - t0) / (double)n;
}

struct cand {
  const char *name;
  uint32_t (*fn)(uint64_t);
};

int main(int argc, char **argv)
{
  const long iters = argc > 1 ? atol(argv[1]) : 20000000L;
  const int  reps  = argc > 2 ? atoi(argv[2]) : 7;

  for (int i = 0; i < NKEYS; ++i) values[i] = 0x200u + (uint32_t)i;
  /* Open MPI-like: addresses of consecutive static objects. */
  for (int i = 0; i < NKEYS; ++i)
    keys_ompi[i] = 0x7f9c00012340u + (uint64_t)i * 512u;

  const struct cand cands[] = {
      {"perfect hash", p_lookup},
      {"open-addressing hash", h_lookup},
      {"sorted + binary search", bsearch_lookup},
      {"sorted + interpolation", isearch_lookup},
  };
  const int nc = (int)(sizeof cands / sizeof *cands);

  printf("%d keys per class, %ld iters, best of %d interleaved bursts\n\n", NKEYS,
         iters, reps);
  printf("%-24s %10s %10s   %10s %10s\n", "", "mpich hot", "mpich sweep",
         "ompi hot", "ompi sweep");
  printf("%-24s %10s %10s   %10s %10s\n", "------------------------", "---------",
         "-----------", "---------", "----------");

  double res[8][4];
  for (int d = 0; d < 2; ++d) {
    const uint64_t *keys = d == 0 ? keys_mpich : keys_ompi;
    h_build(keys);
    s_build(keys);
    if (!p_build(keys)) { printf("no perfect hash found\n"); return 1; }

    uint64_t hot[1024], sweep[1024];
    for (int i = 0; i < 1024; ++i) {
      hot[i]   = keys[9]; /* MPI_INT-ish: one datatype used over and over */
      sweep[i] = keys[i % NKEYS];
    }

    for (int c = 0; c < nc; ++c) {
      double bh = 1e300, bs = 1e300;
      for (int r = 0; r < reps; ++r) {
        const double h = burst(cands[c].fn, hot, iters);
        const double s = burst(cands[c].fn, sweep, iters);
        if (h < bh) bh = h;
        if (s < bs) bs = s;
      }
      res[c][d * 2 + 0] = bh;
      res[c][d * 2 + 1] = bs;
    }
  }

  for (int c = 0; c < nc; ++c)
    printf("%-24s %10.3f %10.3f   %10.3f %10.3f\n", cands[c].name, res[c][0],
           res[c][1], res[c][2], res[c][3]);
  printf("\nns per lookup. 'hot' = same key every time; 'sweep' = round-robin.\n");
  return 0;
}
