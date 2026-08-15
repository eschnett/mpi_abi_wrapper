# Erik's open review questions

- test against MVAPICH as well

- test with Intel and Nvidia compilers

- test FreeBSD

- add a licence (same as mpif)

- do not guard constants with `#ifdef`, this should be configure tests
  instead. you probably don't need one configure test per constant,
  you can test for groups of constants at once.
