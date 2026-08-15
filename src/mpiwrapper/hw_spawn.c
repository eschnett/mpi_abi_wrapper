/* libmpiwrapper -- the two spawn entry points (NOTES.md #8, S4b).
 *
 * #8 names these for their three array arguments, and each is a different
 * question:
 *
 *   argv, array_of_argv    NULL-terminated arrays of *strings*. Character data
 *                          crosses unconverted, so only the sentinel needs
 *                          translating -- MPI_ARGV_NULL and MPI_ARGVS_NULL are
 *                          null pointers in the ABI and in both
 *                          implementations, but that is a fact about today's
 *                          headers and not a promise, so it is one test per
 *                          site like every other sentinel (#5.3).
 *   array_of_info          an array of handles, so it is staged and converted
 *                          element by element (#5.7): never rewritten in
 *                          place, since it is the caller's `const` array.
 *   array_of_errcodes      an OUT array of *error codes*, which is the whole
 *                          reason these two are not generated. Every element
 *                          needs the toabi mapping, and MPI_ERRCODES_IGNORE
 *                          says there is no array at all.
 *
 * The length of the errcodes array is what makes the second one interesting:
 * it is the sum of array_of_maxprocs rather than any single argument, so the
 * body adds them up and refuses an overflow before allocating anything --
 * mpiwrapper_sum_degrees, which S3a wrote for the graph topologies and which
 * answers exactly this question.
 *
 * **Testing.** Spawn needs a launcher, so test/abi_state_test.c exercises it
 * only where one works: MPICH here, with Open MPI's row a documented gap
 * rather than a pass (STAGES.md S4b, test/README.md).
 */

#include "internal.h"

/* ------------------------------------------------------------ MPI_Comm_spawn */

#ifdef MPIWRAPPER_HAVE_MPI_Comm_spawn
#  define BODY_MPI_Comm_spawn(TARGET)                                          \
    {                                                                          \
      const char *const command = abi_command;                                 \
      char **const      argv =                                                 \
          abi_argv == MPIABI_ARGV_NULL ? MPI_ARGV_NULL : abi_argv;             \
      const int      maxprocs = abi_maxprocs;                                  \
      const MPI_Info info     = mpiwrapper_info_fromabi(abi_info);             \
      const int      root     = mpiwrapper_rank_fromabi(abi_root);             \
      const MPI_Comm comm     = mpiwrapper_comm_fromabi(abi_comm);             \
      const int      ignore = abi_array_of_errcodes == MPIABI_ERRCODES_IGNORE; \
                                                                               \
      if (maxprocs < 0) {                                                      \
        *abi_intercomm = MPIABI_COMM_NULL;                                     \
        return MPIABI_ERR_ARG;                                                 \
      }                                                                        \
                                                                               \
      int  errcodes_stack[MPIWRAPPER_STAGE_BYTES / sizeof(int)];               \
      int *errcodes   = NULL;                                                  \
      int  abi_ierror = MPIABI_ERR_INTERN;                                     \
                                                                               \
      if (!ignore) {                                                           \
        errcodes = mpiwrapper_stage(errcodes_stack, sizeof errcodes_stack,     \
                                    (size_t)maxprocs, sizeof *errcodes);       \
        if (!errcodes) goto done;                                              \
      }                                                                        \
                                                                               \
      {                                                                        \
        MPI_Comm  intercomm;                                                   \
        const int ierror =                                                     \
            TARGET(command, argv, maxprocs, info, root, comm, &intercomm,      \
                   ignore ? MPI_ERRCODES_IGNORE : errcodes);                   \
                                                                               \
        if (!ignore)                                                           \
          for (int i = 0; i < maxprocs; ++i)                                   \
            abi_array_of_errcodes[i] =                                         \
                mpiwrapper_errorcode_toabi(errcodes[i]);                       \
                                                                               \
        *abi_intercomm = (ierror == MPI_SUCCESS)                               \
                             ? mpiwrapper_comm_toabi(intercomm)                \
                             : MPIABI_COMM_NULL;                               \
        abi_ierror     = mpiwrapper_errorcode_toabi(ierror);                   \
        if (mpiwrapper_take_handle_error()) abi_ierror = MPIABI_ERR_INTERN;    \
      }                                                                        \
                                                                               \
    done:                                                                      \
      mpiwrapper_unstage(errcodes, errcodes_stack);                            \
      return abi_ierror;                                                       \
    }
#else
#  define BODY_MPI_Comm_spawn(TARGET)                                          \
    {                                                                          \
      (void)abi_command;                                                       \
      (void)abi_argv;                                                          \
      (void)abi_maxprocs;                                                      \
      (void)abi_info;                                                          \
      (void)abi_root;                                                          \
      (void)abi_comm;                                                          \
      (void)abi_array_of_errcodes;                                             \
      *abi_intercomm = MPIABI_COMM_NULL;                                       \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Comm_spawn(const char *abi_command, char *abi_argv[],
                                int abi_maxprocs, MPIABI_Info abi_info,
                                int abi_root, MPIABI_Comm abi_comm,
                                MPIABI_Comm *abi_intercomm,
                                int abi_array_of_errcodes[])
    BODY_MPI_Comm_spawn(MPI_Comm_spawn)
int mpiwrapper_w_PMPI_Comm_spawn(const char *abi_command, char *abi_argv[],
                                 int abi_maxprocs, MPIABI_Info abi_info,
                                 int abi_root, MPIABI_Comm abi_comm,
                                 MPIABI_Comm *abi_intercomm,
                                 int abi_array_of_errcodes[])
    BODY_MPI_Comm_spawn(PMPI_Comm_spawn)

/* ------------------------------------------------- MPI_Comm_spawn_multiple */

#ifdef MPIWRAPPER_HAVE_MPI_Comm_spawn_multiple
#  define BODY_MPI_Comm_spawn_multiple(TARGET)                                 \
    {                                                                          \
      const int          count             = abi_count;                        \
      char **const       array_of_commands = abi_array_of_commands;            \
      char ***const      array_of_argv     = abi_array_of_argv                 \
                                                 == MPIABI_ARGVS_NULL          \
                                                 ? MPI_ARGVS_NULL              \
                                                 : abi_array_of_argv;          \
      const int *const array_of_maxprocs   = abi_array_of_maxprocs;            \
      const int        root                = mpiwrapper_rank_fromabi(abi_root);\
      const MPI_Comm   comm  = mpiwrapper_comm_fromabi(abi_comm);              \
      const int ignore = abi_array_of_errcodes == MPIABI_ERRCODES_IGNORE;      \
                                                                               \
      if (count < 0) {                                                         \
        *abi_intercomm = MPIABI_COMM_NULL;                                     \
        return MPIABI_ERR_ARG;                                                 \
      }                                                                        \
                                                                               \
      /* The errcodes array is as long as the *sum* of the maxprocs, which is  \
       * the one length in this file that no single argument gives.            \
       */                                                                      \
      int nerrcodes = 0;                                                       \
      if (!ignore                                                              \
          && !mpiwrapper_sum_degrees(array_of_maxprocs, count, &nerrcodes)) {  \
        *abi_intercomm = MPIABI_COMM_NULL;                                     \
        return MPIABI_ERR_ARG;                                                 \
      }                                                                        \
                                                                               \
      MPI_Info  infos_stack[MPIWRAPPER_STAGE_BYTES / sizeof(MPI_Info)];        \
      MPI_Info *infos = NULL;                                                  \
      int       errcodes_stack[MPIWRAPPER_STAGE_BYTES / sizeof(int)];          \
      int      *errcodes   = NULL;                                             \
      int       abi_ierror = MPIABI_ERR_INTERN;                                \
                                                                               \
      infos = mpiwrapper_stage(infos_stack, sizeof infos_stack, (size_t)count, \
                               sizeof *infos);                                 \
      if (!infos) goto done;                                                   \
      for (int i = 0; i < count; ++i)                                          \
        infos[i] = mpiwrapper_info_fromabi(abi_array_of_info[i]);              \
                                                                               \
      if (!ignore) {                                                           \
        errcodes = mpiwrapper_stage(errcodes_stack, sizeof errcodes_stack,     \
                                    (size_t)nerrcodes, sizeof *errcodes);      \
        if (!errcodes) goto done;                                              \
      }                                                                        \
                                                                               \
      {                                                                        \
        MPI_Comm  intercomm;                                                   \
        const int ierror =                                                     \
            TARGET(count, array_of_commands, array_of_argv,                    \
                   array_of_maxprocs, infos, root, comm, &intercomm,           \
                   ignore ? MPI_ERRCODES_IGNORE : errcodes);                   \
                                                                               \
        if (!ignore)                                                           \
          for (int i = 0; i < nerrcodes; ++i)                                  \
            abi_array_of_errcodes[i] =                                         \
                mpiwrapper_errorcode_toabi(errcodes[i]);                       \
                                                                               \
        *abi_intercomm = (ierror == MPI_SUCCESS)                               \
                             ? mpiwrapper_comm_toabi(intercomm)                \
                             : MPIABI_COMM_NULL;                               \
        abi_ierror     = mpiwrapper_errorcode_toabi(ierror);                   \
        if (mpiwrapper_take_handle_error()) abi_ierror = MPIABI_ERR_INTERN;    \
      }                                                                        \
                                                                               \
    done:                                                                      \
      mpiwrapper_unstage(errcodes, errcodes_stack);                            \
      mpiwrapper_unstage(infos, infos_stack);                                  \
      return abi_ierror;                                                       \
    }
#else
#  define BODY_MPI_Comm_spawn_multiple(TARGET)                                 \
    {                                                                          \
      (void)abi_count;                                                         \
      (void)abi_array_of_commands;                                             \
      (void)abi_array_of_argv;                                                 \
      (void)abi_array_of_maxprocs;                                             \
      (void)abi_array_of_info;                                                 \
      (void)abi_root;                                                          \
      (void)abi_comm;                                                          \
      (void)abi_array_of_errcodes;                                             \
      *abi_intercomm = MPIABI_COMM_NULL;                                       \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Comm_spawn_multiple(
    int abi_count, char *abi_array_of_commands[], char **abi_array_of_argv[],
    const int abi_array_of_maxprocs[], const MPIABI_Info abi_array_of_info[],
    int abi_root, MPIABI_Comm abi_comm, MPIABI_Comm *abi_intercomm,
    int abi_array_of_errcodes[])
    BODY_MPI_Comm_spawn_multiple(MPI_Comm_spawn_multiple)
int mpiwrapper_w_PMPI_Comm_spawn_multiple(
    int abi_count, char *abi_array_of_commands[], char **abi_array_of_argv[],
    const int abi_array_of_maxprocs[], const MPIABI_Info abi_array_of_info[],
    int abi_root, MPIABI_Comm abi_comm, MPIABI_Comm *abi_intercomm,
    int abi_array_of_errcodes[])
    BODY_MPI_Comm_spawn_multiple(PMPI_Comm_spawn_multiple)
