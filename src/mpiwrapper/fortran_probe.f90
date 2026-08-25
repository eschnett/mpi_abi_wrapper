! libmpiwrapper -- what this build's Fortran compiler does with LOGICAL, REAL
! and INTEGER (NOTES.md #8, decision 25).
!
! The only Fortran in this project, and it exists because MPI-5.0 20.4.1 asks a
! question C cannot answer. MPI_ABI_GET_FORTRAN_BOOLEANS hands back the byte
! patterns of .TRUE. and .FALSE., and the standard passes those literals by
! address rather than by value precisely because their representation is not
! knowable from C -- there is no MPI call, and no arithmetic on any MPI
! datatype, that yields a .TRUE. one has not already been given. So the values
! come from a Fortran compiler or they do not come at all.
!
! **Compiled and linked, never run at configure time.** NOTES.md #1 forbids
! deciding anything by running a program while configuring, because
! cross-compiling would then be impossible; this is a translation unit inside
! libmpiwrapper whose subroutine the C side calls at run time, on the target.
!
! **It must not drag in a Fortran runtime.** libmpiwrapper is loaded into a
! process that may already have the application's own libgfortran (mpif's, for
! one), and a second, differently-versioned copy of a Fortran runtime in one
! process is its own class of failure. Hence bind(C), no I/O, no allocatables
! and no derived types: measured with `nm -u`, the object's only undefined
! symbols are malloc, free and memcpy, all libc, and it links with cc alone.
! Anything added here should be checked the same way.
!
! **Whose compiler this is.** This build's, which need not be the application's
! -- but that is exactly the position a native ABI implementation is in, since
! MPICH and Open MPI report the properties of whatever Fortran compiler *they*
! were built with. The standard's model is that the application may override
! any of it with MPI_ABI_SET_FORTRAN_BOOLEANS, and that setter still wins here.
!
! Which compiler that is, the build picks deliberately: the wrapped MPI's own
! mpifort, when it has one that works, so that "this build's compiler" and "the
! compiler the wrapped MPI reports for" are the same compiler rather than two
! that happen to agree (CMakeLists.txt's Fortran block, NOTES.md #9). When they
! cannot be -- no such wrapper, or one whose backend is missing -- hw_abi.c's
! run-time comparison against PMPI_Type_size(MPI_LOGICAL) is what keeps a
! disagreement from being answered confidently.

subroutine mpiwrapper_fortran_probe(logical_size, integer_size, real_size, &
                                    nbytes, true_bytes, false_bytes) &
     bind(C, name="mpiwrapper_fortran_probe")
  use, intrinsic :: iso_c_binding, only: c_int, c_signed_char
  implicit none

  integer(c_int), intent(out) :: logical_size, integer_size, real_size
  integer(c_int), intent(in), value :: nbytes
  integer(c_signed_char), intent(out) :: true_bytes(nbytes)
  integer(c_signed_char), intent(out) :: false_bytes(nbytes)

  logical :: lt, lf
  ! A mold for TRANSFER, and a fixed size on purpose: a runtime-sized TRANSFER
  ! is the one shape of this subroutine that allocates.
  integer(c_signed_char) :: mold(16)
  integer :: n, i

  lt = .true.
  lf = .false.

  ! storage_size is in bits and is the default kind's, which is the kind
  ! MPI_LOGICAL, MPI_INTEGER and MPI_REAL name.
  logical_size = storage_size(lt) / 8
  integer_size = storage_size(0) / 8
  real_size    = storage_size(0.0) / 8

  ! Defined output before the copy, so a caller's buffer is never left holding
  ! its own stack bytes for the tail beyond logical_size.
  do i = 1, nbytes
     true_bytes(i)  = 0_c_signed_char
     false_bytes(i) = 0_c_signed_char
  end do

  n = min(logical_size, nbytes)
  if (n > 0) then
     true_bytes(1:n)  = transfer(lt, mold)
     false_bytes(1:n) = transfer(lf, mold)
  end if
end subroutine mpiwrapper_fortran_probe
