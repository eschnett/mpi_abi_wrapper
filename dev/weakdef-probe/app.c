/* Links libabi (so its strong foo/pfoo are in the process first, as an
 * application linking libmpi_abi puts ours there), then loads the wrapper the
 * way bootstrap.c does on macOS: dlopen(RTLD_LOCAL), no other isolation.
 */
#include <dlfcn.h>
#include <stdio.h>

int foo(void); /* keeps libabi a dependency of this binary */

static const char *v3(int got, int ok, int hop, int cap)
{
  if (got == ok) return "OK";
  if (got == hop) return "CAPTURED at the PMPI_X hop";
  if (got == cap) return "CAPTURED at MPI_X";
  return "??";
}

int main(void)
{
  (void)foo;
  void *h = dlopen("./wrap.dylib", RTLD_LOCAL | RTLD_LAZY);
  if (!h) {
    printf("dlopen failed: %s\n", dlerror());
    return 1;
  }
  int (*wf)(void) = (int (*)(void))dlsym(h, "wrap_foo");
  int (*wp)(void) = (int (*)(void))dlsym(h, "wrap_pfoo");
  int (*wi)(void) = (int (*)(void))dlsym(h, "wrap_internal");
  int t1 = wf(), t2 = wp(), t3 = wi();
  printf("  T1 wrapper's MPI_X call:      %3d  %s\n", t1, v3(t1, 11, 12, 2));
  printf("  T2 wrapper's PMPI_X call:     %3d  %s\n", t2,
         t2 == 1 ? "OK" : t2 == 2 ? "CAPTURED" : "??");
  printf("  T3 impl-internal MPI_X call:  %3d  %s\n", t3,
         v3(t3, 111, 112, 102));
  return 0;
}
