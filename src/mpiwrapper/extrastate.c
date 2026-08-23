/* libmpiwrapper -- the callback families that carry their own extra state
 * (NOTES.md #6.1, S4b).
 *
 * #6.1 splits the sixteen registrars by mechanism rather than by subject, and
 * this file is the second row of that table: the families whose registrar
 * takes an `extra_state` argument that the implementation hands back to the
 * callback. There is nothing to smuggle a user pointer through in the first
 * row -- MPI_Op_create and the four *_create_errhandler forms -- which is why
 * those need callbacks.c's pools. Here a heap-allocated
 * {user_fn, user_extra} pair *is* the mechanism, and it is also the only
 * place a per-registration allocation can be attached to.
 *
 * Three families, and their lifetimes differ by more than their signatures
 * (#6.2):
 *
 *   attribute keys   never reclaimed. MPI-5.0 7.7: freeing a key "is not
 *                    erroneous ... because the actual free does not transpire
 *                    until after all references have been freed", and the
 *                    implementation calls delete callbacks on MPI_COMM_SELF
 *                    from inside MPI_Finalize, so there is no point at which
 *                    the pair is known dead.
 *   generalized      reclaimed in our own free trampoline, which MPI invokes
 *   requests         exactly once and after which the object is deallocated.
 *                    The one clean case in the whole design.
 *   datareps         never reclaimed, and not by omission: MPI has no
 *                    deregistration call at all.
 *
 * **The predefined attribute functions are sentinels, not functions.** The ABI
 * spells MPI_COMM_NULL_COPY_FN as (function *)0x0 and MPI_COMM_DUP_FN as
 * (function *)0x1, while both implementations spell them as real functions
 * with real addresses -- Open MPI's MPI_COMM_DUP_FN is OMPI_C_MPI_COMM_DUP_FN.
 * So the two have to be recognized and replaced with the implementation's own,
 * exactly like MPI_BOTTOM (#5.3), and a trampoline must *not* be installed for
 * them: wrapping MPI_COMM_DUP_FN in a callback of ours would turn a copy the
 * implementation performs internally into a call back into user code that is
 * not there.
 */

#include "internal.h"

#include <stdlib.h>

/* --------------------------------------------------------- attribute keys */

/* One pair type per family rather than one shared type with a union: the
 * trampolines differ by the handle class in their first parameter, so they are
 * three sets of functions anyway, and a shared type would only move the
 * distinction from the compiler's reach into a comment.
 */
struct comm_attr_pair {
  MPIABI_Comm_copy_attr_function   *copy_fn;
  MPIABI_Comm_delete_attr_function *delete_fn;
  void                             *extra_state;
};

struct type_attr_pair {
  MPIABI_Type_copy_attr_function   *copy_fn;
  MPIABI_Type_delete_attr_function *delete_fn;
  void                             *extra_state;
};

struct win_attr_pair {
  MPIABI_Win_copy_attr_function   *copy_fn;
  MPIABI_Win_delete_attr_function *delete_fn;
  void                            *extra_state;
};

static int comm_copy_tramp(MPI_Comm oldcomm, int comm_keyval, void *extra_state,
                           void *attribute_val_in, void *attribute_val_out,
                           int *flag)
{
  const struct comm_attr_pair *const pair = extra_state;

  const MPIABI_Comm abi_oldcomm     = mpiwrapper_comm_toabi(oldcomm);
  const int         abi_comm_keyval = mpiwrapper_keyval_toabi(comm_keyval);

  /* The attribute values are the application's own pointers in both
   * directions and mean nothing to MPI, so they cross unconverted -- and
   * `flag` is a plain int the callback sets.
   */
  const int abi_ierror =
      pair->copy_fn(abi_oldcomm, abi_comm_keyval, pair->extra_state,
                    attribute_val_in, attribute_val_out, flag);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

static int comm_delete_tramp(MPI_Comm comm, int comm_keyval,
                             void *attribute_val, void *extra_state)
{
  const struct comm_attr_pair *const pair = extra_state;

  const MPIABI_Comm abi_comm        = mpiwrapper_comm_toabi(comm);
  const int         abi_comm_keyval = mpiwrapper_keyval_toabi(comm_keyval);

  const int abi_ierror = pair->delete_fn(abi_comm, abi_comm_keyval,
                                         attribute_val, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

int mpiwrapper_comm_attr_fns(MPIABI_Comm_copy_attr_function   *abi_copy_fn,
                             MPIABI_Comm_delete_attr_function *abi_delete_fn,
                             void                             *abi_extra_state,
                             MPI_Comm_copy_attr_function     **copy_fn,
                             MPI_Comm_delete_attr_function   **delete_fn,
                             void                            **state)
{
  const int copy_is_ours   = abi_copy_fn != MPIABI_COMM_NULL_COPY_FN
                             && abi_copy_fn != MPIABI_COMM_DUP_FN;
  const int delete_is_ours = abi_delete_fn != MPIABI_COMM_NULL_DELETE_FN;

  /* Nothing of ours runs, so nothing of ours has to be allocated -- and the
   * application's own extra_state goes down untouched, which is what an
   * implementation whose predefined functions read it would expect. This is
   * the common case: MPI_COMM_NULL_COPY_FN with MPI_COMM_NULL_DELETE_FN.
   */
  if (!copy_is_ours && !delete_is_ours) {
    *copy_fn   = abi_copy_fn == MPIABI_COMM_DUP_FN ? MPI_COMM_DUP_FN
                                                   : MPI_COMM_NULL_COPY_FN;
    *delete_fn = MPI_COMM_NULL_DELETE_FN;
    *state     = abi_extra_state;
    return 1;
  }

  struct comm_attr_pair *const pair = malloc(sizeof *pair);
  if (!pair) return 0;
  pair->copy_fn     = abi_copy_fn;
  pair->delete_fn   = abi_delete_fn;
  pair->extra_state = abi_extra_state;

  *copy_fn = copy_is_ours ? comm_copy_tramp
             : abi_copy_fn == MPIABI_COMM_DUP_FN ? MPI_COMM_DUP_FN
                                                 : MPI_COMM_NULL_COPY_FN;
  *delete_fn = delete_is_ours ? comm_delete_tramp : MPI_COMM_NULL_DELETE_FN;
  *state     = pair;
  return 1;
}

static int type_copy_tramp(MPI_Datatype oldtype, int type_keyval,
                           void *extra_state, void *attribute_val_in,
                           void *attribute_val_out, int *flag)
{
  const struct type_attr_pair *const pair = extra_state;

  const MPIABI_Datatype abi_oldtype     = mpiwrapper_datatype_toabi(oldtype);
  const int             abi_type_keyval = mpiwrapper_keyval_toabi(type_keyval);

  const int abi_ierror =
      pair->copy_fn(abi_oldtype, abi_type_keyval, pair->extra_state,
                    attribute_val_in, attribute_val_out, flag);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

static int type_delete_tramp(MPI_Datatype datatype, int type_keyval,
                             void *attribute_val, void *extra_state)
{
  const struct type_attr_pair *const pair = extra_state;

  const MPIABI_Datatype abi_datatype    = mpiwrapper_datatype_toabi(datatype);
  const int             abi_type_keyval = mpiwrapper_keyval_toabi(type_keyval);

  const int abi_ierror = pair->delete_fn(abi_datatype, abi_type_keyval,
                                         attribute_val, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

int mpiwrapper_type_attr_fns(MPIABI_Type_copy_attr_function   *abi_copy_fn,
                             MPIABI_Type_delete_attr_function *abi_delete_fn,
                             void                             *abi_extra_state,
                             MPI_Type_copy_attr_function     **copy_fn,
                             MPI_Type_delete_attr_function   **delete_fn,
                             void                            **state)
{
  const int copy_is_ours   = abi_copy_fn != MPIABI_TYPE_NULL_COPY_FN
                             && abi_copy_fn != MPIABI_TYPE_DUP_FN;
  const int delete_is_ours = abi_delete_fn != MPIABI_TYPE_NULL_DELETE_FN;

  if (!copy_is_ours && !delete_is_ours) {
    *copy_fn   = abi_copy_fn == MPIABI_TYPE_DUP_FN ? MPI_TYPE_DUP_FN
                                                   : MPI_TYPE_NULL_COPY_FN;
    *delete_fn = MPI_TYPE_NULL_DELETE_FN;
    *state     = abi_extra_state;
    return 1;
  }

  struct type_attr_pair *const pair = malloc(sizeof *pair);
  if (!pair) return 0;
  pair->copy_fn     = abi_copy_fn;
  pair->delete_fn   = abi_delete_fn;
  pair->extra_state = abi_extra_state;

  *copy_fn = copy_is_ours ? type_copy_tramp
             : abi_copy_fn == MPIABI_TYPE_DUP_FN ? MPI_TYPE_DUP_FN
                                                 : MPI_TYPE_NULL_COPY_FN;
  *delete_fn = delete_is_ours ? type_delete_tramp : MPI_TYPE_NULL_DELETE_FN;
  *state     = pair;
  return 1;
}

static int win_copy_tramp(MPI_Win oldwin, int win_keyval, void *extra_state,
                          void *attribute_val_in, void *attribute_val_out,
                          int *flag)
{
  const struct win_attr_pair *const pair = extra_state;

  const MPIABI_Win abi_oldwin     = mpiwrapper_win_toabi(oldwin);
  const int        abi_win_keyval = mpiwrapper_keyval_toabi(win_keyval);

  const int abi_ierror =
      pair->copy_fn(abi_oldwin, abi_win_keyval, pair->extra_state,
                    attribute_val_in, attribute_val_out, flag);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

static int win_delete_tramp(MPI_Win win, int win_keyval, void *attribute_val,
                            void *extra_state)
{
  const struct win_attr_pair *const pair = extra_state;

  const MPIABI_Win abi_win        = mpiwrapper_win_toabi(win);
  const int        abi_win_keyval = mpiwrapper_keyval_toabi(win_keyval);

  const int abi_ierror = pair->delete_fn(abi_win, abi_win_keyval,
                                         attribute_val, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

int mpiwrapper_win_attr_fns(MPIABI_Win_copy_attr_function   *abi_copy_fn,
                            MPIABI_Win_delete_attr_function *abi_delete_fn,
                            void                            *abi_extra_state,
                            MPI_Win_copy_attr_function     **copy_fn,
                            MPI_Win_delete_attr_function   **delete_fn,
                            void                           **state)
{
  const int copy_is_ours   = abi_copy_fn != MPIABI_WIN_NULL_COPY_FN
                             && abi_copy_fn != MPIABI_WIN_DUP_FN;
  const int delete_is_ours = abi_delete_fn != MPIABI_WIN_NULL_DELETE_FN;

  if (!copy_is_ours && !delete_is_ours) {
    *copy_fn   = abi_copy_fn == MPIABI_WIN_DUP_FN ? MPI_WIN_DUP_FN
                                                  : MPI_WIN_NULL_COPY_FN;
    *delete_fn = MPI_WIN_NULL_DELETE_FN;
    *state     = abi_extra_state;
    return 1;
  }

  struct win_attr_pair *const pair = malloc(sizeof *pair);
  if (!pair) return 0;
  pair->copy_fn     = abi_copy_fn;
  pair->delete_fn   = abi_delete_fn;
  pair->extra_state = abi_extra_state;

  *copy_fn = copy_is_ours ? win_copy_tramp
             : abi_copy_fn == MPIABI_WIN_DUP_FN ? MPI_WIN_DUP_FN
                                                : MPI_WIN_NULL_COPY_FN;
  *delete_fn = delete_is_ours ? win_delete_tramp : MPI_WIN_NULL_DELETE_FN;
  *state     = pair;
  return 1;
}

/* --------------------------------------------------- generalized requests */

#ifdef MPIWRAPPER_HAVE_MPI_Grequest_start

struct grequest_pair {
  MPIABI_Grequest_query_function  *query_fn;
  MPIABI_Grequest_free_function   *free_fn;
  MPIABI_Grequest_cancel_function *cancel_fn;
  void                            *extra_state;
};

/* The status is the reason this one is not a pure forward. The user's query
 * function fills an *ABI* status -- MPI_Status_set_elements and
 * MPI_Status_set_cancelled through the ABI, or the named fields directly --
 * and the implementation expects its own. So the blob crosses in both
 * directions around the call: in, because MPI may have initialized fields the
 * callback is expected to leave alone, and out, because that is the whole
 * point of the callback.
 */
static int grequest_query_tramp(void *extra_state, MPI_Status *status)
{
  const struct grequest_pair *const pair = extra_state;

  MPIABI_Status abi_status;
  mpiwrapper_status_toabi(status, &abi_status);

  const int abi_ierror = pair->query_fn(pair->extra_state, &abi_status);

  mpiwrapper_status_fromabi(&abi_status, status);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

/* #6.2's one clean reclamation point: MPI invokes this exactly once and
 * deallocates the object afterwards, "even if the user's free_fn returns an
 * error" -- so the pair is freed unconditionally, after the user's function
 * has had its last look at it.
 */
static int grequest_free_tramp(void *extra_state)
{
  struct grequest_pair *const pair = extra_state;

  const int abi_ierror = pair->free_fn(pair->extra_state);

  free(pair);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

static int grequest_cancel_tramp(void *extra_state, int complete)
{
  const struct grequest_pair *const pair = extra_state;

  const int abi_ierror = pair->cancel_fn(pair->extra_state, complete);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

int mpiwrapper_grequest_fns(MPIABI_Grequest_query_function  *abi_query_fn,
                            MPIABI_Grequest_free_function   *abi_free_fn,
                            MPIABI_Grequest_cancel_function *abi_cancel_fn,
                            void                            *abi_extra_state,
                            MPI_Grequest_query_function     **query_fn,
                            MPI_Grequest_free_function      **free_fn,
                            MPI_Grequest_cancel_function    **cancel_fn,
                            void                            **state)
{
  struct grequest_pair *const pair = malloc(sizeof *pair);
  if (!pair) return 0;

  pair->query_fn    = abi_query_fn;
  pair->free_fn     = abi_free_fn;
  pair->cancel_fn   = abi_cancel_fn;
  pair->extra_state = abi_extra_state;

  *query_fn  = grequest_query_tramp;
  *free_fn   = grequest_free_tramp;
  *cancel_fn = grequest_cancel_tramp;
  *state     = pair;
  return 1;
}

void mpiwrapper_grequest_discard(void *state) { free(state); }

#endif /* MPIWRAPPER_HAVE_MPI_Grequest_start */

/* ------------------------------------------------------------- datareps */

#if defined(MPIWRAPPER_HAVE_MPI_Register_datarep) ||                           \
    defined(MPIWRAPPER_HAVE_MPI_Register_datarep_c)

/* One pair type for both widths, since the two differ only in the conversion
 * functions' `count` and a datarep is registered once per process: the
 * unused half costs two pointers and removes a second discard path.
 */
struct datarep_pair {
#  ifdef MPIWRAPPER_HAVE_MPI_Register_datarep
  MPIABI_Datarep_conversion_function *read_fn;
  MPIABI_Datarep_conversion_function *write_fn;
#  endif
  /* Present whenever *either* the large-count registrar or its fallback is
   * compiled. The narrower guard that used to be here read naturally and was
   * exactly wrong for the fallback, whose whole premise is that
   * MPI_Register_datarep_c is absent (NOTES.md #5.10).
   */
#  if defined(MPIWRAPPER_HAVE_MPI_Register_datarep_c) ||                       \
      defined(MPIWRAPPER_HAVE_MPI_Register_datarep)
  MPIABI_Datarep_conversion_function_c *read_fn_c;
  MPIABI_Datarep_conversion_function_c *write_fn_c;
#  endif
  MPIABI_Datarep_extent_function *extent_fn;
  void                           *extra_state;
};

/* The datatype is what makes these trampolines necessary: MPI hands the
 * conversion function the datatype it is converting, and the user's function
 * is written against the ABI's handle for it.
 */
static int datarep_extent_tramp(MPI_Datatype datatype, MPI_Aint *extent,
                                void *extra_state)
{
  const struct datarep_pair *const pair = extra_state;

  const MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(datatype);
  MPIABI_Aint           abi_extent   = 0;

  const int abi_ierror =
      pair->extent_fn(abi_datatype, &abi_extent, pair->extra_state);

  *extent = (MPI_Aint)abi_extent;
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

void mpiwrapper_datarep_discard(void *state) { free(state); }

#endif

#ifdef MPIWRAPPER_HAVE_MPI_Register_datarep

static int datarep_read_tramp(void *userbuf, MPI_Datatype datatype, int count,
                              void *filebuf, MPI_Offset position,
                              void *extra_state)
{
  const struct datarep_pair *const pair = extra_state;

  const MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(datatype);

  const int abi_ierror =
      pair->read_fn(userbuf, abi_datatype, count, filebuf,
                    (MPIABI_Offset)position, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

static int datarep_write_tramp(void *userbuf, MPI_Datatype datatype, int count,
                               void *filebuf, MPI_Offset position,
                               void *extra_state)
{
  const struct datarep_pair *const pair = extra_state;

  const MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(datatype);

  const int abi_ierror =
      pair->write_fn(userbuf, abi_datatype, count, filebuf,
                     (MPIABI_Offset)position, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

int mpiwrapper_datarep_fns(MPIABI_Datarep_conversion_function *abi_read_fn,
                           MPIABI_Datarep_conversion_function *abi_write_fn,
                           MPIABI_Datarep_extent_function     *abi_extent_fn,
                           void                               *abi_extra_state,
                           MPI_Datarep_conversion_function   **read_fn,
                           MPI_Datarep_conversion_function   **write_fn,
                           MPI_Datarep_extent_function       **extent_fn,
                           void                              **state)
{
  struct datarep_pair *const pair = calloc(1, sizeof *pair);
  if (!pair) return 0;

  pair->read_fn     = abi_read_fn;
  pair->write_fn    = abi_write_fn;
  pair->extent_fn   = abi_extent_fn;
  pair->extra_state = abi_extra_state;

  /* MPI_CONVERSION_FN_NULL says "this datarep needs no conversion in this
   * direction", so it is a sentinel like the attribute functions above and
   * must reach the implementation as *its* spelling of the same idea rather
   * than as a trampoline that would call through a null pointer.
   */
  *read_fn   = abi_read_fn == MPIABI_CONVERSION_FN_NULL
                   ? MPI_CONVERSION_FN_NULL
                   : datarep_read_tramp;
  *write_fn  = abi_write_fn == MPIABI_CONVERSION_FN_NULL
                   ? MPI_CONVERSION_FN_NULL
                   : datarep_write_tramp;
  *extent_fn = datarep_extent_tramp;
  *state     = pair;
  return 1;
}

#endif /* MPIWRAPPER_HAVE_MPI_Register_datarep */

#ifdef MPIWRAPPER_HAVE_MPI_Register_datarep_c

static int datarep_read_c_tramp(void *userbuf, MPI_Datatype datatype,
                                MPI_Count count, void *filebuf,
                                MPI_Offset position, void *extra_state)
{
  const struct datarep_pair *const pair = extra_state;

  const MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(datatype);

  const int abi_ierror =
      pair->read_fn_c(userbuf, abi_datatype, (MPIABI_Count)count, filebuf,
                      (MPIABI_Offset)position, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

static int datarep_write_c_tramp(void *userbuf, MPI_Datatype datatype,
                                 MPI_Count count, void *filebuf,
                                 MPI_Offset position, void *extra_state)
{
  const struct datarep_pair *const pair = extra_state;

  const MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(datatype);

  const int abi_ierror =
      pair->write_fn_c(userbuf, abi_datatype, (MPIABI_Count)count, filebuf,
                       (MPIABI_Offset)position, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

int mpiwrapper_datarep_c_fns(MPIABI_Datarep_conversion_function_c *abi_read_fn,
                             MPIABI_Datarep_conversion_function_c *abi_write_fn,
                             MPIABI_Datarep_extent_function *abi_extent_fn,
                             void                           *abi_extra_state,
                             MPI_Datarep_conversion_function_c **read_fn,
                             MPI_Datarep_conversion_function_c **write_fn,
                             MPI_Datarep_extent_function      **extent_fn,
                             void                             **state)
{
  struct datarep_pair *const pair = calloc(1, sizeof *pair);
  if (!pair) return 0;

  pair->read_fn_c   = abi_read_fn;
  pair->write_fn_c  = abi_write_fn;
  pair->extent_fn   = abi_extent_fn;
  pair->extra_state = abi_extra_state;

  *read_fn   = abi_read_fn == MPIABI_CONVERSION_FN_NULL_C
                   ? MPI_CONVERSION_FN_NULL_C
                   : datarep_read_c_tramp;
  *write_fn  = abi_write_fn == MPIABI_CONVERSION_FN_NULL_C
                   ? MPI_CONVERSION_FN_NULL_C
                   : datarep_write_c_tramp;
  *extent_fn = datarep_extent_tramp;
  *state     = pair;
  return 1;
}

#endif /* MPIWRAPPER_HAVE_MPI_Register_datarep_c */

/* ------------------------- a large-count datarep, over a small-count MPI ---- */

/* NOTES.md #5.10. The caller registered MPIABI_Datarep_conversion_function_c
 * through MPI_Register_datarep_c, and the implementation has only
 * MPI_Register_datarep, whose conversion function takes `int count`. So these
 * two trampolines have the *small* shape -- that is what the implementation
 * stores and will call -- and widen the count on the way into the caller's
 * function.
 *
 * Like the op adapter, this cannot fail: the count comes from the
 * implementation, which produced it as an int, so widening it is exact.
 *
 * The one thing that is not symmetric is the sentinel. The caller says "no
 * conversion in this direction" with MPIABI_CONVERSION_FN_NULL_C, and what has
 * to reach the implementation is MPI_CONVERSION_FN_NULL -- the *small*
 * spelling of the same idea, because the small registrar is what is being
 * called.
 */
#if !defined(MPIWRAPPER_HAVE_MPI_Register_datarep_c) &&                        \
    defined(MPIWRAPPER_HAVE_MPI_Register_datarep)

static int datarep_read_ca_tramp(void *userbuf, MPI_Datatype datatype,
                                 int count, void *filebuf, MPI_Offset position,
                                 void *extra_state)
{
  const struct datarep_pair *const pair = extra_state;

  const MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(datatype);

  const int abi_ierror =
      pair->read_fn_c(userbuf, abi_datatype, (MPIABI_Count)count, filebuf,
                      (MPIABI_Offset)position, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

static int datarep_write_ca_tramp(void *userbuf, MPI_Datatype datatype,
                                  int count, void *filebuf,
                                  MPI_Offset position, void *extra_state)
{
  const struct datarep_pair *const pair = extra_state;

  const MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(datatype);

  const int abi_ierror =
      pair->write_fn_c(userbuf, abi_datatype, (MPIABI_Count)count, filebuf,
                       (MPIABI_Offset)position, pair->extra_state);
  return mpiwrapper_errorcode_fromabi(abi_ierror);
}

int mpiwrapper_datarep_ca_fns(MPIABI_Datarep_conversion_function_c *abi_read_fn,
                              MPIABI_Datarep_conversion_function_c *abi_write_fn,
                              MPIABI_Datarep_extent_function *abi_extent_fn,
                              void                           *abi_extra_state,
                              MPI_Datarep_conversion_function **read_fn,
                              MPI_Datarep_conversion_function **write_fn,
                              MPI_Datarep_extent_function     **extent_fn,
                              void                            **state)
{
  struct datarep_pair *const pair = calloc(1, sizeof *pair);
  if (!pair) return 0;

  pair->read_fn_c   = abi_read_fn;
  pair->write_fn_c  = abi_write_fn;
  pair->extent_fn   = abi_extent_fn;
  pair->extra_state = abi_extra_state;

  *read_fn   = abi_read_fn == MPIABI_CONVERSION_FN_NULL_C
                   ? MPI_CONVERSION_FN_NULL
                   : datarep_read_ca_tramp;
  *write_fn  = abi_write_fn == MPIABI_CONVERSION_FN_NULL_C
                   ? MPI_CONVERSION_FN_NULL
                   : datarep_write_ca_tramp;
  *extent_fn = datarep_extent_tramp;
  *state     = pair;
  return 1;
}

#endif /* the large-count datarep fallback */
