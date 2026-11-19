/* After MPI_Init through the wrapper: where does every loaded object's dlopen
 * slot point? Prints object -> library holding the slot's target, and the
 * symbol. An ordinary application of the ABI header (x86_64/aarch64, RELA):
 *
 *     cc -I<build>/gen/include dlopen_slots.c -L<build> -lmpi_abi -ldl
 *     mpiexec -n 1 ./a.out | sort | uniq
 *
 * It is what showed UCX's libucm rewriting every slot (README.md). */
#define _GNU_SOURCE
#include <mpi.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uintptr_t da(struct link_map *m, ElfW(Addr) p) { return p < m->l_addr ? m->l_addr + p : p; }
static void scan(struct link_map *m) {
  const ElfW(Sym) *sym = 0; const char *str = 0; uintptr_t rela = 0, relasz = 0, jmp = 0, jmpsz = 0;
  for (ElfW(Dyn) *d = m->l_ld; d && d->d_tag != DT_NULL; ++d) switch (d->d_tag) {
    case DT_SYMTAB: sym = (void *)da(m, d->d_un.d_ptr); break;
    case DT_STRTAB: str = (void *)da(m, d->d_un.d_ptr); break;
    case DT_RELA: rela = da(m, d->d_un.d_ptr); break;
    case DT_RELASZ: relasz = d->d_un.d_val; break;
    case DT_JMPREL: jmp = da(m, d->d_un.d_ptr); break;
    case DT_PLTRELSZ: jmpsz = d->d_un.d_val; break; }
  if (!sym || !str) return;
  uintptr_t ranges[2][2] = {{rela, relasz}, {jmp, jmpsz}};
  for (int k = 0; k < 2; ++k)
    for (ElfW(Rela) *r = (void *)ranges[k][0]; r && (uintptr_t)r < ranges[k][0] + ranges[k][1]; ++r) {
      size_t i = ELF64_R_SYM(r->r_info); if (!i) continue;
      if (strcmp(str + sym[i].st_name, "dlopen") || sym[i].st_shndx != SHN_UNDEF) continue;
      void *v = *(void **)(m->l_addr + r->r_offset); Dl_info di;
      printf("%-28s -> %s %s\n", strrchr(m->l_name, '/') ? strrchr(m->l_name, '/') + 1 : "(main)",
             dladdr(v, &di) && di.dli_fname ? strrchr(di.dli_fname, '/') + 1 : "?",
             dladdr(v, &di) && di.dli_sname ? di.dli_sname : "");
    }
}
int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  struct link_map *m = 0; dlinfo(dlopen(NULL, RTLD_LAZY), RTLD_DI_LINKMAP, &m);
  for (; m; m = m->l_next) scan(m);
  MPI_Finalize(); return 0;
}
