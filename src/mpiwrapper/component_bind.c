/* glibc hides dladdr, RTLD_NEXT and Dl_info behind _GNU_SOURCE, and it has to
 * be defined before any header is included (see getvtable.c).
 */
#if defined(__linux__) && !defined(_GNU_SOURCE)
#  define _GNU_SOURCE
#endif

/* libmpiwrapper -- binding the implementation's own components.
 *
 * Hand-written and permanent. NOTES.md #2, "Components the implementation
 * loads later", has the design and dev/component-probe/ the measurement; in
 * short:
 *
 * RTLD_DEEPBIND makes the wrapper, the implementation and everything that one
 * dlopen loads resolve their MPI_* references inside that group first. It does
 * nothing for an object the implementation dlopen's *later* on its own --
 * Open MPI's MCA components, ROMIO above all, which call the public MPI_*
 * functions. Such an object is loaded without RTLD_DEEPBIND, so its undefined
 * references search the global scope first, and the only MPI_* there is
 * libmpi_abi's: HPC-X's mca_io_romio321.so does not even list libmpi.so among
 * its dependencies, and relies on libmpi being global, as it is when an
 * application links it directly. Behind the wrapper its
 * MPI_Status_set_elements_x therefore binds to libmpi_abi's (the dynamic
 * linker's own LD_DEBUG=bindings log says so), which converts a 32-byte ABI
 * status into the 24-byte Open MPI status the caller passed -- on HPC-X, a
 * smashed stack in the wrapper's own w_MPI_File_write_at_all on every rank.
 *
 * The fix has two halves, both here:
 *
 *  - libmpiwrapper defines dlopen. It is the first object of its own
 *    RTLD_DEEPBIND group, so every dlopen call made from inside the group --
 *    the implementation's, its PMIx's, its transports' -- reaches this one,
 *    and nothing outside the group does: the wrapper is loaded RTLD_LOCAL, so
 *    the application never sees the definition. It forwards to the real
 *    dlopen.
 *  - On Linux, it then rebinds every object that call newly loaded: each
 *    relocation that names a function libmpi_abi defines is pointed at the
 *    implementation's definition instead (or at the object's own, if it
 *    defines the name itself). An application's plugin, loaded through the
 *    application's own dlopen, is untouched and keeps binding to libmpi_abi,
 *    which is where its calls belong.
 *
 * What it does not cover, said in NOTES.md #13.2: a constructor of the new
 * object runs inside the real dlopen, before the rebinding, so an MPI call
 * from a component's constructor is still captured; an object another thread
 * loads while the implementation's dlopen is in progress is rebound as if the
 * implementation had loaded it; and the hook is Linux's alone. Elsewhere this
 * file is not built and the wrapper defines no dlopen: a forwarder would only
 * have changed which object the real dlopen takes for its caller, which on
 * FreeBSD decides $ORIGIN and the search path as it does on Linux.
 */

#if defined(__linux__)

#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "component_bind.h"

/* Declared by <dlfcn.h>; defined here, and exported by
 * cmake/mpiwrapper-linux.version.
 */
__attribute__((visibility("default"))) void *dlopen(const char *file, int mode);

typedef void *(*dlopen_fn)(const char *, int);

/* The real dlopen: the next definition after this library in its own lookup
 * scope, which is libc's (libdl's on glibc before 2.34).
 */
static dlopen_fn real_dlopen_fn(void)
{
  static dlopen_fn real;
  if (!real) real = (dlopen_fn)(uintptr_t)dlsym(RTLD_NEXT, "dlopen");
  return real;
}

/* Set once by mpiwrapper_component_bind_init, from libmpi_abi's constructor,
 * before anything can call an MPI function; read-only afterwards.
 */
static void *abi_handle;  /* libmpi_abi */
static void *impl_handle; /* the implementation's libmpi, and its dependencies */
static struct link_map *self_map;

void mpiwrapper_component_bind_init(const void *abi_probe,
                                    const void *impl_probe)
{
  Dl_info abi_info, impl_info, self_info;
  struct link_map *map = NULL;

  if (!abi_probe || !impl_probe) return; /* the caller opted out */
  if (!dladdr(abi_probe, &abi_info) || !dladdr(impl_probe, &impl_info)) return;
  if (!abi_info.dli_fname || !impl_info.dli_fname) return;
  if (!dladdr1((const void *)(uintptr_t)&mpiwrapper_component_bind_init,
               &self_info, (void **)&map, RTLD_DL_LINKMAP) ||
      !map)
    return;

  /* RTLD_NOLOAD: both are loaded already, and neither changes scope by being
   * named again. The handle of libmpi searches libmpi and then its
   * dependencies, which is exactly where the implementation's own definitions
   * are.
   */
  dlopen_fn real = real_dlopen_fn();
  if (!real) return;
  void *abi  = real(abi_info.dli_fname, RTLD_LAZY | RTLD_NOLOAD);
  void *impl = real(impl_info.dli_fname, RTLD_LAZY | RTLD_NOLOAD);
  if (!abi || !impl) return;
  abi_handle  = abi;
  impl_handle = impl;
  self_map    = map;
}

/* The relocation types that hold a symbol's address in a GOT or data slot:
 * the lazily bound PLT slot, the GOT entry, and an absolute word. These are
 * the architectures the project has run on (NOTES.md #9); any other gets the
 * forwarding half only, and says so in NOTES.md #13.2.
 */
#  if defined(__x86_64__)
#    define RB_RELA 1
#    define RB_JUMP_SLOT R_X86_64_JUMP_SLOT
#    define RB_GLOB_DAT R_X86_64_GLOB_DAT
#    define RB_ABS R_X86_64_64
#  elif defined(__aarch64__)
#    define RB_RELA 1
#    define RB_JUMP_SLOT R_AARCH64_JUMP_SLOT
#    define RB_GLOB_DAT R_AARCH64_GLOB_DAT
#    define RB_ABS R_AARCH64_ABS64
#  elif defined(__i386__)
#    define RB_RELA 0
#    define RB_JUMP_SLOT R_386_JMP_SLOT
#    define RB_GLOB_DAT R_386_GLOB_DAT
#    define RB_ABS R_386_32
#  elif defined(__arm__)
#    define RB_RELA 0
#    define RB_JUMP_SLOT R_ARM_JUMP_SLOT
#    define RB_GLOB_DAT R_ARM_GLOB_DAT
#    define RB_ABS R_ARM_ABS32
#  endif

#  if defined(RB_JUMP_SLOT)

#    if __SIZEOF_POINTER__ == 8
#      define RB_R_SYM(i) ELF64_R_SYM(i)
#      define RB_R_TYPE(i) ELF64_R_TYPE(i)
#    else
#      define RB_R_SYM(i) ELF32_R_SYM(i)
#      define RB_R_TYPE(i) ELF32_R_TYPE(i)
#    endif

/* A dynamic-section address. glibc relocates some of these in place and
 * leaves them alone on other targets, and either way an unrelocated one is an
 * offset below the load bias.
 */
static uintptr_t dyn_addr(const struct link_map *m, ElfW(Addr) p)
{
  return p < (ElfW(Addr))m->l_addr ? (uintptr_t)m->l_addr + p : (uintptr_t)p;
}

struct object {
  const struct link_map *map;
  const ElfW(Sym) *symtab;
  const char *strtab;
  uintptr_t relro_lo, relro_hi; /* the protected pages; empty if none */
  int relro_open;
};

static int find_relro(struct dl_phdr_info *info, size_t size, void *data)
{
  (void)size;
  struct object *o = data;
  if (info->dlpi_addr != (ElfW(Addr))o->map->l_addr) return 0;
  for (int i = 0; i < info->dlpi_phnum; ++i) {
    const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
    if (ph->p_type != PT_GNU_RELRO) continue;
    const uintptr_t page = (uintptr_t)sysconf(_SC_PAGESIZE);
    /* Rounded as the dynamic linker rounds it when it protects the range:
     * both ends down. A last partial page stays writable, and rounding the end
     * up would make whatever shares that page read-only on the way out.
     */
    uintptr_t lo = (uintptr_t)info->dlpi_addr + ph->p_vaddr;
    uintptr_t hi = lo + ph->p_memsz;
    o->relro_lo = lo & ~(page - 1);
    o->relro_hi = hi & ~(page - 1);
  }
  return 1;
}

/* A slot in RELRO is read-only by now; open the whole range once, on the
 * first write into it, and close it again when the object is done.
 */
static int store(struct object *o, uintptr_t *slot, uintptr_t value)
{
  if (*slot == value) return 1;
  const uintptr_t at = (uintptr_t)slot;
  if (at >= o->relro_lo && at < o->relro_hi && !o->relro_open) {
    if (mprotect((void *)o->relro_lo, o->relro_hi - o->relro_lo,
                 PROT_READ | PROT_WRITE) != 0)
      return 0;
    o->relro_open = 1;
  }
  *slot = value;
  return 1;
}

static void rebind_relocation(struct object *o, ElfW(Addr) r_offset,
                              ElfW(Xword) r_info, intptr_t addend)
{
  const unsigned type = (unsigned)RB_R_TYPE(r_info);
  const size_t sym    = (size_t)RB_R_SYM(r_info);
  if (sym == 0) return;
  if (type != RB_JUMP_SLOT && type != RB_GLOB_DAT && type != RB_ABS) return;

  const ElfW(Sym) *s = &o->symtab[sym];
  const char *name   = o->strtab + s->st_name;
  /* libmpi_abi defines MPI_*, PMPI_*, MPIX_* and PMPIX_*, and nothing else;
   * the prefix test keeps dlsym away from the many thousands of other
   * relocations a transport library carries.
   */
  if (!((name[0] == 'M' && name[1] == 'P' && name[2] == 'I') ||
        (name[0] == 'P' && name[1] == 'M' && name[2] == 'P' && name[3] == 'I')))
    return;

  void *abi = dlsym(abi_handle, name);
  if (!abi) return; /* not a name libmpi_abi could capture */

  /* The object's own definition, if it has one -- a reference to it is just
   * as interposable -- else the implementation's. Neither: leave the slot
   * alone; libmpi_abi's answer is wrong, but no answer is worse.
   */
  uintptr_t target;
  if (s->st_shndx != SHN_UNDEF && s->st_value != 0) {
    target = (uintptr_t)o->map->l_addr + s->st_value;
  } else {
    void *impl = dlsym(impl_handle, name);
    if (!impl || impl == abi) return;
    target = (uintptr_t)impl;
  }

  uintptr_t *slot = (uintptr_t *)((uintptr_t)o->map->l_addr + r_offset);
  if (type == RB_JUMP_SLOT) {
    /* Lazily bound slots still hold the PLT's resolver path, which would bind
     * to libmpi_abi on the first call; bind them now.
     */
    store(o, slot, target + (uintptr_t)addend);
  } else if (RB_RELA) {
    if (*slot == (uintptr_t)abi + (uintptr_t)addend)
      store(o, slot, target + (uintptr_t)addend);
  } else {
    /* REL: the addend was the slot's content before relocation, so what is
     * there now is abi plus it.
     */
    const uintptr_t offset = *slot - (uintptr_t)abi;
    if (offset < 4096) store(o, slot, target + offset);
  }
}

static void rebind_object(const struct link_map *m)
{
  struct object o = {.map = m};
  uintptr_t rel = 0, relsz = 0, jmprel = 0, pltrelsz = 0;
  ElfW(Sxword) pltrel = 0;

  for (const ElfW(Dyn) *d = m->l_ld; d && d->d_tag != DT_NULL; ++d) {
    switch (d->d_tag) {
    case DT_SYMTAB: o.symtab = (const ElfW(Sym) *)dyn_addr(m, d->d_un.d_ptr); break;
    case DT_STRTAB: o.strtab = (const char *)dyn_addr(m, d->d_un.d_ptr); break;
#    if RB_RELA
    case DT_RELA: rel = dyn_addr(m, d->d_un.d_ptr); break;
    case DT_RELASZ: relsz = d->d_un.d_val; break;
#    else
    case DT_REL: rel = dyn_addr(m, d->d_un.d_ptr); break;
    case DT_RELSZ: relsz = d->d_un.d_val; break;
#    endif
    case DT_JMPREL: jmprel = dyn_addr(m, d->d_un.d_ptr); break;
    case DT_PLTRELSZ: pltrelsz = d->d_un.d_val; break;
    case DT_PLTREL: pltrel = (ElfW(Sxword))d->d_un.d_val; break;
    default: break;
    }
  }
  if (!o.symtab || !o.strtab) return;
  dl_iterate_phdr(find_relro, &o);

#    if RB_RELA
  typedef ElfW(Rela) reloc;
  const ElfW(Sxword) kind = DT_RELA;
#      define RB_ADDEND(r) ((intptr_t)(r)->r_addend)
#    else
  typedef ElfW(Rel) reloc;
  const ElfW(Sxword) kind = DT_REL;
#      define RB_ADDEND(r) ((intptr_t)0)
#    endif

  for (const reloc *r = (const reloc *)rel; r && r < (const reloc *)(rel + relsz); ++r)
    rebind_relocation(&o, r->r_offset, r->r_info, RB_ADDEND(r));
  if (pltrel == kind)
    for (const reloc *r = (const reloc *)jmprel;
         r && r < (const reloc *)(jmprel + pltrelsz); ++r)
      rebind_relocation(&o, r->r_offset, r->r_info, RB_ADDEND(r));

  if (o.relro_open)
    mprotect((void *)o.relro_lo, o.relro_hi - o.relro_lo, PROT_READ);
}

#  endif /* RB_JUMP_SLOT */

/* glibc's dlopen takes its caller from its own return address, and the caller
 * decides two things: where a bare name is searched for (its DT_RPATH and
 * DT_RUNPATH, NOTES.md #2), and what $ORIGIN in a name means. Called from
 * here, the caller would be this library. So both are done here on the real
 * caller's behalf, and what reaches the real dlopen is a name that means the
 * same whoever calls it: an absolute path, or a bare name that the cache or the
 * default directories will find for anyone.
 */
static const struct link_map *map_of(const void *address)
{
  Dl_info info;
  struct link_map *map = NULL;
  if (!dladdr1(address, &info, (void **)&map, RTLD_DL_LINKMAP)) return NULL;
  return map;
}

/* $ORIGIN and ${ORIGIN}, against the caller's directory. $LIB and $PLATFORM
 * mean the same to every caller, so the real dlopen expands those itself.
 */
static const char *expand_origin(const char *file, const struct link_map *caller,
                                 char *buf, size_t size)
{
  if (!strstr(file, "$ORIGIN") && !strstr(file, "${ORIGIN}")) return file;
  if (!caller || !caller->l_name) return file;
  /* The main program's map has an empty name; its origin is where the kernel
   * says the executable is, which is what the dynamic linker uses too.
   */
  char exe[4096];
  const char *name = caller->l_name;
  if (name[0] == '\0') {
    const ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n <= 0) return file;
    exe[n] = '\0';
    name = exe;
  }
  if (name[0] != '/') return file;
  const char *slash = strrchr(name, '/');
  const size_t dirlen = (size_t)(slash - name);

  size_t n = 0;
  for (const char *p = file; *p;) {
    size_t skip = 0;
    if (strncmp(p, "${ORIGIN}", 9) == 0) skip = 9;
    else if (strncmp(p, "$ORIGIN", 7) == 0) skip = 7;
    if (skip) {
      if (n + dirlen >= size) return file;
      memcpy(buf + n, name, dirlen);
      n += dirlen;
      p += skip;
    } else {
      if (n + 1 >= size) return file;
      buf[n++] = *p++;
    }
  }
  buf[n] = '\0';
  return buf;
}

static void *open_bare(dlopen_fn real, const char *file, int mode,
                       const struct link_map *caller)
{
  /* Already loaded under that name: the real dlopen finds it by name for any
   * caller, and searching directories first could load a second copy.
   */
  void *loaded = real(file, (mode & (RTLD_LAZY | RTLD_NOW)) | RTLD_NOLOAD);
  if (loaded) {
    void *handle = real(file, mode);
    dlclose(loaded);
    return handle;
  }
  if (caller) {
    Dl_serinfo probe;
    if (dlinfo((void *)(uintptr_t)caller, RTLD_DI_SERINFOSIZE, &probe) == 0 &&
        probe.dls_size > 0) {
      Dl_serinfo *info = malloc(probe.dls_size);
      if (info) {
        info->dls_size = probe.dls_size;
        info->dls_cnt  = probe.dls_cnt;
        if (dlinfo((void *)(uintptr_t)caller, RTLD_DI_SERINFO, info) == 0) {
          for (unsigned i = 0; i < info->dls_cnt; ++i) {
            char path[4096];
            const int len = snprintf(path, sizeof path, "%s/%s",
                                     info->dls_serpath[i].dls_name, file);
            if (len <= 0 || (size_t)len >= sizeof path) continue;
            if (access(path, F_OK) != 0) continue;
            void *handle = real(path, mode);
            if (handle) {
              free(info);
              return handle;
            }
          }
        }
        free(info);
      }
    }
  }
  return real(file, mode);
}

static void *open_as_caller(dlopen_fn real, const char *file, int mode,
                            const void *return_address)
{
  if (!file) return real(file, mode);
  const struct link_map *caller = map_of(return_address);
  char expanded[4096];
  file = expand_origin(file, caller, expanded, sizeof expanded);
  if (strchr(file, '/')) return real(file, mode);
  return open_bare(real, file, mode, caller);
}

void *dlopen(const char *file, int mode)
{
  dlopen_fn real = real_dlopen_fn();
  if (!real) return NULL;
  const void *caller = __builtin_return_address(0);
#  if defined(RB_JUMP_SLOT)
  if (!self_map) return open_as_caller(real, file, mode, caller);

  /* A new object is appended to its namespace's chain, which is ours. */
  const struct link_map *tail = self_map;
  while (tail->l_next) tail = tail->l_next;

  void *handle = open_as_caller(real, file, mode, caller);
  if (handle)
    for (const struct link_map *m = tail->l_next; m; m = m->l_next)
      rebind_object(m);
  return handle;
#  else
  return open_as_caller(real, file, mode, caller);
#  endif
}

#endif /* __linux__ */
