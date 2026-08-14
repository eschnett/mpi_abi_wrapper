#ifndef BENCH_H
#define BENCH_H

struct vtable {
  int (*cheap)(int);
  int (*real)(int);
  /* In the real thing this is 688 slots; only the offset of the one being called
   * matters to the cost, and these are at the front either way.
   */
};

struct vtable;
const struct vtable *dispatch_get_vtable(void);
void dispatch_init(void);

int disp_atomic_cheap(int), disp_atomic_real(int);
int disp_plain_cheap(int), disp_plain_real(int);
int disp_copy_cheap(int), disp_copy_real(int);
int disp_ptrs_cheap(int), disp_ptrs_real(int);
int disp_direct_cheap(int), disp_direct_real(int);

#endif
