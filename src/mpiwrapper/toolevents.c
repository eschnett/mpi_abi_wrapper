/* libmpiwrapper -- the MPI_T event callbacks' registration map (NOTES.md
 * #6.1, S4b).
 *
 * The third of #6.1's three mechanisms, and it exists because neither of the
 * other two fits. A pool does not, because these callbacks are not anonymous:
 * they belong to an event-registration handle and there are three kinds of
 * them per handle. The extra-state pair of extrastate.c does not either, and
 * that is the interesting half: MPI_T_EVENT_SET_DROPPED_HANDLER takes *no*
 * user_data argument, while the dropped callback it registers is handed one
 * anyway -- MPI-5.0 15.3.6 says it receives "the pointer to user-allocated
 * memory that was passed to the MPI implementation during callback
 * registration", i.e. whatever some earlier MPI_T_EVENT_REGISTER_CALLBACK
 * happened to pass, for whichever safety level. Nothing of ours can be
 * smuggled through a registration that has no argument to carry it, and
 * borrowing another call's argument would make the dropped handler depend on
 * the order the two were registered in.
 *
 * What all three callbacks *do* receive is the event-registration handle, and
 * we convert that anyway. So one map keyed on the implementation's handle
 * serves the lot.
 *
 * **Reclamation.** #6.2 gives this family a real reclamation point, unlike the
 * ops and the error handlers: MPI-5.0 15.3.6 invokes the free callback "when
 * it is able to guarantee that no further event instances for the
 * corresponding event-registration handle will be raised", which is precisely
 * when the entry is dead. So mpiwrapper_t_event_free_tramp releases it -- and
 * it is installed even when the application passed no free callback of its
 * own, since otherwise a tool that allocates and frees registrations in a loop
 * would exhaust the map.
 *
 * **What is not tested, and it should be said plainly.** Neither
 * implementation available here raises an event: Open MPI 5.0.6 has no event
 * interface at all, and MPICH 4.3.1 declares every entry point and reports
 * MPI_T_event_get_num == 0, so no registration handle can be allocated to key
 * this map with. The bodies and the conversions are compiled and their
 * error paths are exercised by test/abi_state_test.c; the callbacks
 * themselves have no oracle here and are named as a gap in test/README.md.
 */

#include "internal.h"

#ifdef MPIWRAPPER_HAVE_MPI_T_event_registration

#  include <stdatomic.h>

#  ifndef MPIWRAPPER_T_EVENT_SLOTS
#    define MPIWRAPPER_T_EVENT_SLOTS 64
#  endif

/* MPI-5.0 Table 15.5 has four callback safety levels, and the ABI spells them
 * 0x00, 0x03, 0x0f and 0x3f rather than 0..3 -- the values are a bitmask of
 * the guarantees each level makes. An application may register one callback
 * per level for a registration handle, so the entry holds four.
 */
#  define MPIWRAPPER_T_CB_LEVELS 4

static int safety_index(int abi_cb_safety)
{
  switch (abi_cb_safety) {
  case MPIABI_T_CB_REQUIRE_NONE: return 0;
  case MPIABI_T_CB_REQUIRE_MPI_RESTRICTED: return 1;
  case MPIABI_T_CB_REQUIRE_THREAD_SAFE: return 2;
  case MPIABI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE: return 3;
  default: return -1;
  }
}

/* A slot is free when `used` is 0. The key is the implementation handle's
 * bits, formed with the same macro as everywhere else so that a lookup and an
 * insert cannot disagree about sign extension.
 */
struct event_entry {
  atomic_int       used;
  _Atomic uint64_t key;
  MPIABI_T_event_cb_function *_Atomic cb[MPIWRAPPER_T_CB_LEVELS];
  void *_Atomic                       cb_data[MPIWRAPPER_T_CB_LEVELS];
  MPIABI_T_event_dropped_cb_function *_Atomic dropped_fn;
  MPIABI_T_event_free_cb_function *_Atomic    free_fn;
  void *_Atomic                               free_data;
};

static struct event_entry events[MPIWRAPPER_T_EVENT_SLOTS];

static struct event_entry *find(uint64_t key)
{
  for (int i = 0; i < MPIWRAPPER_T_EVENT_SLOTS; ++i)
    if (atomic_load_explicit(&events[i].used, memory_order_acquire)
        && atomic_load_explicit(&events[i].key, memory_order_acquire) == key)
      return &events[i];
  return NULL;
}

/* Find or create. A registration handle is claimed by exactly one entry: the
 * CAS on `used` is what makes that true when two threads register callbacks
 * for the same handle at once, and the loser of the race then finds the
 * winner's entry on its second pass.
 */
static struct event_entry *intern(uint64_t key)
{
  struct event_entry *e = find(key);
  if (e) return e;

  for (int i = 0; i < MPIWRAPPER_T_EVENT_SLOTS; ++i) {
    int expected = 0;
    if (!atomic_compare_exchange_strong_explicit(&events[i].used, &expected, 1,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire))
      continue;
    atomic_store_explicit(&events[i].key, key, memory_order_release);
    for (int l = 0; l < MPIWRAPPER_T_CB_LEVELS; ++l) {
      atomic_store_explicit(&events[i].cb[l], NULL, memory_order_relaxed);
      atomic_store_explicit(&events[i].cb_data[l], NULL, memory_order_relaxed);
    }
    atomic_store_explicit(&events[i].dropped_fn, NULL, memory_order_relaxed);
    atomic_store_explicit(&events[i].free_fn, NULL, memory_order_relaxed);
    atomic_store_explicit(&events[i].free_data, NULL, memory_order_release);
    return &events[i];
  }

  /* Another thread may have interned the key between our two passes. */
  return find(key);
}

int mpiwrapper_t_event_set_cb(MPI_T_event_registration    reg,
                              int                         abi_cb_safety,
                              MPIABI_T_event_cb_function *abi_fn,
                              void                       *user_data)
{
  const int level = safety_index(abi_cb_safety);
  if (level < 0) return 0;

  struct event_entry *const e = intern(MPIWRAPPER_BITS(reg));
  if (!e) return 0;

  /* A null function removes the association, which the standard makes an
   * explicit case rather than an error; storing the null is that.
   */
  atomic_store_explicit(&e->cb_data[level], user_data, memory_order_relaxed);
  atomic_store_explicit(&e->cb[level], abi_fn, memory_order_release);
  return 1;
}

int mpiwrapper_t_event_set_dropped(MPI_T_event_registration            reg,
                                   MPIABI_T_event_dropped_cb_function *abi_fn)
{
  struct event_entry *const e = intern(MPIWRAPPER_BITS(reg));
  if (!e) return 0;

  atomic_store_explicit(&e->dropped_fn, abi_fn, memory_order_release);
  return 1;
}

int mpiwrapper_t_event_set_free(MPI_T_event_registration         reg,
                                MPIABI_T_event_free_cb_function *abi_fn,
                                void                            *user_data)
{
  struct event_entry *const e = intern(MPIWRAPPER_BITS(reg));
  if (!e) return 0;

  atomic_store_explicit(&e->free_data, user_data, memory_order_relaxed);
  atomic_store_explicit(&e->free_fn, abi_fn, memory_order_release);
  return 1;
}

/* The trampolines are declared through the implementation's own typedefs
 * (internal.h), so each signature is checked against its header rather than
 * written out twice -- the same discipline as the variadic error-handler
 * trampolines of #6.1, and for the same reason. They stay inside the
 * registration-type guard, because everything above them is inside it: an
 * implementation with the callback entry points but no registration type
 * cannot exist, and nesting says so rather than hoping.
 */

#  ifdef MPIWRAPPER_HAVE_MPI_T_event_register_callback
void mpiwrapper_t_event_cb_tramp(MPI_T_event_instance     event_instance,
                                 MPI_T_event_registration event_registration,
                                 MPI_T_cb_safety cb_safety, void *user_data)
{
  (void)user_data; /* the map answers this, not the registration (see above) */

  const struct event_entry *const e = find(MPIWRAPPER_BITS(event_registration));
  if (!e) return;

  const int abi_cb_safety = mpiwrapper_tcbsafety_toabi(cb_safety);
  const int level         = safety_index(abi_cb_safety);
  if (level < 0) return;

  MPIABI_T_event_cb_function *const fn =
      atomic_load_explicit(&e->cb[level], memory_order_acquire);
  if (!fn) return;

  fn(mpiwrapper_t_event_instance_toabi(event_instance),
     mpiwrapper_t_event_registration_toabi(event_registration), abi_cb_safety,
     atomic_load_explicit(&e->cb_data[level], memory_order_acquire));
}
#  endif

#  ifdef MPIWRAPPER_HAVE_MPI_T_event_set_dropped_handler
void mpiwrapper_t_event_dropped_tramp(
    MPI_Count count, MPI_T_event_registration event_registration,
    int source_index, MPI_T_cb_safety cb_safety, void *user_data)
{
  (void)user_data;

  const struct event_entry *const e = find(MPIWRAPPER_BITS(event_registration));
  if (!e) return;

  MPIABI_T_event_dropped_cb_function *const fn =
      atomic_load_explicit(&e->dropped_fn, memory_order_acquire);
  if (!fn) return;

  const int abi_cb_safety = mpiwrapper_tcbsafety_toabi(cb_safety);
  const int level         = safety_index(abi_cb_safety);

  fn((MPIABI_Count)count,
     mpiwrapper_t_event_registration_toabi(event_registration), source_index,
     abi_cb_safety,
     level < 0 ? NULL
               : atomic_load_explicit(&e->cb_data[level],
                                      memory_order_acquire));
}
#  endif

#  ifdef MPIWRAPPER_HAVE_MPI_T_event_handle_free
void mpiwrapper_t_event_free_tramp(MPI_T_event_registration event_registration,
                                   MPI_T_cb_safety cb_safety, void *user_data)
{
  (void)user_data;

  struct event_entry *const e = find(MPIWRAPPER_BITS(event_registration));
  if (!e) return;

  MPIABI_T_event_free_cb_function *const fn =
      atomic_load_explicit(&e->free_fn, memory_order_acquire);
  if (fn)
    fn(mpiwrapper_t_event_registration_toabi(event_registration),
       mpiwrapper_tcbsafety_toabi(cb_safety),
       atomic_load_explicit(&e->free_data, memory_order_acquire));

  /* #6.2's reclamation point. Released after the user's callback has run and
   * not before: the handle it is given is converted out of this very entry.
   */
  atomic_store_explicit(&e->used, 0, memory_order_release);
}
#  endif

#endif /* MPIWRAPPER_HAVE_MPI_T_event_registration */
