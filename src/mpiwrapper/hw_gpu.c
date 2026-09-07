/* libmpiwrapper -- the five GPU-support queries (NOTES.md #7 decision 28,
 * #8's reason 1).
 *
 * These are the one group here that is not in MPI-5.0 at all. Applications
 * ask their MPI whether it is GPU-aware through a vendor extension, and the
 * two implementations this project is developed against spell that question
 * differently: MPICH 4.3.x declares MPIX_Query_cuda_support,
 * MPIX_Query_hip_support, MPIX_Query_ze_support, the enum form
 * MPIX_GPU_query_support and its three kind constants in its own mpi.h, with
 * PMPIX_ twins; Open MPI 5.0.x declares MPIX_Query_cuda_support and
 * MPIX_Query_rocm_support in <mpi-ext.h> and no PMPIX_ anything. The ABI
 * header declares the union, so a program built here can ask whichever it
 * knows about, and this file answers all five over whatever the wrapped MPI
 * happens to have.
 *
 * **Absent means 0, and that is the judgement that makes these hand-written**
 * rather than generated. Decision 6 says an entry point the implementation
 * lacks answers MPI_ERR_UNSUPPORTED_OPERATION, and that is the right answer
 * where the caller asked for work to be done. It is the wrong answer here:
 * "is this MPI CUDA-aware?" has a true answer for an MPI that cannot be
 * asked, and the answer is no. An application that got 55 back from a query
 * returning a boolean would read it as *yes* (55 is not zero). So the guard
 * chain below ends in a constant 0 rather than in decision 6's stub, and
 * gen/report.txt names the group with that reason.
 *
 * **rocm and hip are one question under two names**, which is why five ABI
 * entry points reach four helpers. AMD's ROCm stack is what HIP runs on;
 * MPICH spells it hip and Open MPI spells it rocm, and no implementation has
 * been seen to distinguish them. Both ABI names therefore call the same
 * helper, and test/abi_tools_test.c asserts they agree -- a body that
 * forwarded each name to its own literal spelling would answer 1 and 0 on an
 * implementation that has only one of them, which is the plausible-but-wrong
 * body this check exists for.
 *
 * **The enum form is composed, never forwarded.** MPIX_GPU_query_support
 * switches on the *ABI's* MPIABIX_GPU_SUPPORT_* value and calls the kind's
 * helper; the ABI's enum value never crosses to the implementation. Two
 * things follow. There is no constant map for this family in constants.c --
 * nothing to keep in step, and no case that could silently pass an unmapped
 * value through (NOTES.md #5.6). And an implementation that has only the enum
 * form, or only the single forms, is served identically, because composition
 * runs in the direction the guards already answer.
 *
 * An unknown gpu_type answers MPI_ERR_ARG with *is_supported left 0, which is
 * MPICH's own answer (src/mpi/misc/gpu_query.c, `**badgputype`) rather than
 * an invention here.
 *
 * **No initialization check.** Both implementations answer from state that
 * MPI_Init establishes -- Open MPI's reads the accelerator component selected
 * there, so before init it answers 0 -- and the wrapper forwards what they
 * say rather than second-guessing when they may be asked.
 */

#include "internal.h"

/* Open MPI's two declarations live here rather than in its mpi.h, and this is
 * the only file that needs them. The guard is written by dev/probe_impl.py,
 * which compiles `#include <mpi.h>` / `#include <mpi-ext.h>` and asks the
 * compiler -- not `__has_include`, because a stray mpi-ext.h belonging to a
 * different MPI on the include path *exists* without compiling against this
 * one (HISTORY.md #1.26). Included after internal.h on purpose: the
 * implementation's <mpi.h> is what defines the OMPI_DECLSPEC that this header
 * is written in terms of.
 */
#ifdef MPIWRAPPER_HAVE_MPI_EXT_H
#  include <mpi-ext.h>
#endif

/* One target per kind and per profiling side, chosen by the probe's guards and
 * nothing else (decision 6's rule about what an #ifdef may test). A kind the
 * implementation cannot be asked about resolves to NULL on both counts, which
 * `ask` below turns into 0.
 *
 * The PMPIX_ side falls back to the MPIX_ spelling, and that is decision 7
 * applied where it can be rather than abandoned: the decision's premise is
 * that both names always exist, which holds for the standard's entry points
 * and does not hold for an extension -- Open MPI has no PMPIX_ twin of
 * either of its two. Calling the unshifted name from the PMPI_ side is worse
 * than calling the shifted one and better than answering 0, because the
 * question is a query with no side effects and nothing interposes on it.
 */

/* --- CUDA --------------------------------------------------------------- */
#ifdef MPIWRAPPER_HAVE_MPIX_Query_cuda_support
#  define W_CUDA_SINGLE   MPIX_Query_cuda_support
#else
#  define W_CUDA_SINGLE   NULL
#endif
#ifdef MPIWRAPPER_HAVE_PMPIX_Query_cuda_support
#  define W_CUDA_PSINGLE  PMPIX_Query_cuda_support
#elif defined MPIWRAPPER_HAVE_MPIX_Query_cuda_support
#  define W_CUDA_PSINGLE  MPIX_Query_cuda_support
#else
#  define W_CUDA_PSINGLE  NULL
#endif

/* --- ROCm/HIP: one question, and rocm is asked first because it is the name
 * of the stack rather than of the language ------------------------------- */
#if defined MPIWRAPPER_HAVE_MPIX_Query_rocm_support
#  define W_ROCM_SINGLE   MPIX_Query_rocm_support
#elif defined MPIWRAPPER_HAVE_MPIX_Query_hip_support
#  define W_ROCM_SINGLE   MPIX_Query_hip_support
#else
#  define W_ROCM_SINGLE   NULL
#endif
#if defined MPIWRAPPER_HAVE_PMPIX_Query_rocm_support
#  define W_ROCM_PSINGLE  PMPIX_Query_rocm_support
#elif defined MPIWRAPPER_HAVE_PMPIX_Query_hip_support
#  define W_ROCM_PSINGLE  PMPIX_Query_hip_support
#elif defined MPIWRAPPER_HAVE_MPIX_Query_rocm_support
#  define W_ROCM_PSINGLE  MPIX_Query_rocm_support
#elif defined MPIWRAPPER_HAVE_MPIX_Query_hip_support
#  define W_ROCM_PSINGLE  MPIX_Query_hip_support
#else
#  define W_ROCM_PSINGLE  NULL
#endif

/* --- Level Zero --------------------------------------------------------- */
#ifdef MPIWRAPPER_HAVE_MPIX_Query_ze_support
#  define W_ZE_SINGLE     MPIX_Query_ze_support
#else
#  define W_ZE_SINGLE     NULL
#endif
#ifdef MPIWRAPPER_HAVE_PMPIX_Query_ze_support
#  define W_ZE_PSINGLE    PMPIX_Query_ze_support
#elif defined MPIWRAPPER_HAVE_MPIX_Query_ze_support
#  define W_ZE_PSINGLE    MPIX_Query_ze_support
#else
#  define W_ZE_PSINGLE    NULL
#endif

/* --- The implementation's own enum form, where it has one. Each kind needs
 * both the function and the implementation's constant naming that kind, since
 * the ABI's value is never the one passed. --------------------------------- */
#if defined MPIWRAPPER_HAVE_MPIX_GPU_query_support && \
    defined MPIWRAPPER_HAVE_MPIX_GPU_SUPPORT_CUDA
#  define W_CUDA_BYENUM   MPIX_GPU_query_support
#  define W_CUDA_KIND     MPIX_GPU_SUPPORT_CUDA
#else
#  define W_CUDA_BYENUM   NULL
#  define W_CUDA_KIND     0
#endif
#if defined MPIWRAPPER_HAVE_MPIX_GPU_query_support && \
    defined MPIWRAPPER_HAVE_MPIX_GPU_SUPPORT_HIP
#  define W_ROCM_BYENUM   MPIX_GPU_query_support
#  define W_ROCM_KIND     MPIX_GPU_SUPPORT_HIP
#else
#  define W_ROCM_BYENUM   NULL
#  define W_ROCM_KIND     0
#endif
#if defined MPIWRAPPER_HAVE_MPIX_GPU_query_support && \
    defined MPIWRAPPER_HAVE_MPIX_GPU_SUPPORT_ZE
#  define W_ZE_BYENUM     MPIX_GPU_query_support
#  define W_ZE_KIND       MPIX_GPU_SUPPORT_ZE
#else
#  define W_ZE_BYENUM     NULL
#  define W_ZE_KIND       0
#endif

#ifdef MPIWRAPPER_HAVE_PMPIX_GPU_query_support
#  define W_PBYENUM       PMPIX_GPU_query_support
#elif defined MPIWRAPPER_HAVE_MPIX_GPU_query_support
#  define W_PBYENUM       MPIX_GPU_query_support
#else
#  define W_PBYENUM       NULL
#endif
#if defined MPIWRAPPER_HAVE_MPIX_GPU_SUPPORT_CUDA
#  define W_CUDA_PBYENUM  W_PBYENUM
#else
#  define W_CUDA_PBYENUM  NULL
#endif
#if defined MPIWRAPPER_HAVE_MPIX_GPU_SUPPORT_HIP
#  define W_ROCM_PBYENUM  W_PBYENUM
#else
#  define W_ROCM_PBYENUM  NULL
#endif
#if defined MPIWRAPPER_HAVE_MPIX_GPU_SUPPORT_ZE
#  define W_ZE_PBYENUM    W_PBYENUM
#else
#  define W_ZE_PBYENUM    NULL
#endif

/* The whole of the answering rule, once. `single` first because it is the
 * form both implementations agree on; the enum form second, because an
 * implementation that has only it can still answer; and 0 last, because that
 * is what "this MPI is not GPU-aware" looks like.
 *
 * A failed enum call answers 0 rather than propagating: the return code is an
 * MPI error class and this function returns a boolean, so there is nowhere to
 * put it, and an implementation that refuses the question has told us the
 * kind is not supported.
 */
static int ask(int (*single)(void), int (*byenum)(int, int *), int kind)
{
  if (single)
    return single() != 0;
  if (byenum) {
    int is_supported = 0;
    if (byenum(kind, &is_supported) != MPI_SUCCESS)
      return 0;
    return is_supported != 0;
  }
  (void)kind;
  return 0;
}

static int gpu_cuda(void) { return ask(W_CUDA_SINGLE, W_CUDA_BYENUM,
                                       W_CUDA_KIND); }
static int gpu_rocm(void) { return ask(W_ROCM_SINGLE, W_ROCM_BYENUM,
                                       W_ROCM_KIND); }
static int gpu_ze(void) { return ask(W_ZE_SINGLE, W_ZE_BYENUM, W_ZE_KIND); }

static int gpu_cuda_p(void) { return ask(W_CUDA_PSINGLE, W_CUDA_PBYENUM,
                                         W_CUDA_KIND); }
static int gpu_rocm_p(void) { return ask(W_ROCM_PSINGLE, W_ROCM_PBYENUM,
                                         W_ROCM_KIND); }
static int gpu_ze_p(void) { return ask(W_ZE_PSINGLE, W_ZE_PBYENUM,
                                       W_ZE_KIND); }

int mpiwrapper_w_MPIX_Query_cuda_support(void) { return gpu_cuda(); }
int mpiwrapper_w_PMPIX_Query_cuda_support(void) { return gpu_cuda_p(); }

/* Both names, one helper: see the head of this file. */
int mpiwrapper_w_MPIX_Query_rocm_support(void) { return gpu_rocm(); }
int mpiwrapper_w_PMPIX_Query_rocm_support(void) { return gpu_rocm_p(); }
int mpiwrapper_w_MPIX_Query_hip_support(void) { return gpu_rocm(); }
int mpiwrapper_w_PMPIX_Query_hip_support(void) { return gpu_rocm_p(); }

int mpiwrapper_w_MPIX_Query_ze_support(void) { return gpu_ze(); }
int mpiwrapper_w_PMPIX_Query_ze_support(void) { return gpu_ze_p(); }

/* Composed from the three helpers rather than forwarded, so the ABI's kind
 * value stays on this side of the boundary. The default arm is MPICH's own
 * answer for a kind it does not recognise: MPI_ERR_ARG, with the flag left 0
 * rather than undefined -- a caller that ignores the return code still reads
 * a defined value, which is the rule the generated bodies follow for every
 * out parameter (`out-params-defined`).
 */
int mpiwrapper_w_MPIX_GPU_query_support(int abi_gpu_type, int *abi_is_supported)
{
  switch (abi_gpu_type) {
  case MPIABIX_GPU_SUPPORT_CUDA:
    *abi_is_supported = gpu_cuda();
    return MPIABI_SUCCESS;
  case MPIABIX_GPU_SUPPORT_ZE:
    *abi_is_supported = gpu_ze();
    return MPIABI_SUCCESS;
  case MPIABIX_GPU_SUPPORT_HIP:
    *abi_is_supported = gpu_rocm();
    return MPIABI_SUCCESS;
  default:
    *abi_is_supported = 0;
    return MPIABI_ERR_ARG;
  }
}

int mpiwrapper_w_PMPIX_GPU_query_support(int abi_gpu_type,
                                         int *abi_is_supported)
{
  switch (abi_gpu_type) {
  case MPIABIX_GPU_SUPPORT_CUDA:
    *abi_is_supported = gpu_cuda_p();
    return MPIABI_SUCCESS;
  case MPIABIX_GPU_SUPPORT_ZE:
    *abi_is_supported = gpu_ze_p();
    return MPIABI_SUCCESS;
  case MPIABIX_GPU_SUPPORT_HIP:
    *abi_is_supported = gpu_rocm_p();
    return MPIABI_SUCCESS;
  default:
    *abi_is_supported = 0;
    return MPIABI_ERR_ARG;
  }
}
