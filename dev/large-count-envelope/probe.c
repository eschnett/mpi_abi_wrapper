/* Is a datatype's envelope a property of the *type* or of the *constructor*?
 *
 * MPI-4.0 gave every datatype constructor a large-count twin and gave
 * MPI_TYPE_GET_ENVELOPE a fourth count, `num_large_counts`, with
 * MPI_TYPE_GET_CONTENTS_C gaining the matching `array_of_large_counts`. The
 * question this probe exists to answer is which of two readings is right:
 *
 *   (a) the envelope describes the *type*, so a count that fits in an `int`
 *       lands in array_of_integers however the type was built, and
 *       num_large_counts is nonzero only for a value that does not fit; or
 *   (b) the envelope describes the *constructor*, so a type built by
 *       MPI_Type_contiguous_c reports its count in array_of_large_counts even
 *       when the count is 5, and one built by MPI_Type_contiguous never does.
 *
 * It decides whether this project's narrowing fallback is observable. Where an
 * implementation has no `_c` constructors we serve MPI_Type_contiguous_c by
 * calling MPI_Type_contiguous with the count narrowed to an `int`. Under (a)
 * that is indistinguishable from a native large-count implementation. Under
 * (b) it is not: the caller built the type through the large-count API and
 * would see its count come back from the small half of the envelope, and
 * MPI_Type_get_envelope_c's own fallback -- which reports num_large_counts = 0
 * because it can only ask the small envelope -- would then be wrong for a
 * reason no guard can detect.
 *
 * Ask MPICH, which has both halves. Open MPI 5.0.x has neither and cannot
 * answer.
 *
 *   cc -I$MPI/include -L$MPI/lib -o probe probe.c -lmpi   # add -lpmpi for MPICH
 *   ./probe
 */

#include <mpi.h>

#include <limits.h>
#include <stdio.h>

#if !defined(MPI_VERSION) || MPI_VERSION < 4
#error "this probe needs an implementation with the large-count constructors"
#endif

static const char *combiner_name(int combiner)
{
  if (combiner == MPI_COMBINER_CONTIGUOUS) return "CONTIGUOUS";
  if (combiner == MPI_COMBINER_STRUCT) return "STRUCT";
  if (combiner == MPI_COMBINER_VECTOR) return "VECTOR";
  if (combiner == MPI_COMBINER_NAMED) return "NAMED";
  return "?";
}

/* The envelope both ways, then the contents both ways, for one datatype. The
 * two get_contents calls are the second half of the question: even if the
 * envelope answers alike, the *values* have to come back from the same array.
 */
static void report(const char *how, MPI_Datatype t)
{
  MPI_Count ni_c = -1, na_c = -1, nl_c = -1, nd_c = -1;
  int       combiner_c = -1;
  const int e_c =
      MPI_Type_get_envelope_c(t, &ni_c, &na_c, &nl_c, &nd_c, &combiner_c);

  int ni = -1, na = -1, nd = -1, combiner = -1;
  const int e = MPI_Type_get_envelope(t, &ni, &na, &nd, &combiner);

  printf("%s\n", how);
  printf("  envelope_c -> %d: %lld integers, %lld addresses, %lld large counts,"
         " %lld datatypes, combiner %s\n",
         e_c, (long long)ni_c, (long long)na_c, (long long)nl_c,
         (long long)nd_c, combiner_name(combiner_c));
  printf("  envelope   -> %d: %d integers, %d addresses, %d datatypes,"
         " combiner %s\n",
         e, ni, na, nd, combiner_name(combiner));

  /* When the small envelope refuses, the *class* is the part worth recording:
   * it says whether the implementation is calling this a bad argument or a bad
   * datatype, and a wrapper emulating the large form has to answer alike.
   */
  if (e != MPI_SUCCESS) {
    int  class = -1, len = 0;
    char text[MPI_MAX_ERROR_STRING] = "";
    MPI_Error_class(e, &class);
    MPI_Error_string(e, text, &len);
    printf("               class %d (MPI_ERR_ARG is %d, MPI_ERR_TYPE is %d):"
           " %s\n",
           class, MPI_ERR_ARG, MPI_ERR_TYPE, text);
  }

  /* Sized from the envelope's own counts, never larger: dev/get-contents-extent
   * measured Open MPI 5.0.6 dereferencing the whole of max_datatypes.
   */
  int          gi[16];
  MPI_Aint     ga[16];
  MPI_Count    gl[16];
  MPI_Datatype gd[16];
  for (int i = 0; i < 16; ++i) {
    gi[i] = -1;
    ga[i] = -1;
    gl[i] = -1;
    gd[i] = MPI_DATATYPE_NULL;
  }

  if (e_c == MPI_SUCCESS && ni_c <= 16 && na_c <= 16 && nl_c <= 16 &&
      nd_c <= 16) {
    const int r =
        MPI_Type_get_contents_c(t, ni_c, na_c, nl_c, nd_c, gi, ga, gl, gd);
    printf("  contents_c -> %d:", r);
    for (MPI_Count i = 0; i < ni_c; ++i) printf(" int[%lld]=%d", (long long)i, gi[i]);
    for (MPI_Count i = 0; i < nl_c; ++i)
      printf(" large[%lld]=%lld", (long long)i, (long long)gl[i]);
    printf("\n");
  }

  /* The small form on the same type. MPI-5.0 5.1.13 makes this erroneous when
   * a value does not fit, but every value here does, so a refusal would be a
   * statement about the *constructor* rather than about the values -- which is
   * exactly reading (b).
   */
  for (int i = 0; i < 16; ++i) gi[i] = -1;
  if (e == MPI_SUCCESS && ni <= 16 && na <= 16 && nd <= 16) {
    const int r = MPI_Type_get_contents(t, ni, na, nd, gi, ga, gd);
    printf("  contents   -> %d:", r);
    for (int i = 0; i < ni; ++i) printf(" int[%d]=%d", i, gi[i]);
    printf("\n");
  }
  printf("\n");
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* Errors returned rather than aborting: a refusal is a result here. */
  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  /* 1. The same contiguous type, built each way, with a count of 5 -- a value
   *    that fits in an int with room to spare, so nothing about its magnitude
   *    can force it into the large half.
   */
  MPI_Datatype small_contig = MPI_DATATYPE_NULL;
  MPI_Type_contiguous(5, MPI_INT, &small_contig);
  MPI_Type_commit(&small_contig);
  report("MPI_Type_contiguous(5, MPI_INT)", small_contig);

  MPI_Datatype large_contig = MPI_DATATYPE_NULL;
  MPI_Type_contiguous_c(5, MPI_INT, &large_contig);
  MPI_Type_commit(&large_contig);
  report("MPI_Type_contiguous_c(5, MPI_INT)", large_contig);

  /* 2. A struct, which carries a blocklength array as well as a count, so it
   *    shows whether the split is per-argument or whole-envelope.
   */
  const int      sbl[2]  = {1, 1};
  const MPI_Aint sdis[2] = {0, (MPI_Aint)sizeof(int)};
  MPI_Datatype   stypes[2] = {MPI_INT, MPI_DOUBLE};

  MPI_Datatype small_struct = MPI_DATATYPE_NULL;
  MPI_Type_create_struct(2, sbl, sdis, stypes, &small_struct);
  MPI_Type_commit(&small_struct);
  report("MPI_Type_create_struct(2, ...)", small_struct);

  const MPI_Count lbl[2]  = {1, 1};
  const MPI_Count ldis[2] = {0, (MPI_Count)sizeof(int)};
  MPI_Datatype    large_struct = MPI_DATATYPE_NULL;
  MPI_Type_create_struct_c(2, lbl, ldis, stypes, &large_struct);
  MPI_Type_commit(&large_struct);
  report("MPI_Type_create_struct_c(2, ...)", large_struct);

  /* 3. A count that genuinely does not fit in an int. Nothing this project
   *    does can serve this over a small-count implementation; it is here to
   *    show what the large half looks like when it is unavoidable, so the
   *    rows above can be read against it.
   */
  const MPI_Count huge = (MPI_Count)INT_MAX + 7;
  MPI_Datatype    huge_contig = MPI_DATATYPE_NULL;
  const int       he = MPI_Type_contiguous_c(huge, MPI_BYTE, &huge_contig);
  if (he == MPI_SUCCESS) {
    MPI_Type_commit(&huge_contig);
    report("MPI_Type_contiguous_c(INT_MAX + 7, MPI_BYTE)", huge_contig);
    MPI_Type_free(&huge_contig);
  } else {
    printf("MPI_Type_contiguous_c(INT_MAX + 7, MPI_BYTE) -> %d, not built\n\n",
           he);
  }

  MPI_Type_free(&small_contig);
  MPI_Type_free(&large_contig);
  MPI_Type_free(&small_struct);
  MPI_Type_free(&large_struct);

  MPI_Finalize();
  return 0;
}
