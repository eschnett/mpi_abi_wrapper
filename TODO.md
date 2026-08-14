# Erik's open review questions

- test against MVAPICH as well

- "An implementation request handle does not uniquely identify an
  operation": why would this be relevant? is this for callbacks?

- allow MPI 3.0

- "Stage any array whose element type is not *identical* on both
  sides, even when no value mapping is needed": no. we design against
  the system ABI, not the language API, and these mismatches don't
  matter. verify this: the the system ABIs, and whether the compilers
  behave unexpectedly for such casts.

- discuss weak definitions and dlopen on macos, i need to understand
  the current state
