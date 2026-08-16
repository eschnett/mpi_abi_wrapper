/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate.py. libmpiwrapper -- the ABI <-> implementation
 * constant tables, from the ABI header's own definitions.
 *
 * Two rules govern every table here (NOTES.md #3):
 *
 *  - Case labels over handles are *numeric*, with the ABI's symbolic name in a
 *    comment. MPIABI_INT expands to ((MPIABI_Datatype)0x00000209), and casting
 *    an integer constant to a pointer type and back is not an integer constant
 *    expression in standard C -- gcc and clang accept it, but a case label is
 *    where that extension is not worth relying on. The value came from parsing
 *    the header, so nothing is transcribed either way. The integer families
 *    further down are plain enumerators and are switched by name.
 *
 *  - Every implementation-side value is named by the implementation's own
 *    macro or enumerator, never written out. A name that does not exist is
 *    then a compile error rather than a wrong number -- which is why only the
 *    entries the standard makes optional are guarded at all: the sized Fortran
 *    types, the predefined handles and enumerators newer than the MPI-3.0
 *    floor, and MPI_T.
 *
 *  - Those guards test MPIWRAPPER_HAVE_<name>, from dev/probe_impl.py, and
 *    never `#ifdef <the implementation's own name>`. `#ifdef` sees macros and
 *    not enumerators, and implementations use both: MPICH spells MPI_COMBINER_*
 *    and MPI_CART as enumerators, Open MPI spells MPI_THREAD_SINGLE,
 *    MPI_COMM_TYPE_SHARED and MPI_IDENT that way. An `#ifdef` on one of those
 *    answers *no* for a constant that is right there, and the case then drops
 *    out of the switch, reaches the default arm and passes an unmapped value
 *    through -- silently, which is the one failure mode these tables exist to
 *    prevent. Measured, not hypothetical: MPICH 4.3.1 has
 *    MPI_COMBINER_VALUE_INDEX as `= 20` in an enum, and an S2 draft that used
 *    `#ifdef` on it stopped translating that combiner without failing
 *    anything. The probe asks the compiler, which sees both, and asks about
 *    every name in one translation unit rather than one test per constant.
 *
 * The fromabi default deserves a word. A value inside the ABI's predefined
 * range that reached the default arm is a predefined handle *this*
 * implementation does not provide, and bit-casting it would hand the
 * implementation a fabricated handle -- on MPICH an int whose kind bits it
 * will happily interpret. Returning the class's null handle instead makes the
 * implementation reject the call with its own error, which is the honest
 * outcome. Outside the range it is a handle we produced earlier, and the
 * bit-cast is the identity the toabi direction promised.
 */

#include "internal.h"

#include <string.h>


MPI_Op mpiwrapper_op_fromabi(MPIABI_Op abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000020: return MPI_OP_NULL; /* MPIABI_OP_NULL */
  case 0x00000021: return MPI_SUM; /* MPIABI_SUM */
  case 0x00000022: return MPI_MIN; /* MPIABI_MIN */
  case 0x00000023: return MPI_MAX; /* MPIABI_MAX */
  case 0x00000024: return MPI_PROD; /* MPIABI_PROD */
  case 0x00000028: return MPI_BAND; /* MPIABI_BAND */
  case 0x00000029: return MPI_BOR; /* MPIABI_BOR */
  case 0x0000002a: return MPI_BXOR; /* MPIABI_BXOR */
  case 0x00000030: return MPI_LAND; /* MPIABI_LAND */
  case 0x00000031: return MPI_LOR; /* MPIABI_LOR */
  case 0x00000032: return MPI_LXOR; /* MPIABI_LXOR */
  case 0x00000038: return MPI_MINLOC; /* MPIABI_MINLOC */
  case 0x00000039: return MPI_MAXLOC; /* MPIABI_MAXLOC */
  case 0x0000003c: return MPI_REPLACE; /* MPIABI_REPLACE */
  case 0x0000003d: return MPI_NO_OP; /* MPIABI_NO_OP */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_OP_NULL;
  return MPIWRAPPER_HANDLE(MPI_Op, abi);
}

MPI_Comm mpiwrapper_comm_fromabi(MPIABI_Comm abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000100: return MPI_COMM_NULL; /* MPIABI_COMM_NULL */
  case 0x00000101: return MPI_COMM_WORLD; /* MPIABI_COMM_WORLD */
  case 0x00000102: return MPI_COMM_SELF; /* MPIABI_COMM_SELF */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_COMM_NULL;
  return MPIWRAPPER_HANDLE(MPI_Comm, abi);
}

MPI_Group mpiwrapper_group_fromabi(MPIABI_Group abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000108: return MPI_GROUP_NULL; /* MPIABI_GROUP_NULL */
  case 0x00000109: return MPI_GROUP_EMPTY; /* MPIABI_GROUP_EMPTY */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_GROUP_NULL;
  return MPIWRAPPER_HANDLE(MPI_Group, abi);
}

MPI_Win mpiwrapper_win_fromabi(MPIABI_Win abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000110: return MPI_WIN_NULL; /* MPIABI_WIN_NULL */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_WIN_NULL;
  return MPIWRAPPER_HANDLE(MPI_Win, abi);
}

MPI_File mpiwrapper_file_fromabi(MPIABI_File abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000118: return MPI_FILE_NULL; /* MPIABI_FILE_NULL */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_FILE_NULL;
  return MPIWRAPPER_HANDLE(MPI_File, abi);
}

#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
MPI_Session mpiwrapper_session_fromabi(MPIABI_Session abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000120: return MPI_SESSION_NULL; /* MPIABI_SESSION_NULL */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_SESSION_NULL;
  return MPIWRAPPER_HANDLE(MPI_Session, abi);
}
#endif

MPI_Message mpiwrapper_message_fromabi(MPIABI_Message abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000128: return MPI_MESSAGE_NULL; /* MPIABI_MESSAGE_NULL */
  case 0x00000129: return MPI_MESSAGE_NO_PROC; /* MPIABI_MESSAGE_NO_PROC */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_MESSAGE_NULL;
  return MPIWRAPPER_HANDLE(MPI_Message, abi);
}

MPI_Info mpiwrapper_info_fromabi(MPIABI_Info abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000130: return MPI_INFO_NULL; /* MPIABI_INFO_NULL */
  case 0x00000131: return MPI_INFO_ENV; /* MPIABI_INFO_ENV */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_INFO_NULL;
  return MPIWRAPPER_HANDLE(MPI_Info, abi);
}

MPI_Errhandler mpiwrapper_errhandler_fromabi(MPIABI_Errhandler abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000140: return MPI_ERRHANDLER_NULL; /* MPIABI_ERRHANDLER_NULL */
  case 0x00000141: return MPI_ERRORS_ARE_FATAL; /* MPIABI_ERRORS_ARE_FATAL */
#ifdef MPIWRAPPER_HAVE_MPI_ERRORS_ABORT
  case 0x00000142: return MPI_ERRORS_ABORT; /* MPIABI_ERRORS_ABORT */
#endif
  case 0x00000143: return MPI_ERRORS_RETURN; /* MPIABI_ERRORS_RETURN */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_ERRHANDLER_NULL;
  return MPIWRAPPER_HANDLE(MPI_Errhandler, abi);
}

MPI_Request mpiwrapper_request_fromabi(MPIABI_Request abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000180: return MPI_REQUEST_NULL; /* MPIABI_REQUEST_NULL */
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_REQUEST_NULL;
  return MPIWRAPPER_HANDLE(MPI_Request, abi);
}

MPI_Datatype mpiwrapper_datatype_fromabi(MPIABI_Datatype abi)
{
  switch ((uint64_t)(uintptr_t)abi) {
  case 0x00000200: return MPI_DATATYPE_NULL; /* MPIABI_DATATYPE_NULL */
  case 0x00000201: return MPI_AINT; /* MPIABI_AINT */
  case 0x00000202: return MPI_COUNT; /* MPIABI_COUNT */
  case 0x00000203: return MPI_OFFSET; /* MPIABI_OFFSET */
  case 0x00000207: return MPI_PACKED; /* MPIABI_PACKED */
  case 0x00000208: return MPI_SHORT; /* MPIABI_SHORT */
  case 0x00000209: return MPI_INT; /* MPIABI_INT */
  case 0x0000020a: return MPI_LONG; /* MPIABI_LONG */
  case 0x0000020b: return MPI_LONG_LONG; /* MPIABI_LONG_LONG */
  case 0x0000020c: return MPI_UNSIGNED_SHORT; /* MPIABI_UNSIGNED_SHORT */
  case 0x0000020d: return MPI_UNSIGNED; /* MPIABI_UNSIGNED */
  case 0x0000020e: return MPI_UNSIGNED_LONG; /* MPIABI_UNSIGNED_LONG */
  case 0x0000020f: return MPI_UNSIGNED_LONG_LONG; /* MPIABI_UNSIGNED_LONG_LONG */
  case 0x00000210: return MPI_FLOAT; /* MPIABI_FLOAT */
  case 0x00000212: return MPI_C_FLOAT_COMPLEX; /* MPIABI_C_FLOAT_COMPLEX */
  case 0x00000213: return MPI_CXX_FLOAT_COMPLEX; /* MPIABI_CXX_FLOAT_COMPLEX */
  case 0x00000214: return MPI_DOUBLE; /* MPIABI_DOUBLE */
  case 0x00000216: return MPI_C_DOUBLE_COMPLEX; /* MPIABI_C_DOUBLE_COMPLEX */
  case 0x00000217: return MPI_CXX_DOUBLE_COMPLEX; /* MPIABI_CXX_DOUBLE_COMPLEX */
  case 0x00000218: return MPI_LOGICAL; /* MPIABI_LOGICAL */
  case 0x00000219: return MPI_INTEGER; /* MPIABI_INTEGER */
  case 0x0000021a: return MPI_REAL; /* MPIABI_REAL */
  case 0x0000021b: return MPI_COMPLEX; /* MPIABI_COMPLEX */
  case 0x0000021c: return MPI_DOUBLE_PRECISION; /* MPIABI_DOUBLE_PRECISION */
  case 0x0000021d: return MPI_DOUBLE_COMPLEX; /* MPIABI_DOUBLE_COMPLEX */
  case 0x0000021e: return MPI_CHARACTER; /* MPIABI_CHARACTER */
  case 0x00000220: return MPI_LONG_DOUBLE; /* MPIABI_LONG_DOUBLE */
  case 0x00000224: return MPI_C_LONG_DOUBLE_COMPLEX; /* MPIABI_C_LONG_DOUBLE_COMPLEX */
  case 0x00000225: return MPI_CXX_LONG_DOUBLE_COMPLEX; /* MPIABI_CXX_LONG_DOUBLE_COMPLEX */
  case 0x00000228: return MPI_FLOAT_INT; /* MPIABI_FLOAT_INT */
  case 0x00000229: return MPI_DOUBLE_INT; /* MPIABI_DOUBLE_INT */
  case 0x0000022a: return MPI_LONG_INT; /* MPIABI_LONG_INT */
  case 0x0000022b: return MPI_2INT; /* MPIABI_2INT */
  case 0x0000022c: return MPI_SHORT_INT; /* MPIABI_SHORT_INT */
  case 0x0000022d: return MPI_LONG_DOUBLE_INT; /* MPIABI_LONG_DOUBLE_INT */
  case 0x00000230: return MPI_2REAL; /* MPIABI_2REAL */
  case 0x00000231: return MPI_2DOUBLE_PRECISION; /* MPIABI_2DOUBLE_PRECISION */
  case 0x00000232: return MPI_2INTEGER; /* MPIABI_2INTEGER */
  case 0x00000238: return MPI_C_BOOL; /* MPIABI_C_BOOL */
  case 0x00000239: return MPI_CXX_BOOL; /* MPIABI_CXX_BOOL */
  case 0x0000023c: return MPI_WCHAR; /* MPIABI_WCHAR */
  case 0x00000240: return MPI_INT8_T; /* MPIABI_INT8_T */
  case 0x00000241: return MPI_UINT8_T; /* MPIABI_UINT8_T */
  case 0x00000243: return MPI_CHAR; /* MPIABI_CHAR */
  case 0x00000244: return MPI_SIGNED_CHAR; /* MPIABI_SIGNED_CHAR */
  case 0x00000245: return MPI_UNSIGNED_CHAR; /* MPIABI_UNSIGNED_CHAR */
  case 0x00000247: return MPI_BYTE; /* MPIABI_BYTE */
  case 0x00000248: return MPI_INT16_T; /* MPIABI_INT16_T */
  case 0x00000249: return MPI_UINT16_T; /* MPIABI_UINT16_T */
  case 0x00000250: return MPI_INT32_T; /* MPIABI_INT32_T */
  case 0x00000251: return MPI_UINT32_T; /* MPIABI_UINT32_T */
  case 0x00000258: return MPI_INT64_T; /* MPIABI_INT64_T */
  case 0x00000259: return MPI_UINT64_T; /* MPIABI_UINT64_T */
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL1
  case 0x000002c0: return MPI_LOGICAL1; /* MPIABI_LOGICAL1 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER1
  case 0x000002c1: return MPI_INTEGER1; /* MPIABI_INTEGER1 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL2
  case 0x000002c8: return MPI_LOGICAL2; /* MPIABI_LOGICAL2 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER2
  case 0x000002c9: return MPI_INTEGER2; /* MPIABI_INTEGER2 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_REAL2
  case 0x000002ca: return MPI_REAL2; /* MPIABI_REAL2 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL4
  case 0x000002d0: return MPI_LOGICAL4; /* MPIABI_LOGICAL4 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER4
  case 0x000002d1: return MPI_INTEGER4; /* MPIABI_INTEGER4 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_REAL4
  case 0x000002d2: return MPI_REAL4; /* MPIABI_REAL4 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMPLEX4
  case 0x000002d3: return MPI_COMPLEX4; /* MPIABI_COMPLEX4 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL8
  case 0x000002d8: return MPI_LOGICAL8; /* MPIABI_LOGICAL8 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER8
  case 0x000002d9: return MPI_INTEGER8; /* MPIABI_INTEGER8 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_REAL8
  case 0x000002da: return MPI_REAL8; /* MPIABI_REAL8 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMPLEX8
  case 0x000002db: return MPI_COMPLEX8; /* MPIABI_COMPLEX8 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL16
  case 0x000002e0: return MPI_LOGICAL16; /* MPIABI_LOGICAL16 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER16
  case 0x000002e1: return MPI_INTEGER16; /* MPIABI_INTEGER16 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_REAL16
  case 0x000002e2: return MPI_REAL16; /* MPIABI_REAL16 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMPLEX16
  case 0x000002e3: return MPI_COMPLEX16; /* MPIABI_COMPLEX16 */
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMPLEX32
  case 0x000002eb: return MPI_COMPLEX32; /* MPIABI_COMPLEX32 */
#endif
  default: break;
  }
  if (mpiwrapper_in_predef_range(MPIWRAPPER_BITS(abi))) return MPI_DATATYPE_NULL;
  return MPIWRAPPER_HANDLE(MPI_Datatype, abi);
}


/* One filler per class. Written into caller storage at run time rather than
 * declared as static data, because Open MPI's predefined handles are
 * addresses and a pointer-to-integer cast is not a constant expression: a
 * `static const uint64_t t[] = {(uint64_t)(uintptr_t)MPI_INT}` compiles only
 * against an integer-handle MPI, which is how it slips through review.
 */
#define PREDEF(abi_macro, abi_value, impl_macro)                              \
  do {                                                                        \
    if (n < max) {                                                            \
      out[n].abi  = (abi_value);                                              \
      out[n].impl = MPIWRAPPER_BITS(impl_macro);                              \
      out[n].name = #abi_macro;                                               \
    }                                                                         \
    ++n;                                                                      \
  } while (0)

static size_t predef_op(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_OP_NULL, 0x00000020, MPI_OP_NULL);
  PREDEF(MPIABI_SUM, 0x00000021, MPI_SUM);
  PREDEF(MPIABI_MIN, 0x00000022, MPI_MIN);
  PREDEF(MPIABI_MAX, 0x00000023, MPI_MAX);
  PREDEF(MPIABI_PROD, 0x00000024, MPI_PROD);
  PREDEF(MPIABI_BAND, 0x00000028, MPI_BAND);
  PREDEF(MPIABI_BOR, 0x00000029, MPI_BOR);
  PREDEF(MPIABI_BXOR, 0x0000002a, MPI_BXOR);
  PREDEF(MPIABI_LAND, 0x00000030, MPI_LAND);
  PREDEF(MPIABI_LOR, 0x00000031, MPI_LOR);
  PREDEF(MPIABI_LXOR, 0x00000032, MPI_LXOR);
  PREDEF(MPIABI_MINLOC, 0x00000038, MPI_MINLOC);
  PREDEF(MPIABI_MAXLOC, 0x00000039, MPI_MAXLOC);
  PREDEF(MPIABI_REPLACE, 0x0000003c, MPI_REPLACE);
  PREDEF(MPIABI_NO_OP, 0x0000003d, MPI_NO_OP);
  return n;
}

static size_t predef_comm(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_COMM_NULL, 0x00000100, MPI_COMM_NULL);
  PREDEF(MPIABI_COMM_WORLD, 0x00000101, MPI_COMM_WORLD);
  PREDEF(MPIABI_COMM_SELF, 0x00000102, MPI_COMM_SELF);
  return n;
}

static size_t predef_group(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_GROUP_NULL, 0x00000108, MPI_GROUP_NULL);
  PREDEF(MPIABI_GROUP_EMPTY, 0x00000109, MPI_GROUP_EMPTY);
  return n;
}

static size_t predef_win(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_WIN_NULL, 0x00000110, MPI_WIN_NULL);
  return n;
}

static size_t predef_file(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_FILE_NULL, 0x00000118, MPI_FILE_NULL);
  return n;
}

#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
static size_t predef_session(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_SESSION_NULL, 0x00000120, MPI_SESSION_NULL);
  return n;
}
#endif

static size_t predef_message(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_MESSAGE_NULL, 0x00000128, MPI_MESSAGE_NULL);
  PREDEF(MPIABI_MESSAGE_NO_PROC, 0x00000129, MPI_MESSAGE_NO_PROC);
  return n;
}

static size_t predef_info(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_INFO_NULL, 0x00000130, MPI_INFO_NULL);
  PREDEF(MPIABI_INFO_ENV, 0x00000131, MPI_INFO_ENV);
  return n;
}

static size_t predef_errhandler(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_ERRHANDLER_NULL, 0x00000140, MPI_ERRHANDLER_NULL);
  PREDEF(MPIABI_ERRORS_ARE_FATAL, 0x00000141, MPI_ERRORS_ARE_FATAL);
#ifdef MPIWRAPPER_HAVE_MPI_ERRORS_ABORT
  PREDEF(MPIABI_ERRORS_ABORT, 0x00000142, MPI_ERRORS_ABORT);
#endif
  PREDEF(MPIABI_ERRORS_RETURN, 0x00000143, MPI_ERRORS_RETURN);
  return n;
}

static size_t predef_request(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_REQUEST_NULL, 0x00000180, MPI_REQUEST_NULL);
  return n;
}

static size_t predef_datatype(struct mpiwrapper_predef *out, size_t max)
{
  size_t n = 0;
  PREDEF(MPIABI_DATATYPE_NULL, 0x00000200, MPI_DATATYPE_NULL);
  PREDEF(MPIABI_AINT, 0x00000201, MPI_AINT);
  PREDEF(MPIABI_COUNT, 0x00000202, MPI_COUNT);
  PREDEF(MPIABI_OFFSET, 0x00000203, MPI_OFFSET);
  PREDEF(MPIABI_PACKED, 0x00000207, MPI_PACKED);
  PREDEF(MPIABI_SHORT, 0x00000208, MPI_SHORT);
  PREDEF(MPIABI_INT, 0x00000209, MPI_INT);
  PREDEF(MPIABI_LONG, 0x0000020a, MPI_LONG);
  PREDEF(MPIABI_LONG_LONG, 0x0000020b, MPI_LONG_LONG);
  PREDEF(MPIABI_UNSIGNED_SHORT, 0x0000020c, MPI_UNSIGNED_SHORT);
  PREDEF(MPIABI_UNSIGNED, 0x0000020d, MPI_UNSIGNED);
  PREDEF(MPIABI_UNSIGNED_LONG, 0x0000020e, MPI_UNSIGNED_LONG);
  PREDEF(MPIABI_UNSIGNED_LONG_LONG, 0x0000020f, MPI_UNSIGNED_LONG_LONG);
  PREDEF(MPIABI_FLOAT, 0x00000210, MPI_FLOAT);
  PREDEF(MPIABI_C_FLOAT_COMPLEX, 0x00000212, MPI_C_FLOAT_COMPLEX);
  PREDEF(MPIABI_CXX_FLOAT_COMPLEX, 0x00000213, MPI_CXX_FLOAT_COMPLEX);
  PREDEF(MPIABI_DOUBLE, 0x00000214, MPI_DOUBLE);
  PREDEF(MPIABI_C_DOUBLE_COMPLEX, 0x00000216, MPI_C_DOUBLE_COMPLEX);
  PREDEF(MPIABI_CXX_DOUBLE_COMPLEX, 0x00000217, MPI_CXX_DOUBLE_COMPLEX);
  PREDEF(MPIABI_LOGICAL, 0x00000218, MPI_LOGICAL);
  PREDEF(MPIABI_INTEGER, 0x00000219, MPI_INTEGER);
  PREDEF(MPIABI_REAL, 0x0000021a, MPI_REAL);
  PREDEF(MPIABI_COMPLEX, 0x0000021b, MPI_COMPLEX);
  PREDEF(MPIABI_DOUBLE_PRECISION, 0x0000021c, MPI_DOUBLE_PRECISION);
  PREDEF(MPIABI_DOUBLE_COMPLEX, 0x0000021d, MPI_DOUBLE_COMPLEX);
  PREDEF(MPIABI_CHARACTER, 0x0000021e, MPI_CHARACTER);
  PREDEF(MPIABI_LONG_DOUBLE, 0x00000220, MPI_LONG_DOUBLE);
  PREDEF(MPIABI_C_LONG_DOUBLE_COMPLEX, 0x00000224, MPI_C_LONG_DOUBLE_COMPLEX);
  PREDEF(MPIABI_CXX_LONG_DOUBLE_COMPLEX, 0x00000225, MPI_CXX_LONG_DOUBLE_COMPLEX);
  PREDEF(MPIABI_FLOAT_INT, 0x00000228, MPI_FLOAT_INT);
  PREDEF(MPIABI_DOUBLE_INT, 0x00000229, MPI_DOUBLE_INT);
  PREDEF(MPIABI_LONG_INT, 0x0000022a, MPI_LONG_INT);
  PREDEF(MPIABI_2INT, 0x0000022b, MPI_2INT);
  PREDEF(MPIABI_SHORT_INT, 0x0000022c, MPI_SHORT_INT);
  PREDEF(MPIABI_LONG_DOUBLE_INT, 0x0000022d, MPI_LONG_DOUBLE_INT);
  PREDEF(MPIABI_2REAL, 0x00000230, MPI_2REAL);
  PREDEF(MPIABI_2DOUBLE_PRECISION, 0x00000231, MPI_2DOUBLE_PRECISION);
  PREDEF(MPIABI_2INTEGER, 0x00000232, MPI_2INTEGER);
  PREDEF(MPIABI_C_BOOL, 0x00000238, MPI_C_BOOL);
  PREDEF(MPIABI_CXX_BOOL, 0x00000239, MPI_CXX_BOOL);
  PREDEF(MPIABI_WCHAR, 0x0000023c, MPI_WCHAR);
  PREDEF(MPIABI_INT8_T, 0x00000240, MPI_INT8_T);
  PREDEF(MPIABI_UINT8_T, 0x00000241, MPI_UINT8_T);
  PREDEF(MPIABI_CHAR, 0x00000243, MPI_CHAR);
  PREDEF(MPIABI_SIGNED_CHAR, 0x00000244, MPI_SIGNED_CHAR);
  PREDEF(MPIABI_UNSIGNED_CHAR, 0x00000245, MPI_UNSIGNED_CHAR);
  PREDEF(MPIABI_BYTE, 0x00000247, MPI_BYTE);
  PREDEF(MPIABI_INT16_T, 0x00000248, MPI_INT16_T);
  PREDEF(MPIABI_UINT16_T, 0x00000249, MPI_UINT16_T);
  PREDEF(MPIABI_INT32_T, 0x00000250, MPI_INT32_T);
  PREDEF(MPIABI_UINT32_T, 0x00000251, MPI_UINT32_T);
  PREDEF(MPIABI_INT64_T, 0x00000258, MPI_INT64_T);
  PREDEF(MPIABI_UINT64_T, 0x00000259, MPI_UINT64_T);
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL1
  PREDEF(MPIABI_LOGICAL1, 0x000002c0, MPI_LOGICAL1);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER1
  PREDEF(MPIABI_INTEGER1, 0x000002c1, MPI_INTEGER1);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL2
  PREDEF(MPIABI_LOGICAL2, 0x000002c8, MPI_LOGICAL2);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER2
  PREDEF(MPIABI_INTEGER2, 0x000002c9, MPI_INTEGER2);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_REAL2
  PREDEF(MPIABI_REAL2, 0x000002ca, MPI_REAL2);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL4
  PREDEF(MPIABI_LOGICAL4, 0x000002d0, MPI_LOGICAL4);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER4
  PREDEF(MPIABI_INTEGER4, 0x000002d1, MPI_INTEGER4);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_REAL4
  PREDEF(MPIABI_REAL4, 0x000002d2, MPI_REAL4);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMPLEX4
  PREDEF(MPIABI_COMPLEX4, 0x000002d3, MPI_COMPLEX4);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL8
  PREDEF(MPIABI_LOGICAL8, 0x000002d8, MPI_LOGICAL8);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER8
  PREDEF(MPIABI_INTEGER8, 0x000002d9, MPI_INTEGER8);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_REAL8
  PREDEF(MPIABI_REAL8, 0x000002da, MPI_REAL8);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMPLEX8
  PREDEF(MPIABI_COMPLEX8, 0x000002db, MPI_COMPLEX8);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_LOGICAL16
  PREDEF(MPIABI_LOGICAL16, 0x000002e0, MPI_LOGICAL16);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_INTEGER16
  PREDEF(MPIABI_INTEGER16, 0x000002e1, MPI_INTEGER16);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_REAL16
  PREDEF(MPIABI_REAL16, 0x000002e2, MPI_REAL16);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMPLEX16
  PREDEF(MPIABI_COMPLEX16, 0x000002e3, MPI_COMPLEX16);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMPLEX32
  PREDEF(MPIABI_COMPLEX32, 0x000002eb, MPI_COMPLEX32);
#endif
  return n;
}

/* Thin shims so that test/mpiwrapper_selftest.c can walk every class through
 * one loop instead of eleven copies of the same five assertions.
 */

static uint64_t toabi_bits_op(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_op_toabi(MPIWRAPPER_HANDLE(MPI_Op, impl_bits)));
}

static uint64_t fromabi_bits_op(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_op_fromabi(MPIWRAPPER_HANDLE(MPIABI_Op, abi_bits)));
}

static uint64_t toabi_bits_comm(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_comm_toabi(MPIWRAPPER_HANDLE(MPI_Comm, impl_bits)));
}

static uint64_t fromabi_bits_comm(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_comm_fromabi(MPIWRAPPER_HANDLE(MPIABI_Comm, abi_bits)));
}

static uint64_t toabi_bits_group(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_group_toabi(MPIWRAPPER_HANDLE(MPI_Group, impl_bits)));
}

static uint64_t fromabi_bits_group(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_group_fromabi(MPIWRAPPER_HANDLE(MPIABI_Group, abi_bits)));
}

static uint64_t toabi_bits_win(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_win_toabi(MPIWRAPPER_HANDLE(MPI_Win, impl_bits)));
}

static uint64_t fromabi_bits_win(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_win_fromabi(MPIWRAPPER_HANDLE(MPIABI_Win, abi_bits)));
}

static uint64_t toabi_bits_file(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_file_toabi(MPIWRAPPER_HANDLE(MPI_File, impl_bits)));
}

static uint64_t fromabi_bits_file(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_file_fromabi(MPIWRAPPER_HANDLE(MPIABI_File, abi_bits)));
}

#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
static uint64_t toabi_bits_session(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_session_toabi(MPIWRAPPER_HANDLE(MPI_Session, impl_bits)));
}

static uint64_t fromabi_bits_session(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_session_fromabi(MPIWRAPPER_HANDLE(MPIABI_Session, abi_bits)));
}
#endif

static uint64_t toabi_bits_message(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_message_toabi(MPIWRAPPER_HANDLE(MPI_Message, impl_bits)));
}

static uint64_t fromabi_bits_message(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_message_fromabi(MPIWRAPPER_HANDLE(MPIABI_Message, abi_bits)));
}

static uint64_t toabi_bits_info(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_info_toabi(MPIWRAPPER_HANDLE(MPI_Info, impl_bits)));
}

static uint64_t fromabi_bits_info(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_info_fromabi(MPIWRAPPER_HANDLE(MPIABI_Info, abi_bits)));
}

static uint64_t toabi_bits_errhandler(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_errhandler_toabi(MPIWRAPPER_HANDLE(MPI_Errhandler, impl_bits)));
}

static uint64_t fromabi_bits_errhandler(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_errhandler_fromabi(MPIWRAPPER_HANDLE(MPIABI_Errhandler, abi_bits)));
}

static uint64_t toabi_bits_request(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_request_toabi(MPIWRAPPER_HANDLE(MPI_Request, impl_bits)));
}

static uint64_t fromabi_bits_request(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_request_fromabi(MPIWRAPPER_HANDLE(MPIABI_Request, abi_bits)));
}

static uint64_t toabi_bits_datatype(uint64_t impl_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_datatype_toabi(MPIWRAPPER_HANDLE(MPI_Datatype, impl_bits)));
}

static uint64_t fromabi_bits_datatype(uint64_t abi_bits)
{
  return MPIWRAPPER_BITS(mpiwrapper_datatype_fromabi(MPIWRAPPER_HANDLE(MPIABI_Datatype, abi_bits)));
}

/* The reverse maps, built once inside mpiwrapper_get_vtable before any slot
 * can be called. Failure here is fatal and reported through the diagnostic:
 * falling back to a probe loop would reintroduce the only data-dependent
 * branch the perfect hash exists to remove (NOTES.md #5.1).
 *
 * Each table is sized at eight times its key count, rounded up to a power of
 * two and never below eight, which is the headroom the perfect-hash search
 * needs to terminate quickly.
 */
static struct mpiwrapper_rmap_entry rmap_slots_op[128];
static struct mpiwrapper_rmap_entry rmap_slots_comm[32];
static struct mpiwrapper_rmap_entry rmap_slots_group[16];
static struct mpiwrapper_rmap_entry rmap_slots_win[8];
static struct mpiwrapper_rmap_entry rmap_slots_file[8];
#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
static struct mpiwrapper_rmap_entry rmap_slots_session[8];
#endif
static struct mpiwrapper_rmap_entry rmap_slots_message[16];
static struct mpiwrapper_rmap_entry rmap_slots_info[16];
static struct mpiwrapper_rmap_entry rmap_slots_errhandler[32];
static struct mpiwrapper_rmap_entry rmap_slots_request[8];
static struct mpiwrapper_rmap_entry rmap_slots_datatype[1024];

struct mpiwrapper_rmap mpiwrapper_rmap_op = {rmap_slots_op, 128, 0, 0, 0};
struct mpiwrapper_rmap mpiwrapper_rmap_comm = {rmap_slots_comm, 32, 0, 0, 0};
struct mpiwrapper_rmap mpiwrapper_rmap_group = {rmap_slots_group, 16, 0, 0, 0};
struct mpiwrapper_rmap mpiwrapper_rmap_win = {rmap_slots_win, 8, 0, 0, 0};
struct mpiwrapper_rmap mpiwrapper_rmap_file = {rmap_slots_file, 8, 0, 0, 0};
#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
struct mpiwrapper_rmap mpiwrapper_rmap_session = {rmap_slots_session, 8, 0, 0, 0};
#endif
struct mpiwrapper_rmap mpiwrapper_rmap_message = {rmap_slots_message, 16, 0, 0, 0};
struct mpiwrapper_rmap mpiwrapper_rmap_info = {rmap_slots_info, 16, 0, 0, 0};
struct mpiwrapper_rmap mpiwrapper_rmap_errhandler = {rmap_slots_errhandler, 32, 0, 0, 0};
struct mpiwrapper_rmap mpiwrapper_rmap_request = {rmap_slots_request, 8, 0, 0, 0};
struct mpiwrapper_rmap mpiwrapper_rmap_datatype = {rmap_slots_datatype, 1024, 0, 0, 0};

/* NULL-terminated; walked by the selftest. */
const struct mpiwrapper_predef_class mpiwrapper_predef_classes[] = {
    {"op", predef_op, toabi_bits_op, fromabi_bits_op,
     &mpiwrapper_rmap_op},
    {"comm", predef_comm, toabi_bits_comm, fromabi_bits_comm,
     &mpiwrapper_rmap_comm},
    {"group", predef_group, toabi_bits_group, fromabi_bits_group,
     &mpiwrapper_rmap_group},
    {"win", predef_win, toabi_bits_win, fromabi_bits_win,
     &mpiwrapper_rmap_win},
    {"file", predef_file, toabi_bits_file, fromabi_bits_file,
     &mpiwrapper_rmap_file},
#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
    {"session", predef_session, toabi_bits_session, fromabi_bits_session,
     &mpiwrapper_rmap_session},
#endif
    {"message", predef_message, toabi_bits_message, fromabi_bits_message,
     &mpiwrapper_rmap_message},
    {"info", predef_info, toabi_bits_info, fromabi_bits_info,
     &mpiwrapper_rmap_info},
    {"errhandler", predef_errhandler, toabi_bits_errhandler, fromabi_bits_errhandler,
     &mpiwrapper_rmap_errhandler},
    {"request", predef_request, toabi_bits_request, fromabi_bits_request,
     &mpiwrapper_rmap_request},
    {"datatype", predef_datatype, toabi_bits_datatype, fromabi_bits_datatype,
     &mpiwrapper_rmap_datatype},
    {NULL, NULL, NULL, NULL, NULL}};


int mpiwrapper_init_reverse_maps(const char **diagnostic)
{
  struct mpiwrapper_predef predef[128];
  uint64_t                 keys[128], abis[128];

  for (const struct mpiwrapper_predef_class *c = mpiwrapper_predef_classes;
       c->name; ++c) {
    const size_t n = c->fill(predef, sizeof predef / sizeof *predef);
    if (n > sizeof predef / sizeof *predef) {
      *diagnostic = "predefined-handle table larger than the build-time bound";
      return 0;
    }
    for (size_t i = 0; i < n; ++i) {
      keys[i] = predef[i].impl;
      abis[i] = predef[i].abi;
    }
    if (!mpiwrapper_rmap_build(c->map, keys, abis, n)) {
      *diagnostic = "could not construct a collision-free predefined-handle map";
      return 0;
    }
  }
  return 1;
}


/* Error codes. The common case is MPI_SUCCESS, which is 0 everywhere, so it
 * costs one compare. Codes handed out at run time by MPI_Add_error_class/
 * _code need renumbering rather than passing through (the ABI caps
 * MPI_ERR_LASTCODE at 16383 against MPICH's 0x3fffffff), so the default arm
 * hands off to the registry in src/mpiwrapper/errorcodes.c, whose only writers
 * are those two entry points. A code that registry never issued still answers
 * *_ERR_OTHER, which is a legal class for an error this ABI cannot name.
 */
int mpiwrapper_errorcode_toabi(int ierror)
{
  if (ierror == MPI_SUCCESS) return MPIABI_SUCCESS;
  switch (ierror) {
  case MPI_ERR_BUFFER: return MPIABI_ERR_BUFFER;
  case MPI_ERR_COUNT: return MPIABI_ERR_COUNT;
  case MPI_ERR_TYPE: return MPIABI_ERR_TYPE;
  case MPI_ERR_TAG: return MPIABI_ERR_TAG;
  case MPI_ERR_COMM: return MPIABI_ERR_COMM;
  case MPI_ERR_RANK: return MPIABI_ERR_RANK;
  case MPI_ERR_REQUEST: return MPIABI_ERR_REQUEST;
  case MPI_ERR_ROOT: return MPIABI_ERR_ROOT;
  case MPI_ERR_GROUP: return MPIABI_ERR_GROUP;
  case MPI_ERR_OP: return MPIABI_ERR_OP;
  case MPI_ERR_TOPOLOGY: return MPIABI_ERR_TOPOLOGY;
  case MPI_ERR_DIMS: return MPIABI_ERR_DIMS;
  case MPI_ERR_ARG: return MPIABI_ERR_ARG;
  case MPI_ERR_UNKNOWN: return MPIABI_ERR_UNKNOWN;
  case MPI_ERR_TRUNCATE: return MPIABI_ERR_TRUNCATE;
  case MPI_ERR_OTHER: return MPIABI_ERR_OTHER;
  case MPI_ERR_INTERN: return MPIABI_ERR_INTERN;
  case MPI_ERR_PENDING: return MPIABI_ERR_PENDING;
  case MPI_ERR_IN_STATUS: return MPIABI_ERR_IN_STATUS;
  case MPI_ERR_ACCESS: return MPIABI_ERR_ACCESS;
  case MPI_ERR_AMODE: return MPIABI_ERR_AMODE;
  case MPI_ERR_ASSERT: return MPIABI_ERR_ASSERT;
  case MPI_ERR_BAD_FILE: return MPIABI_ERR_BAD_FILE;
  case MPI_ERR_BASE: return MPIABI_ERR_BASE;
  case MPI_ERR_CONVERSION: return MPIABI_ERR_CONVERSION;
  case MPI_ERR_DISP: return MPIABI_ERR_DISP;
  case MPI_ERR_DUP_DATAREP: return MPIABI_ERR_DUP_DATAREP;
  case MPI_ERR_FILE_EXISTS: return MPIABI_ERR_FILE_EXISTS;
  case MPI_ERR_FILE_IN_USE: return MPIABI_ERR_FILE_IN_USE;
  case MPI_ERR_FILE: return MPIABI_ERR_FILE;
  case MPI_ERR_INFO_KEY: return MPIABI_ERR_INFO_KEY;
  case MPI_ERR_INFO_NOKEY: return MPIABI_ERR_INFO_NOKEY;
  case MPI_ERR_INFO_VALUE: return MPIABI_ERR_INFO_VALUE;
  case MPI_ERR_INFO: return MPIABI_ERR_INFO;
  case MPI_ERR_IO: return MPIABI_ERR_IO;
  case MPI_ERR_KEYVAL: return MPIABI_ERR_KEYVAL;
  case MPI_ERR_LOCKTYPE: return MPIABI_ERR_LOCKTYPE;
  case MPI_ERR_NAME: return MPIABI_ERR_NAME;
  case MPI_ERR_NO_MEM: return MPIABI_ERR_NO_MEM;
  case MPI_ERR_NOT_SAME: return MPIABI_ERR_NOT_SAME;
  case MPI_ERR_NO_SPACE: return MPIABI_ERR_NO_SPACE;
  case MPI_ERR_NO_SUCH_FILE: return MPIABI_ERR_NO_SUCH_FILE;
  case MPI_ERR_PORT: return MPIABI_ERR_PORT;
  case MPI_ERR_QUOTA: return MPIABI_ERR_QUOTA;
  case MPI_ERR_READ_ONLY: return MPIABI_ERR_READ_ONLY;
  case MPI_ERR_RMA_ATTACH: return MPIABI_ERR_RMA_ATTACH;
  case MPI_ERR_RMA_CONFLICT: return MPIABI_ERR_RMA_CONFLICT;
  case MPI_ERR_RMA_RANGE: return MPIABI_ERR_RMA_RANGE;
  case MPI_ERR_RMA_SHARED: return MPIABI_ERR_RMA_SHARED;
  case MPI_ERR_RMA_SYNC: return MPIABI_ERR_RMA_SYNC;
  case MPI_ERR_SERVICE: return MPIABI_ERR_SERVICE;
  case MPI_ERR_SIZE: return MPIABI_ERR_SIZE;
  case MPI_ERR_SPAWN: return MPIABI_ERR_SPAWN;
  case MPI_ERR_UNSUPPORTED_DATAREP: return MPIABI_ERR_UNSUPPORTED_DATAREP;
  case MPI_ERR_UNSUPPORTED_OPERATION: return MPIABI_ERR_UNSUPPORTED_OPERATION;
  case MPI_ERR_WIN: return MPIABI_ERR_WIN;
  case MPI_ERR_RMA_FLAVOR: return MPIABI_ERR_RMA_FLAVOR;
#ifdef MPIWRAPPER_HAVE_MPI_ERR_PROC_ABORTED
  case MPI_ERR_PROC_ABORTED: return MPIABI_ERR_PROC_ABORTED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_ERR_VALUE_TOO_LARGE
  case MPI_ERR_VALUE_TOO_LARGE: return MPIABI_ERR_VALUE_TOO_LARGE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_ERR_SESSION
  case MPI_ERR_SESSION: return MPIABI_ERR_SESSION;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_ERR_ERRHANDLER
  case MPI_ERR_ERRHANDLER: return MPIABI_ERR_ERRHANDLER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_ERR_ABI
  case MPI_ERR_ABI: return MPIABI_ERR_ABI;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_CANNOT_INIT
  case MPI_T_ERR_CANNOT_INIT: return MPIABI_T_ERR_CANNOT_INIT;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_NOT_ACCESSIBLE
  case MPI_T_ERR_NOT_ACCESSIBLE: return MPIABI_T_ERR_NOT_ACCESSIBLE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_NOT_INITIALIZED
  case MPI_T_ERR_NOT_INITIALIZED: return MPIABI_T_ERR_NOT_INITIALIZED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_NOT_SUPPORTED
  case MPI_T_ERR_NOT_SUPPORTED: return MPIABI_T_ERR_NOT_SUPPORTED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_MEMORY
  case MPI_T_ERR_MEMORY: return MPIABI_T_ERR_MEMORY;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID
  case MPI_T_ERR_INVALID: return MPIABI_T_ERR_INVALID;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_INDEX
  case MPI_T_ERR_INVALID_INDEX: return MPIABI_T_ERR_INVALID_INDEX;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_ITEM
  case MPI_T_ERR_INVALID_ITEM: return MPIABI_T_ERR_INVALID_ITEM;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_SESSION
  case MPI_T_ERR_INVALID_SESSION: return MPIABI_T_ERR_INVALID_SESSION;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_HANDLE
  case MPI_T_ERR_INVALID_HANDLE: return MPIABI_T_ERR_INVALID_HANDLE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_NAME
  case MPI_T_ERR_INVALID_NAME: return MPIABI_T_ERR_INVALID_NAME;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_OUT_OF_HANDLES
  case MPI_T_ERR_OUT_OF_HANDLES: return MPIABI_T_ERR_OUT_OF_HANDLES;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_OUT_OF_SESSIONS
  case MPI_T_ERR_OUT_OF_SESSIONS: return MPIABI_T_ERR_OUT_OF_SESSIONS;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_CVAR_SET_NOT_NOW
  case MPI_T_ERR_CVAR_SET_NOT_NOW: return MPIABI_T_ERR_CVAR_SET_NOT_NOW;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_CVAR_SET_NEVER
  case MPI_T_ERR_CVAR_SET_NEVER: return MPIABI_T_ERR_CVAR_SET_NEVER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_PVAR_NO_WRITE
  case MPI_T_ERR_PVAR_NO_WRITE: return MPIABI_T_ERR_PVAR_NO_WRITE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_PVAR_NO_STARTSTOP
  case MPI_T_ERR_PVAR_NO_STARTSTOP: return MPIABI_T_ERR_PVAR_NO_STARTSTOP;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_PVAR_NO_ATOMIC
  case MPI_T_ERR_PVAR_NO_ATOMIC: return MPIABI_T_ERR_PVAR_NO_ATOMIC;
#endif
  default: return mpiwrapper_errorcode_dynamic_toabi(ierror);
  }
}

int mpiwrapper_errorcode_fromabi(int abi_ierror)
{
  if (abi_ierror == MPIABI_SUCCESS) return MPI_SUCCESS;
  switch (abi_ierror) {
  case MPIABI_ERR_BUFFER: return MPI_ERR_BUFFER;
  case MPIABI_ERR_COUNT: return MPI_ERR_COUNT;
  case MPIABI_ERR_TYPE: return MPI_ERR_TYPE;
  case MPIABI_ERR_TAG: return MPI_ERR_TAG;
  case MPIABI_ERR_COMM: return MPI_ERR_COMM;
  case MPIABI_ERR_RANK: return MPI_ERR_RANK;
  case MPIABI_ERR_REQUEST: return MPI_ERR_REQUEST;
  case MPIABI_ERR_ROOT: return MPI_ERR_ROOT;
  case MPIABI_ERR_GROUP: return MPI_ERR_GROUP;
  case MPIABI_ERR_OP: return MPI_ERR_OP;
  case MPIABI_ERR_TOPOLOGY: return MPI_ERR_TOPOLOGY;
  case MPIABI_ERR_DIMS: return MPI_ERR_DIMS;
  case MPIABI_ERR_ARG: return MPI_ERR_ARG;
  case MPIABI_ERR_UNKNOWN: return MPI_ERR_UNKNOWN;
  case MPIABI_ERR_TRUNCATE: return MPI_ERR_TRUNCATE;
  case MPIABI_ERR_OTHER: return MPI_ERR_OTHER;
  case MPIABI_ERR_INTERN: return MPI_ERR_INTERN;
  case MPIABI_ERR_PENDING: return MPI_ERR_PENDING;
  case MPIABI_ERR_IN_STATUS: return MPI_ERR_IN_STATUS;
  case MPIABI_ERR_ACCESS: return MPI_ERR_ACCESS;
  case MPIABI_ERR_AMODE: return MPI_ERR_AMODE;
  case MPIABI_ERR_ASSERT: return MPI_ERR_ASSERT;
  case MPIABI_ERR_BAD_FILE: return MPI_ERR_BAD_FILE;
  case MPIABI_ERR_BASE: return MPI_ERR_BASE;
  case MPIABI_ERR_CONVERSION: return MPI_ERR_CONVERSION;
  case MPIABI_ERR_DISP: return MPI_ERR_DISP;
  case MPIABI_ERR_DUP_DATAREP: return MPI_ERR_DUP_DATAREP;
  case MPIABI_ERR_FILE_EXISTS: return MPI_ERR_FILE_EXISTS;
  case MPIABI_ERR_FILE_IN_USE: return MPI_ERR_FILE_IN_USE;
  case MPIABI_ERR_FILE: return MPI_ERR_FILE;
  case MPIABI_ERR_INFO_KEY: return MPI_ERR_INFO_KEY;
  case MPIABI_ERR_INFO_NOKEY: return MPI_ERR_INFO_NOKEY;
  case MPIABI_ERR_INFO_VALUE: return MPI_ERR_INFO_VALUE;
  case MPIABI_ERR_INFO: return MPI_ERR_INFO;
  case MPIABI_ERR_IO: return MPI_ERR_IO;
  case MPIABI_ERR_KEYVAL: return MPI_ERR_KEYVAL;
  case MPIABI_ERR_LOCKTYPE: return MPI_ERR_LOCKTYPE;
  case MPIABI_ERR_NAME: return MPI_ERR_NAME;
  case MPIABI_ERR_NO_MEM: return MPI_ERR_NO_MEM;
  case MPIABI_ERR_NOT_SAME: return MPI_ERR_NOT_SAME;
  case MPIABI_ERR_NO_SPACE: return MPI_ERR_NO_SPACE;
  case MPIABI_ERR_NO_SUCH_FILE: return MPI_ERR_NO_SUCH_FILE;
  case MPIABI_ERR_PORT: return MPI_ERR_PORT;
  case MPIABI_ERR_QUOTA: return MPI_ERR_QUOTA;
  case MPIABI_ERR_READ_ONLY: return MPI_ERR_READ_ONLY;
  case MPIABI_ERR_RMA_ATTACH: return MPI_ERR_RMA_ATTACH;
  case MPIABI_ERR_RMA_CONFLICT: return MPI_ERR_RMA_CONFLICT;
  case MPIABI_ERR_RMA_RANGE: return MPI_ERR_RMA_RANGE;
  case MPIABI_ERR_RMA_SHARED: return MPI_ERR_RMA_SHARED;
  case MPIABI_ERR_RMA_SYNC: return MPI_ERR_RMA_SYNC;
  case MPIABI_ERR_SERVICE: return MPI_ERR_SERVICE;
  case MPIABI_ERR_SIZE: return MPI_ERR_SIZE;
  case MPIABI_ERR_SPAWN: return MPI_ERR_SPAWN;
  case MPIABI_ERR_UNSUPPORTED_DATAREP: return MPI_ERR_UNSUPPORTED_DATAREP;
  case MPIABI_ERR_UNSUPPORTED_OPERATION: return MPI_ERR_UNSUPPORTED_OPERATION;
  case MPIABI_ERR_WIN: return MPI_ERR_WIN;
  case MPIABI_ERR_RMA_FLAVOR: return MPI_ERR_RMA_FLAVOR;
#ifdef MPIWRAPPER_HAVE_MPI_ERR_PROC_ABORTED
  case MPIABI_ERR_PROC_ABORTED: return MPI_ERR_PROC_ABORTED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_ERR_VALUE_TOO_LARGE
  case MPIABI_ERR_VALUE_TOO_LARGE: return MPI_ERR_VALUE_TOO_LARGE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_ERR_SESSION
  case MPIABI_ERR_SESSION: return MPI_ERR_SESSION;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_ERR_ERRHANDLER
  case MPIABI_ERR_ERRHANDLER: return MPI_ERR_ERRHANDLER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_ERR_ABI
  case MPIABI_ERR_ABI: return MPI_ERR_ABI;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_CANNOT_INIT
  case MPIABI_T_ERR_CANNOT_INIT: return MPI_T_ERR_CANNOT_INIT;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_NOT_ACCESSIBLE
  case MPIABI_T_ERR_NOT_ACCESSIBLE: return MPI_T_ERR_NOT_ACCESSIBLE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_NOT_INITIALIZED
  case MPIABI_T_ERR_NOT_INITIALIZED: return MPI_T_ERR_NOT_INITIALIZED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_NOT_SUPPORTED
  case MPIABI_T_ERR_NOT_SUPPORTED: return MPI_T_ERR_NOT_SUPPORTED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_MEMORY
  case MPIABI_T_ERR_MEMORY: return MPI_T_ERR_MEMORY;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID
  case MPIABI_T_ERR_INVALID: return MPI_T_ERR_INVALID;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_INDEX
  case MPIABI_T_ERR_INVALID_INDEX: return MPI_T_ERR_INVALID_INDEX;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_ITEM
  case MPIABI_T_ERR_INVALID_ITEM: return MPI_T_ERR_INVALID_ITEM;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_SESSION
  case MPIABI_T_ERR_INVALID_SESSION: return MPI_T_ERR_INVALID_SESSION;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_HANDLE
  case MPIABI_T_ERR_INVALID_HANDLE: return MPI_T_ERR_INVALID_HANDLE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_INVALID_NAME
  case MPIABI_T_ERR_INVALID_NAME: return MPI_T_ERR_INVALID_NAME;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_OUT_OF_HANDLES
  case MPIABI_T_ERR_OUT_OF_HANDLES: return MPI_T_ERR_OUT_OF_HANDLES;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_OUT_OF_SESSIONS
  case MPIABI_T_ERR_OUT_OF_SESSIONS: return MPI_T_ERR_OUT_OF_SESSIONS;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_CVAR_SET_NOT_NOW
  case MPIABI_T_ERR_CVAR_SET_NOT_NOW: return MPI_T_ERR_CVAR_SET_NOT_NOW;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_CVAR_SET_NEVER
  case MPIABI_T_ERR_CVAR_SET_NEVER: return MPI_T_ERR_CVAR_SET_NEVER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_PVAR_NO_WRITE
  case MPIABI_T_ERR_PVAR_NO_WRITE: return MPI_T_ERR_PVAR_NO_WRITE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_PVAR_NO_STARTSTOP
  case MPIABI_T_ERR_PVAR_NO_STARTSTOP: return MPI_T_ERR_PVAR_NO_STARTSTOP;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_ERR_PVAR_NO_ATOMIC
  case MPIABI_T_ERR_PVAR_NO_ATOMIC: return MPI_T_ERR_PVAR_NO_ATOMIC;
#endif
  default: return mpiwrapper_errorcode_dynamic_fromabi(abi_ierror);
  }
}


/* Ranks and tags are separate classes, and this is the reason: in the ABI,
 * MPI_ANY_TAG is -2 and MPI_PROC_NULL is -3; in MPICH both MPI_ANY_TAG and
 * MPI_PROC_NULL are -1. An int argument cannot be translated without knowing
 * its role, which is why the generator needs each parameter's kind from
 * apis.json and the header alone is insufficient (NOTES.md #5.4).
 *
 * A switch is fine *within* a role: the implementation's magic values are
 * distinct there (MPICH ranks are -1, -2, -3, -32766), so the case labels are
 * unique. It is only a combined rank-and-tag conversion that could not be
 * written as one.
 */
int mpiwrapper_rank_fromabi(int abi_rank)
{
  switch (abi_rank) {
  case MPIABI_ANY_SOURCE: return MPI_ANY_SOURCE;
  case MPIABI_PROC_NULL:  return MPI_PROC_NULL;
  case MPIABI_ROOT:       return MPI_ROOT;
  case MPIABI_UNDEFINED:  return MPI_UNDEFINED;
  default:           return abi_rank;
  }
}

int mpiwrapper_rank_toabi(int rank)
{
  switch (rank) {
  case MPI_ANY_SOURCE: return MPIABI_ANY_SOURCE;
  case MPI_PROC_NULL:  return MPIABI_PROC_NULL;
  case MPI_ROOT:       return MPIABI_ROOT;
  case MPI_UNDEFINED:  return MPIABI_UNDEFINED;
  default:        return rank;
  }
}

int mpiwrapper_tag_fromabi(int abi_tag)
{
  switch (abi_tag) {
  case MPIABI_ANY_TAG:   return MPI_ANY_TAG;
  case MPIABI_UNDEFINED: return MPI_UNDEFINED;
  default:          return abi_tag;
  }
}

int mpiwrapper_tag_toabi(int tag)
{
  switch (tag) {
  case MPI_ANY_TAG:   return MPIABI_ANY_TAG;
  case MPI_UNDEFINED: return MPIABI_UNDEFINED;
  default:       return tag;
  }
}


/* The mode bitmasks. MPI_MODE_* are OR-combined and the bit assignments are
 * unrelated to the ABI's: the ABI's RDONLY is 16 where both implementations
 * use 2, so this is a decomposition rather than a switch (NOTES.md #5.5).
 *
 * There are *two* mappers rather than the one NOTES.md #5.5 first described,
 * and Open MPI is why. The ABI puts file modes (1..256) and window asserts
 * (1024..16384) in one enum with disjoint bits, so on the way in one mapper
 * would do. On the way out it would not: Open MPI numbers its window asserts
 * MPI_MODE_NOCHECK=1, NOPRECEDE=2, NOPUT=4, NOSTORE=8, NOSUCCEED=16 -- exactly
 * the bits it also uses for MPI_MODE_CREATE, RDONLY, WRONLY, RDWR and
 * DELETE_ON_CLOSE. An implementation amode of 0x1 is CREATE or NOCHECK
 * depending on which parameter it came from, and nothing in the value says
 * which. (MPICH keeps the two families disjoint, so a single mapper
 * round-trips there and the bug would have shipped.) This is the same shape as
 * ranks and tags: the role is a property of the parameter, so it belongs in
 * the function name, and apis.json is what tells the generator which to emit.
 */
int mpiwrapper_filemode_fromabi(int abi_mode)
{
  int mode = 0;
  if (abi_mode & MPIABI_MODE_APPEND) mode |= MPI_MODE_APPEND;
  if (abi_mode & MPIABI_MODE_CREATE) mode |= MPI_MODE_CREATE;
  if (abi_mode & MPIABI_MODE_DELETE_ON_CLOSE) mode |= MPI_MODE_DELETE_ON_CLOSE;
  if (abi_mode & MPIABI_MODE_EXCL) mode |= MPI_MODE_EXCL;
  if (abi_mode & MPIABI_MODE_RDONLY) mode |= MPI_MODE_RDONLY;
  if (abi_mode & MPIABI_MODE_RDWR) mode |= MPI_MODE_RDWR;
  if (abi_mode & MPIABI_MODE_SEQUENTIAL) mode |= MPI_MODE_SEQUENTIAL;
  if (abi_mode & MPIABI_MODE_UNIQUE_OPEN) mode |= MPI_MODE_UNIQUE_OPEN;
  if (abi_mode & MPIABI_MODE_WRONLY) mode |= MPI_MODE_WRONLY;
  return mode;
}

int mpiwrapper_filemode_toabi(int mode)
{
  int abi_mode = 0;
  if (mode & MPI_MODE_APPEND) abi_mode |= MPIABI_MODE_APPEND;
  if (mode & MPI_MODE_CREATE) abi_mode |= MPIABI_MODE_CREATE;
  if (mode & MPI_MODE_DELETE_ON_CLOSE) abi_mode |= MPIABI_MODE_DELETE_ON_CLOSE;
  if (mode & MPI_MODE_EXCL) abi_mode |= MPIABI_MODE_EXCL;
  if (mode & MPI_MODE_RDONLY) abi_mode |= MPIABI_MODE_RDONLY;
  if (mode & MPI_MODE_RDWR) abi_mode |= MPIABI_MODE_RDWR;
  if (mode & MPI_MODE_SEQUENTIAL) abi_mode |= MPIABI_MODE_SEQUENTIAL;
  if (mode & MPI_MODE_UNIQUE_OPEN) abi_mode |= MPIABI_MODE_UNIQUE_OPEN;
  if (mode & MPI_MODE_WRONLY) abi_mode |= MPIABI_MODE_WRONLY;
  return abi_mode;
}

int mpiwrapper_winassert_fromabi(int abi_mode)
{
  int mode = 0;
  if (abi_mode & MPIABI_MODE_NOCHECK) mode |= MPI_MODE_NOCHECK;
  if (abi_mode & MPIABI_MODE_NOPRECEDE) mode |= MPI_MODE_NOPRECEDE;
  if (abi_mode & MPIABI_MODE_NOPUT) mode |= MPI_MODE_NOPUT;
  if (abi_mode & MPIABI_MODE_NOSTORE) mode |= MPI_MODE_NOSTORE;
  if (abi_mode & MPIABI_MODE_NOSUCCEED) mode |= MPI_MODE_NOSUCCEED;
  return mode;
}

int mpiwrapper_winassert_toabi(int mode)
{
  int abi_mode = 0;
  if (mode & MPI_MODE_NOCHECK) abi_mode |= MPIABI_MODE_NOCHECK;
  if (mode & MPI_MODE_NOPRECEDE) abi_mode |= MPIABI_MODE_NOPRECEDE;
  if (mode & MPI_MODE_NOPUT) abi_mode |= MPIABI_MODE_NOPUT;
  if (mode & MPI_MODE_NOSTORE) abi_mode |= MPIABI_MODE_NOSTORE;
  if (mode & MPI_MODE_NOSUCCEED) abi_mode |= MPIABI_MODE_NOSUCCEED;
  return abi_mode;
}


/* The remaining mapped integer constants, one family per apis.json kind.
 *
 * These are ordinary ints on both sides, so the case labels are symbolic; the
 * numeric-label rule applies to handles, where the ABI's spelling is a cast to
 * a pointer type.
 *
 * Only the members a conforming implementation may genuinely lack are guarded.
 * Everything else here is MPI-3.0 or older, so an implementation that really
 * lacks one fails the build naming it -- which is where these families would
 * otherwise be at their most dangerous, since a dropped case reaches the
 * default arm and passes an unmapped value through.
 */
int mpiwrapper_combiner_fromabi(int abi_combiner)
{
  switch (abi_combiner) {
  case MPIABI_COMBINER_NAMED:          return MPI_COMBINER_NAMED;
  case MPIABI_COMBINER_DUP:            return MPI_COMBINER_DUP;
  case MPIABI_COMBINER_CONTIGUOUS:     return MPI_COMBINER_CONTIGUOUS;
  case MPIABI_COMBINER_VECTOR:         return MPI_COMBINER_VECTOR;
  case MPIABI_COMBINER_HVECTOR:        return MPI_COMBINER_HVECTOR;
  case MPIABI_COMBINER_INDEXED:        return MPI_COMBINER_INDEXED;
  case MPIABI_COMBINER_HINDEXED:       return MPI_COMBINER_HINDEXED;
  case MPIABI_COMBINER_INDEXED_BLOCK:  return MPI_COMBINER_INDEXED_BLOCK;
  case MPIABI_COMBINER_HINDEXED_BLOCK: return MPI_COMBINER_HINDEXED_BLOCK;
  case MPIABI_COMBINER_STRUCT:         return MPI_COMBINER_STRUCT;
  case MPIABI_COMBINER_SUBARRAY:       return MPI_COMBINER_SUBARRAY;
  case MPIABI_COMBINER_DARRAY:         return MPI_COMBINER_DARRAY;
  case MPIABI_COMBINER_F90_REAL:       return MPI_COMBINER_F90_REAL;
  case MPIABI_COMBINER_F90_COMPLEX:    return MPI_COMBINER_F90_COMPLEX;
  case MPIABI_COMBINER_F90_INTEGER:    return MPI_COMBINER_F90_INTEGER;
  case MPIABI_COMBINER_RESIZED:        return MPI_COMBINER_RESIZED;
#ifdef MPIWRAPPER_HAVE_MPI_COMBINER_VALUE_INDEX /* added in MPI-4.1 */
  case MPIABI_COMBINER_VALUE_INDEX:    return MPI_COMBINER_VALUE_INDEX;
#endif
  default:                        return abi_combiner;
  }
}

int mpiwrapper_combiner_toabi(int combiner)
{
  switch (combiner) {
  case MPI_COMBINER_NAMED:          return MPIABI_COMBINER_NAMED;
  case MPI_COMBINER_DUP:            return MPIABI_COMBINER_DUP;
  case MPI_COMBINER_CONTIGUOUS:     return MPIABI_COMBINER_CONTIGUOUS;
  case MPI_COMBINER_VECTOR:         return MPIABI_COMBINER_VECTOR;
  case MPI_COMBINER_HVECTOR:        return MPIABI_COMBINER_HVECTOR;
  case MPI_COMBINER_INDEXED:        return MPIABI_COMBINER_INDEXED;
  case MPI_COMBINER_HINDEXED:       return MPIABI_COMBINER_HINDEXED;
  case MPI_COMBINER_INDEXED_BLOCK:  return MPIABI_COMBINER_INDEXED_BLOCK;
  case MPI_COMBINER_HINDEXED_BLOCK: return MPIABI_COMBINER_HINDEXED_BLOCK;
  case MPI_COMBINER_STRUCT:         return MPIABI_COMBINER_STRUCT;
  case MPI_COMBINER_SUBARRAY:       return MPIABI_COMBINER_SUBARRAY;
  case MPI_COMBINER_DARRAY:         return MPIABI_COMBINER_DARRAY;
  case MPI_COMBINER_F90_REAL:       return MPIABI_COMBINER_F90_REAL;
  case MPI_COMBINER_F90_COMPLEX:    return MPIABI_COMBINER_F90_COMPLEX;
  case MPI_COMBINER_F90_INTEGER:    return MPIABI_COMBINER_F90_INTEGER;
  case MPI_COMBINER_RESIZED:        return MPIABI_COMBINER_RESIZED;
#ifdef MPIWRAPPER_HAVE_MPI_COMBINER_VALUE_INDEX /* added in MPI-4.1 */
  case MPI_COMBINER_VALUE_INDEX:    return MPIABI_COMBINER_VALUE_INDEX;
#endif
  default:                     return combiner;
  }
}

int mpiwrapper_compare_fromabi(int abi_compare)
{
  switch (abi_compare) {
  case MPIABI_IDENT:     return MPI_IDENT;
  case MPIABI_CONGRUENT: return MPI_CONGRUENT;
  case MPIABI_SIMILAR:   return MPI_SIMILAR;
  case MPIABI_UNEQUAL:   return MPI_UNEQUAL;
  default:          return abi_compare;
  }
}

int mpiwrapper_compare_toabi(int compare)
{
  switch (compare) {
  case MPI_IDENT:     return MPIABI_IDENT;
  case MPI_CONGRUENT: return MPIABI_CONGRUENT;
  case MPI_SIMILAR:   return MPIABI_SIMILAR;
  case MPI_UNEQUAL:   return MPIABI_UNEQUAL;
  default:       return compare;
  }
}

int mpiwrapper_darg_fromabi(int abi_darg)
{
  switch (abi_darg) {
  case MPIABI_DISTRIBUTE_DFLT_DARG: return MPI_DISTRIBUTE_DFLT_DARG;
  default:                     return abi_darg;
  }
}

int mpiwrapper_darg_toabi(int darg)
{
  switch (darg) {
  case MPI_DISTRIBUTE_DFLT_DARG: return MPIABI_DISTRIBUTE_DFLT_DARG;
  default:                  return darg;
  }
}

int mpiwrapper_distribute_fromabi(int abi_distribute)
{
  switch (abi_distribute) {
  case MPIABI_DISTRIBUTE_NONE:   return MPI_DISTRIBUTE_NONE;
  case MPIABI_DISTRIBUTE_BLOCK:  return MPI_DISTRIBUTE_BLOCK;
  case MPIABI_DISTRIBUTE_CYCLIC: return MPI_DISTRIBUTE_CYCLIC;
  default:                  return abi_distribute;
  }
}

int mpiwrapper_distribute_toabi(int distribute)
{
  switch (distribute) {
  case MPI_DISTRIBUTE_NONE:   return MPIABI_DISTRIBUTE_NONE;
  case MPI_DISTRIBUTE_BLOCK:  return MPIABI_DISTRIBUTE_BLOCK;
  case MPI_DISTRIBUTE_CYCLIC: return MPIABI_DISTRIBUTE_CYCLIC;
  default:               return distribute;
  }
}

/* MPI_HOST: deprecated, and an implementation may say so on its own
 * declaration. A conversion table still has to name it.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
int mpiwrapper_keyval_fromabi(int abi_keyval)
{
  switch (abi_keyval) {
  case MPIABI_KEYVAL_INVALID:    return MPI_KEYVAL_INVALID;
  case MPIABI_TAG_UB:            return MPI_TAG_UB;
  case MPIABI_IO:                return MPI_IO;
  case MPIABI_HOST:              return MPI_HOST;
  case MPIABI_WTIME_IS_GLOBAL:   return MPI_WTIME_IS_GLOBAL;
  case MPIABI_APPNUM:            return MPI_APPNUM;
  case MPIABI_LASTUSEDCODE:      return MPI_LASTUSEDCODE;
  case MPIABI_UNIVERSE_SIZE:     return MPI_UNIVERSE_SIZE;
  case MPIABI_WIN_BASE:          return MPI_WIN_BASE;
  case MPIABI_WIN_DISP_UNIT:     return MPI_WIN_DISP_UNIT;
  case MPIABI_WIN_SIZE:          return MPI_WIN_SIZE;
  case MPIABI_WIN_CREATE_FLAVOR: return MPI_WIN_CREATE_FLAVOR;
  case MPIABI_WIN_MODEL:         return MPI_WIN_MODEL;
  default:                  return mpiwrapper_keyval_dynamic_fromabi(abi_keyval);
  }
}

int mpiwrapper_keyval_toabi(int keyval)
{
  switch (keyval) {
  case MPI_KEYVAL_INVALID:    return MPIABI_KEYVAL_INVALID;
  case MPI_TAG_UB:            return MPIABI_TAG_UB;
  case MPI_IO:                return MPIABI_IO;
  case MPI_HOST:              return MPIABI_HOST;
  case MPI_WTIME_IS_GLOBAL:   return MPIABI_WTIME_IS_GLOBAL;
  case MPI_APPNUM:            return MPIABI_APPNUM;
  case MPI_LASTUSEDCODE:      return MPIABI_LASTUSEDCODE;
  case MPI_UNIVERSE_SIZE:     return MPIABI_UNIVERSE_SIZE;
  case MPI_WIN_BASE:          return MPIABI_WIN_BASE;
  case MPI_WIN_DISP_UNIT:     return MPIABI_WIN_DISP_UNIT;
  case MPI_WIN_SIZE:          return MPIABI_WIN_SIZE;
  case MPI_WIN_CREATE_FLAVOR: return MPIABI_WIN_CREATE_FLAVOR;
  case MPI_WIN_MODEL:         return MPIABI_WIN_MODEL;
  default:               return mpiwrapper_keyval_dynamic_toabi(keyval);
  }
}

#pragma GCC diagnostic pop

int mpiwrapper_locktype_fromabi(int abi_locktype)
{
  switch (abi_locktype) {
  case MPIABI_LOCK_EXCLUSIVE: return MPI_LOCK_EXCLUSIVE;
  case MPIABI_LOCK_SHARED:    return MPI_LOCK_SHARED;
  default:               return abi_locktype;
  }
}

int mpiwrapper_locktype_toabi(int locktype)
{
  switch (locktype) {
  case MPI_LOCK_EXCLUSIVE: return MPIABI_LOCK_EXCLUSIVE;
  case MPI_LOCK_SHARED:    return MPIABI_LOCK_SHARED;
  default:            return locktype;
  }
}

int mpiwrapper_order_fromabi(int abi_order)
{
  switch (abi_order) {
  case MPIABI_ORDER_C:       return MPI_ORDER_C;
  case MPIABI_ORDER_FORTRAN: return MPI_ORDER_FORTRAN;
  default:              return abi_order;
  }
}

int mpiwrapper_order_toabi(int order)
{
  switch (order) {
  case MPI_ORDER_C:       return MPIABI_ORDER_C;
  case MPI_ORDER_FORTRAN: return MPIABI_ORDER_FORTRAN;
  default:           return order;
  }
}

int mpiwrapper_seek_fromabi(int abi_seek)
{
  switch (abi_seek) {
  case MPIABI_SEEK_CUR: return MPI_SEEK_CUR;
  case MPIABI_SEEK_END: return MPI_SEEK_END;
  case MPIABI_SEEK_SET: return MPI_SEEK_SET;
  default:         return abi_seek;
  }
}

int mpiwrapper_seek_toabi(int seek)
{
  switch (seek) {
  case MPI_SEEK_CUR: return MPIABI_SEEK_CUR;
  case MPI_SEEK_END: return MPIABI_SEEK_END;
  case MPI_SEEK_SET: return MPIABI_SEEK_SET;
  default:      return seek;
  }
}

int mpiwrapper_splittype_fromabi(int abi_splittype)
{
  switch (abi_splittype) {
  case MPIABI_COMM_TYPE_SHARED:          return MPI_COMM_TYPE_SHARED;
#ifdef MPIWRAPPER_HAVE_MPI_COMM_TYPE_HW_UNGUIDED /* added in MPI-4.0 */
  case MPIABI_COMM_TYPE_HW_UNGUIDED:     return MPI_COMM_TYPE_HW_UNGUIDED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMM_TYPE_HW_GUIDED /* added in MPI-4.0 */
  case MPIABI_COMM_TYPE_HW_GUIDED:       return MPI_COMM_TYPE_HW_GUIDED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMM_TYPE_RESOURCE_GUIDED /* added in MPI-4.1 */
  case MPIABI_COMM_TYPE_RESOURCE_GUIDED: return MPI_COMM_TYPE_RESOURCE_GUIDED;
#endif
  default:                          return abi_splittype;
  }
}

int mpiwrapper_splittype_toabi(int splittype)
{
  switch (splittype) {
  case MPI_COMM_TYPE_SHARED:          return MPIABI_COMM_TYPE_SHARED;
#ifdef MPIWRAPPER_HAVE_MPI_COMM_TYPE_HW_UNGUIDED /* added in MPI-4.0 */
  case MPI_COMM_TYPE_HW_UNGUIDED:     return MPIABI_COMM_TYPE_HW_UNGUIDED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMM_TYPE_HW_GUIDED /* added in MPI-4.0 */
  case MPI_COMM_TYPE_HW_GUIDED:       return MPIABI_COMM_TYPE_HW_GUIDED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_COMM_TYPE_RESOURCE_GUIDED /* added in MPI-4.1 */
  case MPI_COMM_TYPE_RESOURCE_GUIDED: return MPIABI_COMM_TYPE_RESOURCE_GUIDED;
#endif
  default:                       return splittype;
  }
}

int mpiwrapper_tbind_fromabi(int abi_tbind)
{
  switch (abi_tbind) {
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_NO_OBJECT /* MPI_T is optional in full */
  case MPIABI_T_BIND_NO_OBJECT:      return MPI_T_BIND_NO_OBJECT;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_COMM /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_COMM:       return MPI_T_BIND_MPI_COMM;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_DATATYPE /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_DATATYPE:   return MPI_T_BIND_MPI_DATATYPE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_ERRHANDLER /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_ERRHANDLER: return MPI_T_BIND_MPI_ERRHANDLER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_FILE /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_FILE:       return MPI_T_BIND_MPI_FILE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_GROUP /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_GROUP:      return MPI_T_BIND_MPI_GROUP;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_OP /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_OP:         return MPI_T_BIND_MPI_OP;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_REQUEST /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_REQUEST:    return MPI_T_BIND_MPI_REQUEST;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_WIN /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_WIN:        return MPI_T_BIND_MPI_WIN;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_MESSAGE /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_MESSAGE:    return MPI_T_BIND_MPI_MESSAGE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_INFO /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_INFO:       return MPI_T_BIND_MPI_INFO;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_SESSION /* MPI_T is optional in full */
  case MPIABI_T_BIND_MPI_SESSION:    return MPI_T_BIND_MPI_SESSION;
#endif
  default:                      return abi_tbind;
  }
}

int mpiwrapper_tbind_toabi(int tbind)
{
  switch (tbind) {
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_NO_OBJECT /* MPI_T is optional in full */
  case MPI_T_BIND_NO_OBJECT:      return MPIABI_T_BIND_NO_OBJECT;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_COMM /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_COMM:       return MPIABI_T_BIND_MPI_COMM;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_DATATYPE /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_DATATYPE:   return MPIABI_T_BIND_MPI_DATATYPE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_ERRHANDLER /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_ERRHANDLER: return MPIABI_T_BIND_MPI_ERRHANDLER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_FILE /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_FILE:       return MPIABI_T_BIND_MPI_FILE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_GROUP /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_GROUP:      return MPIABI_T_BIND_MPI_GROUP;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_OP /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_OP:         return MPIABI_T_BIND_MPI_OP;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_REQUEST /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_REQUEST:    return MPIABI_T_BIND_MPI_REQUEST;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_WIN /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_WIN:        return MPIABI_T_BIND_MPI_WIN;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_MESSAGE /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_MESSAGE:    return MPIABI_T_BIND_MPI_MESSAGE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_INFO /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_INFO:       return MPIABI_T_BIND_MPI_INFO;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_BIND_MPI_SESSION /* MPI_T is optional in full */
  case MPI_T_BIND_MPI_SESSION:    return MPIABI_T_BIND_MPI_SESSION;
#endif
  default:                   return tbind;
  }
}

int mpiwrapper_tcbsafety_fromabi(int abi_tcbsafety)
{
  switch (abi_tcbsafety) {
#ifdef MPIWRAPPER_HAVE_MPI_T_CB_REQUIRE_NONE /* MPI_T is optional in full */
  case MPIABI_T_CB_REQUIRE_NONE:              return MPI_T_CB_REQUIRE_NONE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_CB_REQUIRE_MPI_RESTRICTED /* MPI_T is optional in full */
  case MPIABI_T_CB_REQUIRE_MPI_RESTRICTED:    return MPI_T_CB_REQUIRE_MPI_RESTRICTED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_CB_REQUIRE_THREAD_SAFE /* MPI_T is optional in full */
  case MPIABI_T_CB_REQUIRE_THREAD_SAFE:       return MPI_T_CB_REQUIRE_THREAD_SAFE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE /* MPI_T is optional in full */
  case MPIABI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE: return MPI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE;
#endif
  default:                               return abi_tcbsafety;
  }
}

int mpiwrapper_tcbsafety_toabi(int tcbsafety)
{
  switch (tcbsafety) {
#ifdef MPIWRAPPER_HAVE_MPI_T_CB_REQUIRE_NONE /* MPI_T is optional in full */
  case MPI_T_CB_REQUIRE_NONE:              return MPIABI_T_CB_REQUIRE_NONE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_CB_REQUIRE_MPI_RESTRICTED /* MPI_T is optional in full */
  case MPI_T_CB_REQUIRE_MPI_RESTRICTED:    return MPIABI_T_CB_REQUIRE_MPI_RESTRICTED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_CB_REQUIRE_THREAD_SAFE /* MPI_T is optional in full */
  case MPI_T_CB_REQUIRE_THREAD_SAFE:       return MPIABI_T_CB_REQUIRE_THREAD_SAFE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE /* MPI_T is optional in full */
  case MPI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE: return MPIABI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE;
#endif
  default:                            return tcbsafety;
  }
}

int mpiwrapper_threadlevel_fromabi(int abi_threadlevel)
{
  switch (abi_threadlevel) {
  case MPIABI_THREAD_SINGLE:     return MPI_THREAD_SINGLE;
  case MPIABI_THREAD_FUNNELED:   return MPI_THREAD_FUNNELED;
  case MPIABI_THREAD_SERIALIZED: return MPI_THREAD_SERIALIZED;
  case MPIABI_THREAD_MULTIPLE:   return MPI_THREAD_MULTIPLE;
  default:                  return abi_threadlevel;
  }
}

int mpiwrapper_threadlevel_toabi(int threadlevel)
{
  switch (threadlevel) {
  case MPI_THREAD_SINGLE:     return MPIABI_THREAD_SINGLE;
  case MPI_THREAD_FUNNELED:   return MPIABI_THREAD_FUNNELED;
  case MPI_THREAD_SERIALIZED: return MPIABI_THREAD_SERIALIZED;
  case MPI_THREAD_MULTIPLE:   return MPIABI_THREAD_MULTIPLE;
  default:               return threadlevel;
  }
}

int mpiwrapper_topology_fromabi(int abi_topology)
{
  switch (abi_topology) {
  case MPIABI_UNDEFINED:  return MPI_UNDEFINED;
  case MPIABI_CART:       return MPI_CART;
  case MPIABI_GRAPH:      return MPI_GRAPH;
  case MPIABI_DIST_GRAPH: return MPI_DIST_GRAPH;
  default:           return abi_topology;
  }
}

int mpiwrapper_topology_toabi(int topology)
{
  switch (topology) {
  case MPI_UNDEFINED:  return MPIABI_UNDEFINED;
  case MPI_CART:       return MPIABI_CART;
  case MPI_GRAPH:      return MPIABI_GRAPH;
  case MPI_DIST_GRAPH: return MPIABI_DIST_GRAPH;
  default:        return topology;
  }
}

int mpiwrapper_tpvarclass_fromabi(int abi_tpvarclass)
{
  switch (abi_tpvarclass) {
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_STATE /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_STATE:         return MPI_T_PVAR_CLASS_STATE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_LEVEL /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_LEVEL:         return MPI_T_PVAR_CLASS_LEVEL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_SIZE /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_SIZE:          return MPI_T_PVAR_CLASS_SIZE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_PERCENTAGE /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_PERCENTAGE:    return MPI_T_PVAR_CLASS_PERCENTAGE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_HIGHWATERMARK /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_HIGHWATERMARK: return MPI_T_PVAR_CLASS_HIGHWATERMARK;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_LOWWATERMARK /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_LOWWATERMARK:  return MPI_T_PVAR_CLASS_LOWWATERMARK;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_COUNTER /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_COUNTER:       return MPI_T_PVAR_CLASS_COUNTER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_AGGREGATE /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_AGGREGATE:     return MPI_T_PVAR_CLASS_AGGREGATE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_TIMER /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_TIMER:         return MPI_T_PVAR_CLASS_TIMER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_GENERIC /* MPI_T is optional in full */
  case MPIABI_T_PVAR_CLASS_GENERIC:       return MPI_T_PVAR_CLASS_GENERIC;
#endif
  default:                           return abi_tpvarclass;
  }
}

int mpiwrapper_tpvarclass_toabi(int tpvarclass)
{
  switch (tpvarclass) {
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_STATE /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_STATE:         return MPIABI_T_PVAR_CLASS_STATE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_LEVEL /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_LEVEL:         return MPIABI_T_PVAR_CLASS_LEVEL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_SIZE /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_SIZE:          return MPIABI_T_PVAR_CLASS_SIZE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_PERCENTAGE /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_PERCENTAGE:    return MPIABI_T_PVAR_CLASS_PERCENTAGE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_HIGHWATERMARK /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_HIGHWATERMARK: return MPIABI_T_PVAR_CLASS_HIGHWATERMARK;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_LOWWATERMARK /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_LOWWATERMARK:  return MPIABI_T_PVAR_CLASS_LOWWATERMARK;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_COUNTER /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_COUNTER:       return MPIABI_T_PVAR_CLASS_COUNTER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_AGGREGATE /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_AGGREGATE:     return MPIABI_T_PVAR_CLASS_AGGREGATE;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_TIMER /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_TIMER:         return MPIABI_T_PVAR_CLASS_TIMER;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_PVAR_CLASS_GENERIC /* MPI_T is optional in full */
  case MPI_T_PVAR_CLASS_GENERIC:       return MPIABI_T_PVAR_CLASS_GENERIC;
#endif
  default:                        return tpvarclass;
  }
}

int mpiwrapper_tscope_fromabi(int abi_tscope)
{
  switch (abi_tscope) {
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_CONSTANT /* MPI_T is optional in full */
  case MPIABI_T_SCOPE_CONSTANT: return MPI_T_SCOPE_CONSTANT;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_READONLY /* MPI_T is optional in full */
  case MPIABI_T_SCOPE_READONLY: return MPI_T_SCOPE_READONLY;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_LOCAL /* MPI_T is optional in full */
  case MPIABI_T_SCOPE_LOCAL:    return MPI_T_SCOPE_LOCAL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_GROUP /* MPI_T is optional in full */
  case MPIABI_T_SCOPE_GROUP:    return MPI_T_SCOPE_GROUP;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_GROUP_EQ /* MPI_T is optional in full */
  case MPIABI_T_SCOPE_GROUP_EQ: return MPI_T_SCOPE_GROUP_EQ;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_ALL /* MPI_T is optional in full */
  case MPIABI_T_SCOPE_ALL:      return MPI_T_SCOPE_ALL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_ALL_EQ /* MPI_T is optional in full */
  case MPIABI_T_SCOPE_ALL_EQ:   return MPI_T_SCOPE_ALL_EQ;
#endif
  default:                 return abi_tscope;
  }
}

int mpiwrapper_tscope_toabi(int tscope)
{
  switch (tscope) {
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_CONSTANT /* MPI_T is optional in full */
  case MPI_T_SCOPE_CONSTANT: return MPIABI_T_SCOPE_CONSTANT;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_READONLY /* MPI_T is optional in full */
  case MPI_T_SCOPE_READONLY: return MPIABI_T_SCOPE_READONLY;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_LOCAL /* MPI_T is optional in full */
  case MPI_T_SCOPE_LOCAL:    return MPIABI_T_SCOPE_LOCAL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_GROUP /* MPI_T is optional in full */
  case MPI_T_SCOPE_GROUP:    return MPIABI_T_SCOPE_GROUP;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_GROUP_EQ /* MPI_T is optional in full */
  case MPI_T_SCOPE_GROUP_EQ: return MPIABI_T_SCOPE_GROUP_EQ;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_ALL /* MPI_T is optional in full */
  case MPI_T_SCOPE_ALL:      return MPIABI_T_SCOPE_ALL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SCOPE_ALL_EQ /* MPI_T is optional in full */
  case MPI_T_SCOPE_ALL_EQ:   return MPIABI_T_SCOPE_ALL_EQ;
#endif
  default:              return tscope;
  }
}

int mpiwrapper_tsourceorder_fromabi(int abi_tsourceorder)
{
  switch (abi_tsourceorder) {
#ifdef MPIWRAPPER_HAVE_MPI_T_SOURCE_ORDERED /* MPI_T is optional in full */
  case MPIABI_T_SOURCE_ORDERED:   return MPI_T_SOURCE_ORDERED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SOURCE_UNORDERED /* MPI_T is optional in full */
  case MPIABI_T_SOURCE_UNORDERED: return MPI_T_SOURCE_UNORDERED;
#endif
  default:                   return abi_tsourceorder;
  }
}

int mpiwrapper_tsourceorder_toabi(int tsourceorder)
{
  switch (tsourceorder) {
#ifdef MPIWRAPPER_HAVE_MPI_T_SOURCE_ORDERED /* MPI_T is optional in full */
  case MPI_T_SOURCE_ORDERED:   return MPIABI_T_SOURCE_ORDERED;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_SOURCE_UNORDERED /* MPI_T is optional in full */
  case MPI_T_SOURCE_UNORDERED: return MPIABI_T_SOURCE_UNORDERED;
#endif
  default:                return tsourceorder;
  }
}

int mpiwrapper_tverbosity_fromabi(int abi_tverbosity)
{
  switch (abi_tverbosity) {
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_USER_BASIC /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_USER_BASIC:    return MPI_T_VERBOSITY_USER_BASIC;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_USER_DETAIL /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_USER_DETAIL:   return MPI_T_VERBOSITY_USER_DETAIL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_USER_ALL /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_USER_ALL:      return MPI_T_VERBOSITY_USER_ALL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_TUNER_BASIC /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_TUNER_BASIC:   return MPI_T_VERBOSITY_TUNER_BASIC;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_TUNER_DETAIL /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_TUNER_DETAIL:  return MPI_T_VERBOSITY_TUNER_DETAIL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_TUNER_ALL /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_TUNER_ALL:     return MPI_T_VERBOSITY_TUNER_ALL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_MPIDEV_BASIC /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_MPIDEV_BASIC:  return MPI_T_VERBOSITY_MPIDEV_BASIC;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_MPIDEV_DETAIL /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_MPIDEV_DETAIL: return MPI_T_VERBOSITY_MPIDEV_DETAIL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_MPIDEV_ALL /* MPI_T is optional in full */
  case MPIABI_T_VERBOSITY_MPIDEV_ALL:    return MPI_T_VERBOSITY_MPIDEV_ALL;
#endif
  default:                          return abi_tverbosity;
  }
}

int mpiwrapper_tverbosity_toabi(int tverbosity)
{
  switch (tverbosity) {
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_USER_BASIC /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_USER_BASIC:    return MPIABI_T_VERBOSITY_USER_BASIC;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_USER_DETAIL /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_USER_DETAIL:   return MPIABI_T_VERBOSITY_USER_DETAIL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_USER_ALL /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_USER_ALL:      return MPIABI_T_VERBOSITY_USER_ALL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_TUNER_BASIC /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_TUNER_BASIC:   return MPIABI_T_VERBOSITY_TUNER_BASIC;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_TUNER_DETAIL /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_TUNER_DETAIL:  return MPIABI_T_VERBOSITY_TUNER_DETAIL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_TUNER_ALL /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_TUNER_ALL:     return MPIABI_T_VERBOSITY_TUNER_ALL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_MPIDEV_BASIC /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_MPIDEV_BASIC:  return MPIABI_T_VERBOSITY_MPIDEV_BASIC;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_MPIDEV_DETAIL /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_MPIDEV_DETAIL: return MPIABI_T_VERBOSITY_MPIDEV_DETAIL;
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_VERBOSITY_MPIDEV_ALL /* MPI_T is optional in full */
  case MPI_T_VERBOSITY_MPIDEV_ALL:    return MPIABI_T_VERBOSITY_MPIDEV_ALL;
#endif
  default:                       return tverbosity;
  }
}

int mpiwrapper_typeclass_fromabi(int abi_typeclass)
{
  switch (abi_typeclass) {
#ifdef MPIWRAPPER_HAVE_MPIX_TYPECLASS_LOGICAL /* a legacy alias the ABI header carries; not a standard name */
  case MPIABIX_TYPECLASS_LOGICAL: return MPIX_TYPECLASS_LOGICAL;
#endif
  case MPIABI_TYPECLASS_INTEGER:  return MPI_TYPECLASS_INTEGER;
  case MPIABI_TYPECLASS_REAL:     return MPI_TYPECLASS_REAL;
  case MPIABI_TYPECLASS_COMPLEX:  return MPI_TYPECLASS_COMPLEX;
  default:                   return abi_typeclass;
  }
}

int mpiwrapper_typeclass_toabi(int typeclass)
{
  switch (typeclass) {
#ifdef MPIWRAPPER_HAVE_MPIX_TYPECLASS_LOGICAL /* a legacy alias the ABI header carries; not a standard name */
  case MPIX_TYPECLASS_LOGICAL: return MPIABIX_TYPECLASS_LOGICAL;
#endif
  case MPI_TYPECLASS_INTEGER:  return MPIABI_TYPECLASS_INTEGER;
  case MPI_TYPECLASS_REAL:     return MPIABI_TYPECLASS_REAL;
  case MPI_TYPECLASS_COMPLEX:  return MPIABI_TYPECLASS_COMPLEX;
  default:                return typeclass;
  }
}

int mpiwrapper_winflavor_fromabi(int abi_winflavor)
{
  switch (abi_winflavor) {
  case MPIABI_WIN_FLAVOR_CREATE:   return MPI_WIN_FLAVOR_CREATE;
  case MPIABI_WIN_FLAVOR_ALLOCATE: return MPI_WIN_FLAVOR_ALLOCATE;
  case MPIABI_WIN_FLAVOR_DYNAMIC:  return MPI_WIN_FLAVOR_DYNAMIC;
  case MPIABI_WIN_FLAVOR_SHARED:   return MPI_WIN_FLAVOR_SHARED;
  default:                    return abi_winflavor;
  }
}

int mpiwrapper_winflavor_toabi(int winflavor)
{
  switch (winflavor) {
  case MPI_WIN_FLAVOR_CREATE:   return MPIABI_WIN_FLAVOR_CREATE;
  case MPI_WIN_FLAVOR_ALLOCATE: return MPIABI_WIN_FLAVOR_ALLOCATE;
  case MPI_WIN_FLAVOR_DYNAMIC:  return MPIABI_WIN_FLAVOR_DYNAMIC;
  case MPI_WIN_FLAVOR_SHARED:   return MPIABI_WIN_FLAVOR_SHARED;
  default:                 return winflavor;
  }
}

int mpiwrapper_winmodel_fromabi(int abi_winmodel)
{
  switch (abi_winmodel) {
  case MPIABI_WIN_UNIFIED:  return MPI_WIN_UNIFIED;
  case MPIABI_WIN_SEPARATE: return MPI_WIN_SEPARATE;
  default:             return abi_winmodel;
  }
}

int mpiwrapper_winmodel_toabi(int winmodel)
{
  switch (winmodel) {
  case MPI_WIN_UNIFIED:  return MPIABI_WIN_UNIFIED;
  case MPI_WIN_SEPARATE: return MPIABI_WIN_SEPARATE;
  default:          return winmodel;
  }
}


/* Sentinels. MPI_BOTTOM is (void *)0 in the ABI and in both implementations,
 * so that arm is an identity today -- it is emitted anyway, because the ABI
 * fixes the value and an implementation is free not to, and a site that omits
 * the test is invisible until that implementation appears. MPI_IN_PLACE is not
 * identity: MPICH's is (void *)-1.
 */

const void *mpiwrapper_sendbuf_fromabi(const void *abi_buf)
{
  if (abi_buf == MPIABI_BOTTOM) return MPI_BOTTOM;
  return abi_buf;
}

const void *mpiwrapper_sendbuf_inplace_fromabi(const void *abi_buf)
{
  if (abi_buf == MPIABI_BOTTOM) return MPI_BOTTOM;
  if (abi_buf == MPIABI_IN_PLACE) return MPI_IN_PLACE;
  return abi_buf;
}

void *mpiwrapper_recvbuf_fromabi(void *abi_buf)
{
  if (abi_buf == MPIABI_BOTTOM) return MPI_BOTTOM;
  return abi_buf;
}

/* The in-place receive buffer is MPI_Scatter's and MPI_Scatterv's, where the
 * root passes MPI_IN_PLACE for what it would otherwise receive into. S1 had
 * only three of these because its prototype had no such call.
 */
void *mpiwrapper_recvbuf_inplace_fromabi(void *abi_buf)
{
  if (abi_buf == MPIABI_BOTTOM) return MPI_BOTTOM;
  if (abi_buf == MPIABI_IN_PLACE) return MPI_IN_PLACE;
  return abi_buf;
}

/* The graph-topology weights. The ABI fixes MPI_UNWEIGHTED at (int *)10 and
 * MPI_WEIGHTS_EMPTY at (int *)11; both implementations spell them as objects
 * whose address is not a build-time constant, which is why this is a run-time
 * test like every other sentinel and not a table. The weights themselves are
 * plain ints and cross unconverted -- only the two pointers mean anything.
 *
 * The out form is MPI_Dist_graph_neighbors', where the caller passes
 * MPI_UNWEIGHTED to say it does not want the weights back.
 */
const int *mpiwrapper_weights_fromabi(const int *abi_weights)
{
  if (abi_weights == MPIABI_UNWEIGHTED) return MPI_UNWEIGHTED;
  if (abi_weights == MPIABI_WEIGHTS_EMPTY) return MPI_WEIGHTS_EMPTY;
  return abi_weights;
}

int *mpiwrapper_weights_out_fromabi(int *abi_weights)
{
  if (abi_weights == MPIABI_UNWEIGHTED) return MPI_UNWEIGHTED;
  if (abi_weights == MPIABI_WEIGHTS_EMPTY) return MPI_WEIGHTS_EMPTY;
  return abi_weights;
}


/* MPI_T's six handle classes.
 *
 * Not the eleven of NOTES.md #5.1 and deliberately not their machinery: those
 * have up to 103 predefined values apiece, spelled in the implementation as
 * addresses that are not compile-time constants, which is what the perfect-hash
 * reverse map exists for. These have at most two apiece, so each direction is
 * one or two compares -- #5.3's sentinel shape rather than #5.1's map shape.
 *
 * The sentinels genuinely differ, which is why this is not a bit-cast: the ABI
 * fixes MPI_T_PVAR_ALL_HANDLES at 1, Open MPI 5.0.6 spells it -1, and MPICH
 * 4.3.1 makes it an `extern ... * const` whose value is not a constant
 * expression at all -- so it could not be a case label even if one were wanted,
 * and both directions have to be run-time compares.
 *
 * A dynamic implementation handle whose bits land on 0 or 1 would be read back
 * as a sentinel, so the toabi direction rejects it exactly as #5.1 does: the
 * class's null handle plus the flag mpiwrapper_take_handle_error() reports. No
 * implementation can produce one -- these are object addresses on both -- and
 * the check is one compare on a path that allocates a tool handle.
 */

#ifdef MPIWRAPPER_HAVE_MPI_T_enum
MPI_T_enum mpiwrapper_t_enum_fromabi(MPIABI_T_enum abi)
{
  if (abi == MPIABI_T_ENUM_NULL) return MPI_T_ENUM_NULL;
  return MPIWRAPPER_HANDLE(MPI_T_enum, MPIWRAPPER_BITS(abi));
}

MPIABI_T_enum mpiwrapper_t_enum_toabi(MPI_T_enum h)
{
  if (h == MPI_T_ENUM_NULL) return MPIABI_T_ENUM_NULL;
  if (MPIWRAPPER_BITS(h) <= MPIWRAPPER_TOOL_PREDEF_LAST) {
    mpiwrapper_set_handle_error();
    return MPIABI_T_ENUM_NULL;
  }
  return MPIWRAPPER_HANDLE(MPIABI_T_enum, MPIWRAPPER_BITS(h));
}
#endif

#ifdef MPIWRAPPER_HAVE_MPI_T_cvar_handle
MPI_T_cvar_handle mpiwrapper_t_cvar_handle_fromabi(MPIABI_T_cvar_handle abi)
{
  if (abi == MPIABI_T_CVAR_HANDLE_NULL) return MPI_T_CVAR_HANDLE_NULL;
  return MPIWRAPPER_HANDLE(MPI_T_cvar_handle, MPIWRAPPER_BITS(abi));
}

MPIABI_T_cvar_handle mpiwrapper_t_cvar_handle_toabi(MPI_T_cvar_handle h)
{
  if (h == MPI_T_CVAR_HANDLE_NULL) return MPIABI_T_CVAR_HANDLE_NULL;
  if (MPIWRAPPER_BITS(h) <= MPIWRAPPER_TOOL_PREDEF_LAST) {
    mpiwrapper_set_handle_error();
    return MPIABI_T_CVAR_HANDLE_NULL;
  }
  return MPIWRAPPER_HANDLE(MPIABI_T_cvar_handle, MPIWRAPPER_BITS(h));
}
#endif

#ifdef MPIWRAPPER_HAVE_MPI_T_pvar_handle
MPI_T_pvar_handle mpiwrapper_t_pvar_handle_fromabi(MPIABI_T_pvar_handle abi)
{
  if (abi == MPIABI_T_PVAR_HANDLE_NULL) return MPI_T_PVAR_HANDLE_NULL;
  if (abi == MPIABI_T_PVAR_ALL_HANDLES) return MPI_T_PVAR_ALL_HANDLES;
  return MPIWRAPPER_HANDLE(MPI_T_pvar_handle, MPIWRAPPER_BITS(abi));
}

MPIABI_T_pvar_handle mpiwrapper_t_pvar_handle_toabi(MPI_T_pvar_handle h)
{
  if (h == MPI_T_PVAR_HANDLE_NULL) return MPIABI_T_PVAR_HANDLE_NULL;
  if (h == MPI_T_PVAR_ALL_HANDLES) return MPIABI_T_PVAR_ALL_HANDLES;
  if (MPIWRAPPER_BITS(h) <= MPIWRAPPER_TOOL_PREDEF_LAST) {
    mpiwrapper_set_handle_error();
    return MPIABI_T_PVAR_HANDLE_NULL;
  }
  return MPIWRAPPER_HANDLE(MPIABI_T_pvar_handle, MPIWRAPPER_BITS(h));
}
#endif

#ifdef MPIWRAPPER_HAVE_MPI_T_pvar_session
MPI_T_pvar_session mpiwrapper_t_pvar_session_fromabi(MPIABI_T_pvar_session abi)
{
  if (abi == MPIABI_T_PVAR_SESSION_NULL) return MPI_T_PVAR_SESSION_NULL;
  return MPIWRAPPER_HANDLE(MPI_T_pvar_session, MPIWRAPPER_BITS(abi));
}

MPIABI_T_pvar_session mpiwrapper_t_pvar_session_toabi(MPI_T_pvar_session h)
{
  if (h == MPI_T_PVAR_SESSION_NULL) return MPIABI_T_PVAR_SESSION_NULL;
  if (MPIWRAPPER_BITS(h) <= MPIWRAPPER_TOOL_PREDEF_LAST) {
    mpiwrapper_set_handle_error();
    return MPIABI_T_PVAR_SESSION_NULL;
  }
  return MPIWRAPPER_HANDLE(MPIABI_T_pvar_session, MPIWRAPPER_BITS(h));
}
#endif

#ifdef MPIWRAPPER_HAVE_MPI_T_event_registration
MPI_T_event_registration mpiwrapper_t_event_registration_fromabi(MPIABI_T_event_registration abi)
{
  return MPIWRAPPER_HANDLE(MPI_T_event_registration, MPIWRAPPER_BITS(abi));
}

MPIABI_T_event_registration mpiwrapper_t_event_registration_toabi(MPI_T_event_registration h)
{
  return MPIWRAPPER_HANDLE(MPIABI_T_event_registration, MPIWRAPPER_BITS(h));
}
#endif

#ifdef MPIWRAPPER_HAVE_MPI_T_event_instance
MPI_T_event_instance mpiwrapper_t_event_instance_fromabi(MPIABI_T_event_instance abi)
{
  return MPIWRAPPER_HANDLE(MPI_T_event_instance, MPIWRAPPER_BITS(abi));
}

MPIABI_T_event_instance mpiwrapper_t_event_instance_toabi(MPI_T_event_instance h)
{
  return MPIWRAPPER_HANDLE(MPIABI_T_event_instance, MPIWRAPPER_BITS(h));
}
#endif


/* MPI_T's obj_handle: the one parameter whose *class* is not in the argument
 * list. MPI-5.0 15.3.6 makes it "an address to a local variable that stores the
 * object's handle", and which kind of handle that is comes from the `bind` a
 * prior get_info reported -- so src/mpiwrapper/toolobj.c asks the
 * implementation for `bind` first and this switch converts accordingly.
 *
 * `bind` here is the *implementation's* own value, straight from its own
 * get_info, so the labels are its names and no conversion intervenes. A null
 * obj_handle and MPI_T_BIND_NO_OBJECT both mean the same thing -- the argument
 * is ignored -- and both produce a null pointer, which is what the
 * implementation is then given.
 */

void *mpiwrapper_tool_obj_fromabi(int bind, void *abi_obj,
                                  union mpiwrapper_tool_obj *out)
{
  if (!abi_obj) return NULL;
  switch (bind) {
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_COMM)
  case MPI_T_BIND_MPI_COMM:
    out->comm = mpiwrapper_comm_fromabi(*(MPIABI_Comm *)abi_obj);
    return &out->comm;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_DATATYPE)
  case MPI_T_BIND_MPI_DATATYPE:
    out->datatype = mpiwrapper_datatype_fromabi(*(MPIABI_Datatype *)abi_obj);
    return &out->datatype;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_ERRHANDLER)
  case MPI_T_BIND_MPI_ERRHANDLER:
    out->errhandler = mpiwrapper_errhandler_fromabi(*(MPIABI_Errhandler *)abi_obj);
    return &out->errhandler;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_FILE)
  case MPI_T_BIND_MPI_FILE:
    out->file = mpiwrapper_file_fromabi(*(MPIABI_File *)abi_obj);
    return &out->file;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_GROUP)
  case MPI_T_BIND_MPI_GROUP:
    out->group = mpiwrapper_group_fromabi(*(MPIABI_Group *)abi_obj);
    return &out->group;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_INFO)
  case MPI_T_BIND_MPI_INFO:
    out->info = mpiwrapper_info_fromabi(*(MPIABI_Info *)abi_obj);
    return &out->info;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_MESSAGE)
  case MPI_T_BIND_MPI_MESSAGE:
    out->message = mpiwrapper_message_fromabi(*(MPIABI_Message *)abi_obj);
    return &out->message;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_OP)
  case MPI_T_BIND_MPI_OP:
    out->op = mpiwrapper_op_fromabi(*(MPIABI_Op *)abi_obj);
    return &out->op;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_REQUEST)
  case MPI_T_BIND_MPI_REQUEST:
    out->request = mpiwrapper_request_fromabi(*(MPIABI_Request *)abi_obj);
    return &out->request;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_SESSION) && defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
  case MPI_T_BIND_MPI_SESSION:
    out->session = mpiwrapper_session_fromabi(*(MPIABI_Session *)abi_obj);
    return &out->session;
#endif
#if defined(MPIWRAPPER_HAVE_MPI_T_BIND_MPI_WIN)
  case MPI_T_BIND_MPI_WIN:
    out->win = mpiwrapper_win_fromabi(*(MPIABI_Win *)abi_obj);
    return &out->win;
#endif
  default: break;
  }
  return NULL;
}
