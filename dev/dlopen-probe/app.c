/* The normal case: an executable linked directly against libabi.
 *
 * T1  does the wrapper's own MPI_Send call reach libimpl?
 * T2  does the *implementation's internal* MPI_Send call reach libimpl?
 *
 * T2 is the one RTLD_DEEPBIND may not cover, since libimpl is a dependency loaded
 * by the same dlopen rather than the object dlopen was called on.
 */

#include "probe.h"

#include <stdio.h>
#include <stdlib.h>

extern int MPI_Send(const char *msg);

/* Each test has its own correct answer; a bare "libimpl was reached" is not it. */
static const char *verdict(int rc, int want)
{
  if (rc == want) return "OK";
  switch (rc) {
  case IMPL_REACHED:  return "BYPASSED";  /* reached libimpl without going through us */
  case ABI_CAPTURED:  return "CAPTURED";  /* recursion: our MPI_Send was re-entered */
  case ABI_FORWARDED: return "FORWARDED"; /* went through us when it should not have */
  default:            return "ERROR";
  }
}

int main(int argc, char **argv)
{
  const char *mode = argc > 1 ? argv[1] : "local";

  fprintf(stderr, "== app (linked against libabi), mode=%s ==\n", mode);

  const char *err = abi_init(mode);
  if (err) {
    fprintf(stderr, "  abi_init failed: %s\n", err);
    printf("RESULT %s T1 SKIP\nRESULT %s T2 SKIP\n", mode, mode);
    return 0;
  }

  abi_report_resolution();

  fprintf(stderr, "  -- T1: app -> MPI_Send --\n");
  const int t1 = MPI_Send("t1");

  fprintf(stderr, "  -- T2: implementation's own internal MPI_Send call --\n");
  const int t2 = abi_test_impl_internal();

  /* T1: the wrapper must reach libimpl via us.  T2: the implementation's own
   * internal call must reach libimpl *directly*, never coming back through us. */
  printf("RESULT %s T1 %s\n", mode, verdict(t1, ABI_FORWARDED));
  printf("RESULT %s T2 %s\n", mode, verdict(t2, IMPL_REACHED));
  return 0;
}
