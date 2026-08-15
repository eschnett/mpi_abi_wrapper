# Erik's open review questions

- test against MVAPICH as well

- test with Intel and Nvidia compilers

- test FreeBSD

- allow MPI 3.0

- add a licence (same as mpif)

- "Stage any array whose element type is not *identical* on both
  sides, even when no value mapping is needed": no. we design against
  the system ABI, not the language API, and these mismatches don't
  matter. verify this: the the system ABIs, and whether the compilers
  behave unexpectedly for such casts.

- re-think the overall approach and the stages. clean up the notes.
