/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate_headers.py from gen/include/mpi.h by renaming
 * typedef names, macro names and enumerator names from MPI_ to MPIABI_.
 * Struct/enum *tags* and struct *members* are left untouched, UNLESS a tag's
 * spelling is identical to its own typedef name (MPI_T_cb_safety,
 * MPI_T_source_order): those two are real MPI-standard tag names that a
 * conforming implementation's own <mpi.h> also declares, so leaving them
 * unrenamed would redeclare the same tag with different enumerators the
 * moment this header and an implementation's mpi.h are included in the same
 * translation unit (which is exactly what libmpiwrapper does). Renaming both
 * occurrences avoids that collision; see NOTES.md #2.
 *
 * Function prototypes are dropped entirely: libmpiwrapper calls the
 * implementation, never the ABI. See NOTES.md #2 and #3.
 */

#ifndef MPIABI_H
#define MPIABI_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define MPIABI_VERSION    5
#define MPIABI_SUBVERSION 0

#define MPIABI_ABI_VERSION    1
#define MPIABI_ABI_SUBVERSION 0

/* MPIABI_Aint is defined to be intptr_t (or equivalent to it, if compiler support is absent).
 * The only acceptable alternative to intptr_t is the C89 type equivalent to it. */
typedef intptr_t MPIABI_Aint;

/* MPIABI_Offset will be 64b on all relevant systems.
 * We allow for MPI implementations supporting for 128b filesystems. */
typedef int64_t MPIABI_Offset;

/* MPIABI_Count must be large enough to hold the larger of MPIABI_Aint and MPIABI_Offset.
 * Platforms where MPIABI_Aint is larger than MPIABI_Offset are extremely rare. */
typedef MPIABI_Offset MPIABI_Count;

/* The struct definition is guarded and the typedef is not, so that a second
 * header may introduce another typedef name for this same struct without
 * redefining it. The tag matters as much as the guard: without one, two such
 * headers describe two incompatible types with identical layout, and every
 * function that passes a status between them needs a cast. */
#if !defined(MPI_ABI_STATUS_DEFINED)
#define MPI_ABI_STATUS_DEFINED
struct MPI_ABI_Status {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int MPI_internal[5];
};
#endif
typedef struct MPI_ABI_Status MPIABI_Status;

typedef struct MPI_ABI_Op* MPIABI_Op;
#define MPIABI_OP_NULL                    ((MPIABI_Op)0x00000020)
#define MPIABI_SUM                        ((MPIABI_Op)0x00000021)
#define MPIABI_MIN                        ((MPIABI_Op)0x00000022)
#define MPIABI_MAX                        ((MPIABI_Op)0x00000023)
#define MPIABI_PROD                       ((MPIABI_Op)0x00000024)
#define MPIABI_BAND                       ((MPIABI_Op)0x00000028)
#define MPIABI_BOR                        ((MPIABI_Op)0x00000029)
#define MPIABI_BXOR                       ((MPIABI_Op)0x0000002a)
#define MPIABI_LAND                       ((MPIABI_Op)0x00000030)
#define MPIABI_LOR                        ((MPIABI_Op)0x00000031)
#define MPIABI_LXOR                       ((MPIABI_Op)0x00000032)
#define MPIABI_MINLOC                     ((MPIABI_Op)0x00000038)
#define MPIABI_MAXLOC                     ((MPIABI_Op)0x00000039)
#define MPIABI_REPLACE                    ((MPIABI_Op)0x0000003c)
#define MPIABI_NO_OP                      ((MPIABI_Op)0x0000003d)

typedef struct MPI_ABI_Comm* MPIABI_Comm;
#define MPIABI_COMM_NULL                  ((MPIABI_Comm)0x00000100)
#define MPIABI_COMM_WORLD                 ((MPIABI_Comm)0x00000101)
#define MPIABI_COMM_SELF                  ((MPIABI_Comm)0x00000102)

typedef struct MPI_ABI_Group* MPIABI_Group;
#define MPIABI_GROUP_NULL                 ((MPIABI_Group)0x00000108)
#define MPIABI_GROUP_EMPTY                ((MPIABI_Group)0x00000109)

typedef struct MPI_ABI_Win* MPIABI_Win;
#define MPIABI_WIN_NULL                   ((MPIABI_Win)0x00000110)

typedef struct MPI_ABI_File* MPIABI_File;
#define MPIABI_FILE_NULL                  ((MPIABI_File)0x00000118)

typedef struct MPI_ABI_Session* MPIABI_Session;
#define MPIABI_SESSION_NULL               ((MPIABI_Session)0x00000120)

typedef struct MPI_ABI_Message* MPIABI_Message;
#define MPIABI_MESSAGE_NULL               ((MPIABI_Message)0x00000128)
#define MPIABI_MESSAGE_NO_PROC            ((MPIABI_Message)0x00000129)

typedef struct MPI_ABI_Info* MPIABI_Info;
#define MPIABI_INFO_NULL                  ((MPIABI_Info)0x00000130)
#define MPIABI_INFO_ENV                   ((MPIABI_Info)0x00000131)

typedef struct MPI_ABI_Errhandler* MPIABI_Errhandler;
#define MPIABI_ERRHANDLER_NULL            ((MPIABI_Errhandler)0x00000140)
#define MPIABI_ERRORS_ARE_FATAL           ((MPIABI_Errhandler)0x00000141)
#define MPIABI_ERRORS_ABORT               ((MPIABI_Errhandler)0x00000142)
#define MPIABI_ERRORS_RETURN              ((MPIABI_Errhandler)0x00000143)

typedef struct MPI_ABI_Request* MPIABI_Request;
#define MPIABI_REQUEST_NULL               ((MPIABI_Request)0x00000180)

typedef struct MPI_ABI_Datatype* MPIABI_Datatype;
#define MPIABI_DATATYPE_NULL              ((MPIABI_Datatype)0x00000200)
#define MPIABI_AINT                       ((MPIABI_Datatype)0x00000201)
#define MPIABI_COUNT                      ((MPIABI_Datatype)0x00000202)
#define MPIABI_OFFSET                     ((MPIABI_Datatype)0x00000203)
#define MPIABI_PACKED                     ((MPIABI_Datatype)0x00000207)
#define MPIABI_SHORT                      ((MPIABI_Datatype)0x00000208)
#define MPIABI_INT                        ((MPIABI_Datatype)0x00000209)
#define MPIABI_LONG                       ((MPIABI_Datatype)0x0000020a)
#define MPIABI_LONG_LONG                  ((MPIABI_Datatype)0x0000020b)
#define MPIABI_LONG_LONG_INT              MPIABI_LONG_LONG
#define MPIABI_UNSIGNED_SHORT             ((MPIABI_Datatype)0x0000020c)
#define MPIABI_UNSIGNED                   ((MPIABI_Datatype)0x0000020d)
#define MPIABI_UNSIGNED_LONG              ((MPIABI_Datatype)0x0000020e)
#define MPIABI_UNSIGNED_LONG_LONG         ((MPIABI_Datatype)0x0000020f)
#define MPIABI_FLOAT                      ((MPIABI_Datatype)0x00000210)
#define MPIABI_C_FLOAT_COMPLEX            ((MPIABI_Datatype)0x00000212)
#define MPIABI_C_COMPLEX                  MPIABI_C_FLOAT_COMPLEX
#define MPIABI_CXX_FLOAT_COMPLEX          ((MPIABI_Datatype)0x00000213)
#define MPIABI_DOUBLE                     ((MPIABI_Datatype)0x00000214)
#define MPIABI_C_DOUBLE_COMPLEX           ((MPIABI_Datatype)0x00000216)
#define MPIABI_CXX_DOUBLE_COMPLEX         ((MPIABI_Datatype)0x00000217)
#define MPIABI_LOGICAL                    ((MPIABI_Datatype)0x00000218)
#define MPIABI_INTEGER                    ((MPIABI_Datatype)0x00000219)
#define MPIABI_REAL                       ((MPIABI_Datatype)0x0000021a)
#define MPIABI_COMPLEX                    ((MPIABI_Datatype)0x0000021b)
#define MPIABI_DOUBLE_PRECISION           ((MPIABI_Datatype)0x0000021c)
#define MPIABI_DOUBLE_COMPLEX             ((MPIABI_Datatype)0x0000021d)
#define MPIABI_CHARACTER                  ((MPIABI_Datatype)0x0000021e)
#define MPIABI_LONG_DOUBLE                ((MPIABI_Datatype)0x00000220)
#define MPIABI_C_LONG_DOUBLE_COMPLEX      ((MPIABI_Datatype)0x00000224)
#define MPIABI_CXX_LONG_DOUBLE_COMPLEX    ((MPIABI_Datatype)0x00000225)
#define MPIABI_FLOAT_INT                  ((MPIABI_Datatype)0x00000228)
#define MPIABI_DOUBLE_INT                 ((MPIABI_Datatype)0x00000229)
#define MPIABI_LONG_INT                   ((MPIABI_Datatype)0x0000022a)
#define MPIABI_2INT                       ((MPIABI_Datatype)0x0000022b)
#define MPIABI_SHORT_INT                  ((MPIABI_Datatype)0x0000022c)
#define MPIABI_LONG_DOUBLE_INT            ((MPIABI_Datatype)0x0000022d)
#define MPIABI_2REAL                      ((MPIABI_Datatype)0x00000230)
#define MPIABI_2DOUBLE_PRECISION          ((MPIABI_Datatype)0x00000231)
#define MPIABI_2INTEGER                   ((MPIABI_Datatype)0x00000232)
#define MPIABI_C_BOOL                     ((MPIABI_Datatype)0x00000238)
#define MPIABI_CXX_BOOL                   ((MPIABI_Datatype)0x00000239)
#define MPIABI_WCHAR                      ((MPIABI_Datatype)0x0000023c)
#define MPIABI_INT8_T                     ((MPIABI_Datatype)0x00000240)
#define MPIABI_UINT8_T                    ((MPIABI_Datatype)0x00000241)
#define MPIABI_CHAR                       ((MPIABI_Datatype)0x00000243)
#define MPIABI_SIGNED_CHAR                ((MPIABI_Datatype)0x00000244)
#define MPIABI_UNSIGNED_CHAR              ((MPIABI_Datatype)0x00000245)
#define MPIABI_BYTE                       ((MPIABI_Datatype)0x00000247)
#define MPIABI_INT16_T                    ((MPIABI_Datatype)0x00000248)
#define MPIABI_UINT16_T                   ((MPIABI_Datatype)0x00000249)
#define MPIABI_INT32_T                    ((MPIABI_Datatype)0x00000250)
#define MPIABI_UINT32_T                   ((MPIABI_Datatype)0x00000251)
#define MPIABI_INT64_T                    ((MPIABI_Datatype)0x00000258)
#define MPIABI_UINT64_T                   ((MPIABI_Datatype)0x00000259)
#define MPIABI_LOGICAL1                   ((MPIABI_Datatype)0x000002c0)
#define MPIABI_INTEGER1                   ((MPIABI_Datatype)0x000002c1)
#define MPIABI_LOGICAL2                   ((MPIABI_Datatype)0x000002c8)
#define MPIABI_INTEGER2                   ((MPIABI_Datatype)0x000002c9)
#define MPIABI_REAL2                      ((MPIABI_Datatype)0x000002ca)
#define MPIABI_LOGICAL4                   ((MPIABI_Datatype)0x000002d0)
#define MPIABI_INTEGER4                   ((MPIABI_Datatype)0x000002d1)
#define MPIABI_REAL4                      ((MPIABI_Datatype)0x000002d2)
#define MPIABI_COMPLEX4                   ((MPIABI_Datatype)0x000002d3)
#define MPIABI_LOGICAL8                   ((MPIABI_Datatype)0x000002d8)
#define MPIABI_INTEGER8                   ((MPIABI_Datatype)0x000002d9)
#define MPIABI_REAL8                      ((MPIABI_Datatype)0x000002da)
#define MPIABI_COMPLEX8                   ((MPIABI_Datatype)0x000002db)
#define MPIABI_LOGICAL16                  ((MPIABI_Datatype)0x000002e0)
#define MPIABI_INTEGER16                  ((MPIABI_Datatype)0x000002e1)
#define MPIABI_REAL16                     ((MPIABI_Datatype)0x000002e2)
#define MPIABI_COMPLEX16                  ((MPIABI_Datatype)0x000002e3)
#define MPIABI_COMPLEX32                  ((MPIABI_Datatype)0x000002eb)

/* Fortran 1977 Status Size and Indices */
enum {
    MPIABI_F_STATUS_SIZE                  = 8,
    MPIABI_F_SOURCE                       = 0,
    MPIABI_F_TAG                          = 1,
    MPIABI_F_ERROR                        = 2
};

/* Error Classes */
enum {
    MPIABI_SUCCESS                        =  0,

    MPIABI_ERR_BUFFER                     =  1, /* added: MPI-1.0 */
    MPIABI_ERR_COUNT                      =  2, /* added: MPI-1.0 */
    MPIABI_ERR_TYPE                       =  3, /* added: MPI-1.0 */
    MPIABI_ERR_TAG                        =  4, /* added: MPI-1.0 */
    MPIABI_ERR_COMM                       =  5, /* added: MPI-1.0 */
    MPIABI_ERR_RANK                       =  6, /* added: MPI-1.0 */
    MPIABI_ERR_REQUEST                    =  7, /* added: MPI-1.0 */
    MPIABI_ERR_ROOT                       =  8, /* added: MPI-1.0 */
    MPIABI_ERR_GROUP                      =  9, /* added: MPI-1.0 */
    MPIABI_ERR_OP                         = 10, /* added: MPI-1.0 */
    MPIABI_ERR_TOPOLOGY                   = 11, /* added: MPI-1.0 */
    MPIABI_ERR_DIMS                       = 12, /* added: MPI-1.0 */
    MPIABI_ERR_ARG                        = 13, /* added: MPI-1.0 */
    MPIABI_ERR_UNKNOWN                    = 14, /* added: MPI-1.0 */
    MPIABI_ERR_TRUNCATE                   = 15, /* added: MPI-1.0 */
    MPIABI_ERR_OTHER                      = 16, /* added: MPI-1.0 */
    MPIABI_ERR_INTERN                     = 17, /* added: MPI-1.0 */
    MPIABI_ERR_PENDING                    = 18, /* added: MPI-1.1 */
    MPIABI_ERR_IN_STATUS                  = 19, /* added: MPI-1.1 */
    MPIABI_ERR_ACCESS                     = 20, /* added: MPI-2.0 */
    MPIABI_ERR_AMODE                      = 21, /* added: MPI-2.0 */
    MPIABI_ERR_ASSERT                     = 22, /* added: MPI-2.0 */
    MPIABI_ERR_BAD_FILE                   = 23, /* added: MPI-2.0 */
    MPIABI_ERR_BASE                       = 24, /* added: MPI-2.0 */
    MPIABI_ERR_CONVERSION                 = 25, /* added: MPI-2.0 */
    MPIABI_ERR_DISP                       = 26, /* added: MPI-2.0 */
    MPIABI_ERR_DUP_DATAREP                = 27, /* added: MPI-2.0 */
    MPIABI_ERR_FILE_EXISTS                = 28, /* added: MPI-2.0 */
    MPIABI_ERR_FILE_IN_USE                = 29, /* added: MPI-2.0 */
    MPIABI_ERR_FILE                       = 30, /* added: MPI-2.0 */
    MPIABI_ERR_INFO_KEY                   = 31, /* added: MPI-2.0 */
    MPIABI_ERR_INFO_NOKEY                 = 32, /* added: MPI-2.0 */
    MPIABI_ERR_INFO_VALUE                 = 33, /* added: MPI-2.0 */
    MPIABI_ERR_INFO                       = 34, /* added: MPI-2.0 */
    MPIABI_ERR_IO                         = 35, /* added: MPI-2.0 */
    MPIABI_ERR_KEYVAL                     = 36, /* added: MPI-2.0 */
    MPIABI_ERR_LOCKTYPE                   = 37, /* added: MPI-2.0 */
    MPIABI_ERR_NAME                       = 38, /* added: MPI-2.0 */
    MPIABI_ERR_NO_MEM                     = 39, /* added: MPI-2.0 */
    MPIABI_ERR_NOT_SAME                   = 40, /* added: MPI-2.0 */
    MPIABI_ERR_NO_SPACE                   = 41, /* added: MPI-2.0 */
    MPIABI_ERR_NO_SUCH_FILE               = 42, /* added: MPI-2.0 */
    MPIABI_ERR_PORT                       = 43, /* added: MPI-2.0 */
    MPIABI_ERR_QUOTA                      = 44, /* added: MPI-2.0 */
    MPIABI_ERR_READ_ONLY                  = 45, /* added: MPI-2.0 */
    MPIABI_ERR_RMA_ATTACH                 = 46, /* added: MPI-2.0 */
    MPIABI_ERR_RMA_CONFLICT               = 47, /* added: MPI-2.0 */
    MPIABI_ERR_RMA_RANGE                  = 48, /* added: MPI-2.0 */
    MPIABI_ERR_RMA_SHARED                 = 49, /* added: MPI-2.0 */
    MPIABI_ERR_RMA_SYNC                   = 50, /* added: MPI-2.0 */
    MPIABI_ERR_SERVICE                    = 51, /* added: MPI-2.0 */
    MPIABI_ERR_SIZE                       = 52, /* added: MPI-2.0 */
    MPIABI_ERR_SPAWN                      = 53, /* added: MPI-2.0 */
    MPIABI_ERR_UNSUPPORTED_DATAREP        = 54, /* added: MPI-2.0 */
    MPIABI_ERR_UNSUPPORTED_OPERATION      = 55, /* added: MPI-2.0 */
    MPIABI_ERR_WIN                        = 56, /* added: MPI-2.0 */
    MPIABI_ERR_RMA_FLAVOR                 = 57, /* added: MPI-3.0 */
    MPIABI_ERR_PROC_ABORTED               = 58, /* added: MPI-4.0 */
    MPIABI_ERR_VALUE_TOO_LARGE            = 59, /* added: MPI-4.0 */
    MPIABI_ERR_SESSION                    = 60, /* added: MPI-4.0 */
    MPIABI_ERR_ERRHANDLER                 = 61, /* added: MPI-4.1 */
    MPIABI_ERR_ABI                        = 62, /* added: MPI-5.0 */

    MPIABI_T_ERR_CANNOT_INIT              = 1001,
    MPIABI_T_ERR_NOT_ACCESSIBLE           = 1002,
    MPIABI_T_ERR_NOT_INITIALIZED          = 1003,
    MPIABI_T_ERR_NOT_SUPPORTED            = 1004,
    MPIABI_T_ERR_MEMORY                   = 1005,
    MPIABI_T_ERR_INVALID                  = 1006,
    MPIABI_T_ERR_INVALID_INDEX            = 1007,
    MPIABI_T_ERR_INVALID_ITEM             = 1008, /* deprecated: MPI-4.0 */
    MPIABI_T_ERR_INVALID_SESSION          = 1009,
    MPIABI_T_ERR_INVALID_HANDLE           = 1010,
    MPIABI_T_ERR_INVALID_NAME             = 1011,
    MPIABI_T_ERR_OUT_OF_HANDLES           = 1012,
    MPIABI_T_ERR_OUT_OF_SESSIONS          = 1013,
    MPIABI_T_ERR_CVAR_SET_NOT_NOW         = 1014,
    MPIABI_T_ERR_CVAR_SET_NEVER           = 1015,
    MPIABI_T_ERR_PVAR_NO_WRITE            = 1016,
    MPIABI_T_ERR_PVAR_NO_STARTSTOP        = 1017,
    MPIABI_T_ERR_PVAR_NO_ATOMIC           = 1018,

    MPIABI_ERR_LASTCODE                   = 0x3fff /* half of the minimum required value of INT_MAX */
};

/* Buffer Address Constants */
#define MPIABI_BOTTOM                     ((void*)0)
#define MPIABI_IN_PLACE                   ((void*)1)
#define MPIABI_BUFFER_AUTOMATIC           ((void*)2)

/* Empty/Ignored Constants */
#define MPIABI_ARGV_NULL                  ((char**)0)
#define MPIABI_ARGVS_NULL                 ((char***)0)
#define MPIABI_ERRCODES_IGNORE            ((int*)0)
#define MPIABI_STATUS_IGNORE              ((MPIABI_Status*)0)
#define MPIABI_STATUSES_IGNORE            ((MPIABI_Status*)0)
#define MPIABI_UNWEIGHTED                 ((int*)10)
#define MPIABI_WEIGHTS_EMPTY              ((int*)11)

/* Maximum Sizes for Strings */
#define MPIABI_MAX_DATAREP_STRING          128 /* MPICH:  128 - OMPI:  128 */
#define MPIABI_MAX_ERROR_STRING            512 /* MPICH:  512 - OMPI:  256 */
#define MPIABI_MAX_INFO_KEY                256 /* MPICH:  255 - OMPI:   36 */
#define MPIABI_MAX_INFO_VAL               1024 /* MPICH: 1024 - OMPI:  256 */
#define MPIABI_MAX_LIBRARY_VERSION_STRING 8192 /* MPICH: 8192 - OMPI:  256 */
#define MPIABI_MAX_OBJECT_NAME             128 /* MPICH:  128 - OMPI:   64 */
#define MPIABI_MAX_PORT_NAME              1024 /* MPICH:  256 - OMPI: 1024 */
#define MPIABI_MAX_PROCESSOR_NAME          256 /* MPICH:  128 - OMPI:  256 */
#define MPIABI_MAX_STRINGTAG_LEN          1024 /* MPICH:  256 - OMPI: 1024 */
#define MPIABI_MAX_PSET_NAME_LEN          1024 /* MPICH:  256 - OMPI:  512 */
/* Assorted Constants */
#define MPIABI_BSEND_OVERHEAD              512 /* MPICH:   96 - OMPI:  128 */

/* Mode Constants - must be powers-of-2 to support OR-ing */
enum {
    /* File Open Modes */
    MPIABI_MODE_APPEND                    = 1,
    MPIABI_MODE_CREATE                    = 2,
    MPIABI_MODE_DELETE_ON_CLOSE           = 4,
    MPIABI_MODE_EXCL                      = 8,
    MPIABI_MODE_RDONLY                    = 16,
    MPIABI_MODE_RDWR                      = 32,
    MPIABI_MODE_SEQUENTIAL                = 64,
    MPIABI_MODE_UNIQUE_OPEN               = 128,
    MPIABI_MODE_WRONLY                    = 256,

    /* Window Assertion Modes */
    MPIABI_MODE_NOCHECK                   = 1024,
    MPIABI_MODE_NOPRECEDE                 = 2048,
    MPIABI_MODE_NOPUT                     = 4096,
    MPIABI_MODE_NOSTORE                   = 8192,
    MPIABI_MODE_NOSUCCEED                 = 16384
};

enum {
    /* Wildcard values - must be negative */
    MPIABI_ANY_SOURCE                     = -1,
    MPIABI_ANY_TAG                        = -2,

    /* Rank sentinels - must be negative */
    MPIABI_PROC_NULL                      = -3,
    MPIABI_ROOT                           = -4,

    /* Multi-purpose sentinel - must be negative */
    MPIABI_UNDEFINED                      = -32766
};

enum {
    /* Thread Support - monotonic values, SINGLE < FUNNELED < SERIALIZED < MULTIPLE. */
    MPIABI_THREAD_SINGLE                  = 0,
    MPIABI_THREAD_FUNNELED                = 1024,
    MPIABI_THREAD_SERIALIZED              = 2048,
    MPIABI_THREAD_MULTIPLE                = 4096,

    /* Array Datatype Order */
    MPIABI_ORDER_C                        = 0xC, /* 12 */
    MPIABI_ORDER_FORTRAN                  = 0xF, /* 15 */

    /* Array Datatype Distribution */
    MPIABI_DISTRIBUTE_NONE                = 16,
    MPIABI_DISTRIBUTE_BLOCK               = 17,
    MPIABI_DISTRIBUTE_CYCLIC              = 18,
    MPIABI_DISTRIBUTE_DFLT_DARG           = 19,

    /* Datatype Decoding Combiners */
    MPIABI_COMBINER_NAMED                 = 101,
    MPIABI_COMBINER_DUP                   = 102,
    MPIABI_COMBINER_CONTIGUOUS            = 103,
    MPIABI_COMBINER_VECTOR                = 104,
    MPIABI_COMBINER_HVECTOR               = 105,
    MPIABI_COMBINER_INDEXED               = 106,
    MPIABI_COMBINER_HINDEXED              = 107,
    MPIABI_COMBINER_INDEXED_BLOCK         = 108,
    MPIABI_COMBINER_HINDEXED_BLOCK        = 109,
    MPIABI_COMBINER_STRUCT                = 110,
    MPIABI_COMBINER_SUBARRAY              = 111,
    MPIABI_COMBINER_DARRAY                = 112,
    MPIABI_COMBINER_F90_REAL              = 113,
    MPIABI_COMBINER_F90_COMPLEX           = 114,
    MPIABI_COMBINER_F90_INTEGER           = 115,
    MPIABI_COMBINER_RESIZED               = 116,
    MPIABI_COMBINER_VALUE_INDEX           = 117,

    /* Fortran Datatype Matching */
    MPIABIX_TYPECLASS_LOGICAL             = 191,
    MPIABI_TYPECLASS_INTEGER              = 192,
    MPIABI_TYPECLASS_REAL                 = 193,
    MPIABI_TYPECLASS_COMPLEX              = 194,

    /* Communicator and Group Comparisons */
    MPIABI_IDENT                          = 201,
    MPIABI_CONGRUENT                      = 202,
    MPIABI_SIMILAR                        = 203,
    MPIABI_UNEQUAL                        = 204,

    /* Communicator Virtual Topology Types */
    MPIABI_CART                           = 211,
    MPIABI_GRAPH                          = 212,
    MPIABI_DIST_GRAPH                     = 213,

    /* Communicator Split Types */
    MPIABI_COMM_TYPE_SHARED               = 221,
    MPIABI_COMM_TYPE_HW_UNGUIDED          = 222,
    MPIABI_COMM_TYPE_HW_GUIDED            = 223,
    MPIABI_COMM_TYPE_RESOURCE_GUIDED      = 224,

    /* Window Lock Types */
    MPIABI_LOCK_EXCLUSIVE                 = 301,
    MPIABI_LOCK_SHARED                    = 302,

    /* Window Create Flavors */
    MPIABI_WIN_FLAVOR_CREATE              = 311,
    MPIABI_WIN_FLAVOR_ALLOCATE            = 312,
    MPIABI_WIN_FLAVOR_DYNAMIC             = 313,
    MPIABI_WIN_FLAVOR_SHARED              = 314,

    /* Window Memory Models */
    MPIABI_WIN_UNIFIED                    = 321,
    MPIABI_WIN_SEPARATE                   = 322,

    /* File Positioning */
    MPIABI_SEEK_CUR                       = 401,
    MPIABI_SEEK_END                       = 402,
    MPIABI_SEEK_SET                       = 403
};

/* File Operation Constants */
#define MPIABI_DISPLACEMENT_CURRENT       ((MPIABI_Offset)-1)

/* Predefined Attribute Keys */
enum {
    /* Invalid Attribute Key */
    MPIABI_KEYVAL_INVALID                 = 0,

    /* Communicator */
    MPIABI_TAG_UB                         = 501,
    MPIABI_IO                             = 502,
    MPIABI_HOST                           = 503, /* deprecated: MPI-4.1 */
    MPIABI_WTIME_IS_GLOBAL                = 504,
    MPIABI_APPNUM                         = 505,
    MPIABI_LASTUSEDCODE                   = 506,
    MPIABI_UNIVERSE_SIZE                  = 507,

    /* Window */
    MPIABI_WIN_BASE                       = 601,
    MPIABI_WIN_DISP_UNIT                  = 602,
    MPIABI_WIN_SIZE                       = 603,
    MPIABI_WIN_CREATE_FLAVOR              = 604,
    MPIABI_WIN_MODEL                      = 605
};

typedef void (MPIABI_User_function)(void *invec, void *inoutvec, int *len, MPIABI_Datatype *datatype);
typedef void (MPIABI_User_function_c)(void *invec, void *inoutvec, MPIABI_Count *len, MPIABI_Datatype *datatype);

typedef int (MPIABI_Grequest_query_function)(void *extra_state, MPIABI_Status *status);
typedef int (MPIABI_Grequest_free_function)(void *extra_state);
typedef int (MPIABI_Grequest_cancel_function)(void *extra_state, int complete);

typedef int (MPIABI_Copy_function)(MPIABI_Comm comm, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag); /* deprecated: MPI-2.0 */
typedef int (MPIABI_Delete_function)(MPIABI_Comm omm, int keyval, void *attribute_val, void *extra_state); /* deprecated: MPI-2.0 */
typedef int (MPIABI_Comm_copy_attr_function)(MPIABI_Comm comm, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);
typedef int (MPIABI_Comm_delete_attr_function)(MPIABI_Comm comm, int keyval, void *attribute_val, void *extra_state);
typedef int (MPIABI_Type_copy_attr_function)(MPIABI_Datatype datatype, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);
typedef int (MPIABI_Type_delete_attr_function)(MPIABI_Datatype datatype, int keyval, void *attribute_val, void *extra_state);
typedef int (MPIABI_Win_copy_attr_function)(MPIABI_Win win, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);
typedef int (MPIABI_Win_delete_attr_function)(MPIABI_Win win, int keyval, void *attribute_val, void *extra_state);

typedef int (MPIABI_Datarep_extent_function)(MPIABI_Datatype datatype, MPIABI_Aint *extent, void *extra_state);
typedef int (MPIABI_Datarep_conversion_function)(void *userbuf, MPIABI_Datatype datatype, int count, void *filebuf, MPIABI_Offset position, void *extra_state);
typedef int (MPIABI_Datarep_conversion_function_c)(void *userbuf, MPIABI_Datatype datatype, MPIABI_Count count, void *filebuf, MPIABI_Offset position, void *extra_state);

typedef void (MPIABI_Comm_errhandler_function)(MPIABI_Comm *comm, int *error_code, ...);
typedef void (MPIABI_File_errhandler_function)(MPIABI_File *file, int *error_code, ...);
typedef void (MPIABI_Win_errhandler_function)(MPIABI_Win *win, int *error_code, ...);
typedef void (MPIABI_Session_errhandler_function)(MPIABI_Session *session, int *error_code, ...);

typedef MPIABI_Comm_errhandler_function MPIABI_Comm_errhandler_fn;
typedef MPIABI_File_errhandler_function MPIABI_File_errhandler_fn;
typedef MPIABI_Win_errhandler_function MPIABI_Win_errhandler_fn;
typedef MPIABI_Session_errhandler_function MPIABI_Session_errhandler_fn;

#define MPIABI_NULL_COPY_FN               ((MPIABI_Copy_function*)0x0) /* deprecated: MPI-2.0 */
#define MPIABI_DUP_FN                     ((MPIABI_Copy_function*)0x1) /* deprecated: MPI-2.0 */
#define MPIABI_NULL_DELETE_FN             ((MPIABI_Delete_function*)0x0) /* deprecated: MPI-2.0 */
#define MPIABI_COMM_NULL_COPY_FN          ((MPIABI_Comm_copy_attr_function*)0x0)
#define MPIABI_COMM_DUP_FN                ((MPIABI_Comm_copy_attr_function*)0x1)
#define MPIABI_COMM_NULL_DELETE_FN        ((MPIABI_Comm_delete_attr_function*)0x0)
#define MPIABI_TYPE_NULL_COPY_FN          ((MPIABI_Type_copy_attr_function*)0x0)
#define MPIABI_TYPE_DUP_FN                ((MPIABI_Type_copy_attr_function*)0x1)
#define MPIABI_TYPE_NULL_DELETE_FN        ((MPIABI_Type_delete_attr_function*)0x0)
#define MPIABI_WIN_NULL_COPY_FN           ((MPIABI_Win_copy_attr_function*)0x0)
#define MPIABI_WIN_DUP_FN                 ((MPIABI_Win_copy_attr_function*)0x1)
#define MPIABI_WIN_NULL_DELETE_FN         ((MPIABI_Win_delete_attr_function*)0x0)
#define MPIABI_CONVERSION_FN_NULL         ((MPIABI_Datarep_conversion_function*)0x0)
#define MPIABI_CONVERSION_FN_NULL_C       ((MPIABI_Datarep_conversion_function_c*)0x0)

/* MPI_T types and constants */

typedef struct MPI_T_enum_t* MPIABI_T_enum;
typedef struct MPI_T_cvar_handle_t* MPIABI_T_cvar_handle;
typedef struct MPI_T_pvar_handle_t* MPIABI_T_pvar_handle;
typedef struct MPI_T_pvar_session_t* MPIABI_T_pvar_session;
typedef struct MPI_T_event_registration_t* MPIABI_T_event_registration;
typedef struct MPI_T_event_instance_t* MPIABI_T_event_instance;

#define MPIABI_T_ENUM_NULL                ((MPIABI_T_enum)0)
#define MPIABI_T_CVAR_HANDLE_NULL         ((MPIABI_T_cvar_handle)0)
#define MPIABI_T_PVAR_SESSION_NULL        ((MPIABI_T_pvar_session)0)
#define MPIABI_T_PVAR_HANDLE_NULL         ((MPIABI_T_pvar_handle)0)
#define MPIABI_T_PVAR_ALL_HANDLES         ((MPIABI_T_pvar_handle)1)

typedef enum  MPIABI_T_cb_safety {
    MPIABI_T_CB_REQUIRE_NONE              = 0x00,
    MPIABI_T_CB_REQUIRE_MPI_RESTRICTED    = 0x03,
    MPIABI_T_CB_REQUIRE_THREAD_SAFE       = 0x0F,
    MPIABI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE = 0x3F
} MPIABI_T_cb_safety;

typedef enum MPIABI_T_source_order {
    MPIABI_T_SOURCE_ORDERED               = 1,
    MPIABI_T_SOURCE_UNORDERED             = 2
} MPIABI_T_source_order;

enum {
    MPIABI_T_VERBOSITY_USER_BASIC         = 0x09,
    MPIABI_T_VERBOSITY_USER_DETAIL        = 0x0a,
    MPIABI_T_VERBOSITY_USER_ALL           = 0x0c,
    MPIABI_T_VERBOSITY_TUNER_BASIC        = 0x11,
    MPIABI_T_VERBOSITY_TUNER_DETAIL       = 0x12,
    MPIABI_T_VERBOSITY_TUNER_ALL          = 0x14,
    MPIABI_T_VERBOSITY_MPIDEV_BASIC       = 0x21,
    MPIABI_T_VERBOSITY_MPIDEV_DETAIL      = 0x22,
    MPIABI_T_VERBOSITY_MPIDEV_ALL         = 0x24
};

enum {
    MPIABI_T_BIND_NO_OBJECT               = 1,
    MPIABI_T_BIND_MPI_COMM                = 2,
    MPIABI_T_BIND_MPI_DATATYPE            = 3,
    MPIABI_T_BIND_MPI_ERRHANDLER          = 4,
    MPIABI_T_BIND_MPI_FILE                = 5,
    MPIABI_T_BIND_MPI_GROUP               = 6,
    MPIABI_T_BIND_MPI_OP                  = 7,
    MPIABI_T_BIND_MPI_REQUEST             = 8,
    MPIABI_T_BIND_MPI_WIN                 = 9,
    MPIABI_T_BIND_MPI_MESSAGE             = 10,
    MPIABI_T_BIND_MPI_INFO                = 11,
    MPIABI_T_BIND_MPI_SESSION             = 12
};

enum {
    MPIABI_T_SCOPE_CONSTANT               = 1,
    MPIABI_T_SCOPE_READONLY               = 2,
    MPIABI_T_SCOPE_LOCAL                  = 3,
    MPIABI_T_SCOPE_GROUP                  = 4,
    MPIABI_T_SCOPE_GROUP_EQ               = 5,
    MPIABI_T_SCOPE_ALL                    = 6,
    MPIABI_T_SCOPE_ALL_EQ                 = 7
};

enum {
    MPIABI_T_PVAR_CLASS_STATE             =  1,
    MPIABI_T_PVAR_CLASS_LEVEL             =  2,
    MPIABI_T_PVAR_CLASS_SIZE              =  3,
    MPIABI_T_PVAR_CLASS_PERCENTAGE        =  4,
    MPIABI_T_PVAR_CLASS_HIGHWATERMARK     =  5,
    MPIABI_T_PVAR_CLASS_LOWWATERMARK      =  6,
    MPIABI_T_PVAR_CLASS_COUNTER           =  7,
    MPIABI_T_PVAR_CLASS_AGGREGATE         =  8,
    MPIABI_T_PVAR_CLASS_TIMER             =  9,
    MPIABI_T_PVAR_CLASS_GENERIC           =  10
};

typedef void (MPIABI_T_event_cb_function)(MPIABI_T_event_instance event_instance, MPIABI_T_event_registration event_registration, MPIABI_T_cb_safety cb_safety, void *user_data);
typedef void (MPIABI_T_event_free_cb_function)(MPIABI_T_event_registration event_registration, MPIABI_T_cb_safety cb_safety, void *user_data);
typedef void (MPIABI_T_event_dropped_cb_function)(MPIABI_Count count, MPIABI_T_event_registration event_registration, int source_index, MPIABI_T_cb_safety cb_safety, void *user_data);

/* Fortran declarations */

typedef int MPIABI_Fint;

typedef MPIABI_Status MPIABI_F08_Status;

/* MPI-5.0 19.3.5 asks for "two global variables of type MPIABI_Fint*,
   MPIABI_F_STATUS_IGNORE and MPIABI_F_STATUSES_IGNORE, ... declared in mpi.h. They
   can be used to test, in C, whether f_status is the Fortran value of
   MPIABI_STATUS_IGNORE or MPIABI_STATUSES_IGNORE", and its rationale notes that
   "this constant need not have the same value in Fortran and C".

   mpif's Fortran sentinels are COMMON blocks of its own, whose storage
   src/mpif_constants.c defines, so these name that storage rather than the C
   null pointer. Only code that uses one of the four macros needs the symbol,
   and such code is by definition linking mpif's Fortran bindings.

   All four used to be MPIABI_STATUS_IGNORE, i.e. null, which made them
   indistinguishable -- a C layer could not tell an mpi_f08 status sentinel from
   an mpif.h one. Now each names its own object. */

extern const MPIABI_Fint mpif_status_ignore_[];
extern const MPIABI_Fint mpif_statuses_ignore_[];
extern const MPIABI_Fint mpif_f08_status_ignore_[];
extern const MPIABI_Fint mpif_f08_statuses_ignore_[];

#define MPIABI_F_STATUS_IGNORE     ((MPIABI_Fint*)mpif_status_ignore_)
#define MPIABI_F_STATUSES_IGNORE   ((MPIABI_Fint*)mpif_statuses_ignore_)
#define MPIABI_F08_STATUS_IGNORE   ((MPIABI_F08_Status*)mpif_f08_status_ignore_)
#define MPIABI_F08_STATUSES_IGNORE ((MPIABI_F08_Status*)mpif_f08_statuses_ignore_)

#if defined(__cplusplus)
}
#endif

#endif /* MPIABI_H */
