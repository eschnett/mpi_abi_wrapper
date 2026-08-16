/* libmpiwrapper -- the two attribute getters whose *value* is a converted
 * class (NOTES.md #5.1, #5.4, S7).
 *
 * Every other conversion in this project is decided by a parameter's type.
 * These two are decided by a parameter's *value*: MPI_Comm_get_attr and
 * MPI_Win_get_attr hand back a `void *` whose meaning is whatever the keyval
 * says it is, and for five of the thirteen predefined keys that meaning is a
 * family this library maps.
 *
 *   MPI_HOST, MPI_IO             a rank (#5.4). MPICH answers MPI_PROC_NULL
 *                                for HOST and MPI_ANY_SOURCE for IO, which
 *                                are -1 and -2 there and -3 and -1 on the
 *                                ABI. Passed through, an application reading
 *                                MPI_HOST gets the ABI's MPI_ANY_SOURCE.
 *   MPI_LASTUSEDCODE             an error code (#5.6), so the implementation's
 *                                value, not the ABI's.
 *   MPI_WIN_CREATE_FLAVOR        MPI_WIN_FLAVOR_* is 1-4 in MPICH and Open
 *   MPI_WIN_MODEL                MPI, 311-314 and 321-322 on the ABI.
 *
 * The other eight -- MPI_TAG_UB, MPI_WTIME_IS_GLOBAL, MPI_UNIVERSE_SIZE,
 * MPI_APPNUM, MPI_WIN_BASE, MPI_WIN_SIZE, MPI_WIN_DISP_UNIT and the invalid
 * marker -- are plain values in both spellings and pass through untouched, as
 * does every dynamic keyval, whose value is the application's own pointer and
 * was never ours to read.
 *
 * **What found this: the MPICH suite's attr/baseattr2 (S7).** No assertion in
 * the generator could have: the generated body was correct C, forwarded a
 * correct pointer, and returned MPI_SUCCESS. The value inside it was simply a
 * number from the other side of the boundary. It is the same class of mistake
 * as returning an unmapped error code, and the only reason it survived four
 * stages is that an attribute value is a `void *` rather than an `int` in the
 * signature, so nothing in apis.json marks it.
 *
 * **Where the converted value lives.** The pointer the implementation hands
 * back points into *its* storage, so a converted value needs storage of ours,
 * and MPI-5.0 7.7.2 requires only that the pointer stay valid for the caller
 * to read the value out of. One thread-local int per convertible key is
 * therefore enough and is what this file uses: two threads asking for
 * MPI_HOST at once cannot see each other's slot, and one thread asking twice
 * overwrites a value it has already read. The alternative -- a slot per
 * (object, key) pair -- would be a table with no bound and no point, since
 * every one of these five keys has a single value per object and four of them
 * are constants.
 *
 * MPI_Attr_get needs nothing here: libmpi_abi answers it in terms of
 * MPI_Comm_get_attr (NOTES.md #3, "the five entry points MPI-3.0 deleted"),
 * so it inherits this body rather than repeating it.
 */

#include "internal.h"

/* One slot per convertible key. Thread-local, so the storage question above
 * is answered by the language rather than by a lock.
 */
enum {
  ATTR_SLOT_HOST,
  ATTR_SLOT_IO,
  ATTR_SLOT_LASTUSEDCODE,
  ATTR_SLOT_WIN_CREATE_FLAVOR,
  ATTR_SLOT_WIN_MODEL,
  ATTR_SLOT_COUNT
};

static _Thread_local int attr_slot[ATTR_SLOT_COUNT];

static void *attr_store(int slot, int value)
{
  attr_slot[slot] = value;
  return &attr_slot[slot];
}

/* The implementation wrote an `int` through the pointer it returned; read it
 * back, map it, and answer with a pointer to our own copy.
 */
static void *comm_attr_toabi(int abi_comm_keyval, void *value)
{
  if (!value) return value;
  switch (abi_comm_keyval) {
  case MPIABI_HOST:
    return attr_store(ATTR_SLOT_HOST, mpiwrapper_rank_toabi(*(int *)value));
  case MPIABI_IO:
    return attr_store(ATTR_SLOT_IO, mpiwrapper_rank_toabi(*(int *)value));
  case MPIABI_LASTUSEDCODE:
    /* Not a conversion of the implementation's answer: the question is what
     * the largest code an application can be handed *here* is, and after
     * mapping that is a property of the wrapper's own error-code registry
     * (src/mpiwrapper/errorcodes.c). The implementation's maximum is a number
     * in its numbering, and nothing maps it into ours -- interning it would
     * produce a code in our range that is not an upper bound of anything.
     */
    (void)value;
    return attr_store(ATTR_SLOT_LASTUSEDCODE, mpiwrapper_errorcode_lastused());
  default:
    return value;
  }
}

static void *win_attr_toabi(int abi_win_keyval, void *value)
{
  if (!value) return value;
  switch (abi_win_keyval) {
  case MPIABI_WIN_CREATE_FLAVOR:
    return attr_store(ATTR_SLOT_WIN_CREATE_FLAVOR,
                      mpiwrapper_winflavor_toabi(*(int *)value));
  case MPIABI_WIN_MODEL:
    return attr_store(ATTR_SLOT_WIN_MODEL,
                      mpiwrapper_winmodel_toabi(*(int *)value));
  default:
    return value;
  }
}

/* --------------------------------------------------- MPI_Comm_get_attr --- */

/* `attribute_val` is a `void *` in the signature and a `void **` in fact --
 * MPI's own convention for an OUT pointer-to-pointer. The generated body
 * passed it straight to the implementation, which is why it could not convert
 * anything: the value never passed through this library at all.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Comm_get_attr
#  define BODY_MPI_Comm_get_attr(TARGET)                                       \
    {                                                                          \
      const MPI_Comm comm        = mpiwrapper_comm_fromabi(abi_comm);          \
      const int      comm_keyval = mpiwrapper_keyval_fromabi(abi_comm_keyval); \
                                                                               \
      void     *value  = NULL;                                                 \
      int       flag   = 0;                                                    \
      const int ierror = TARGET(comm, comm_keyval, &value, &flag);             \
                                                                               \
      if (ierror == MPI_SUCCESS) {                                             \
        if (flag) *(void **)abi_attribute_val = comm_attr_toabi(               \
                      abi_comm_keyval, value);                                 \
        *abi_flag = flag;                                                      \
      }                                                                        \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Comm_get_attr(TARGET)                                       \
    {                                                                          \
      (void)abi_comm;                                                          \
      (void)abi_comm_keyval;                                                   \
      (void)abi_attribute_val;                                                 \
      (void)abi_flag;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Comm_get_attr(MPIABI_Comm abi_comm, int abi_comm_keyval,
                                   void *abi_attribute_val, int *abi_flag)
    BODY_MPI_Comm_get_attr(MPI_Comm_get_attr)
int mpiwrapper_w_PMPI_Comm_get_attr(MPIABI_Comm abi_comm, int abi_comm_keyval,
                                    void *abi_attribute_val, int *abi_flag)
    BODY_MPI_Comm_get_attr(PMPI_Comm_get_attr)

/* ---------------------------------------------------- MPI_Win_get_attr --- */

#ifdef MPIWRAPPER_HAVE_MPI_Win_get_attr
#  define BODY_MPI_Win_get_attr(TARGET)                                        \
    {                                                                          \
      const MPI_Win win        = mpiwrapper_win_fromabi(abi_win);              \
      const int     win_keyval = mpiwrapper_keyval_fromabi(abi_win_keyval);    \
                                                                               \
      void     *value  = NULL;                                                 \
      int       flag   = 0;                                                    \
      const int ierror = TARGET(win, win_keyval, &value, &flag);               \
                                                                               \
      if (ierror == MPI_SUCCESS) {                                             \
        if (flag)                                                              \
          *(void **)abi_attribute_val = win_attr_toabi(abi_win_keyval, value); \
        *abi_flag = flag;                                                      \
      }                                                                        \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Win_get_attr(TARGET)                                        \
    {                                                                          \
      (void)abi_win;                                                           \
      (void)abi_win_keyval;                                                    \
      (void)abi_attribute_val;                                                 \
      (void)abi_flag;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Win_get_attr(MPIABI_Win abi_win, int abi_win_keyval,
                                  void *abi_attribute_val, int *abi_flag)
    BODY_MPI_Win_get_attr(MPI_Win_get_attr)
int mpiwrapper_w_PMPI_Win_get_attr(MPIABI_Win abi_win, int abi_win_keyval,
                                   void *abi_attribute_val, int *abi_flag)
    BODY_MPI_Win_get_attr(PMPI_Win_get_attr)
