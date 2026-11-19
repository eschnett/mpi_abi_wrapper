# Does MPI_File_write_at(_all) write past the MPI_Status it is given?
# The status is a guard buffer of 0xAA bytes, 8 statuses long; each rank
# prints the last byte that changed, against sizeof(MPI_Status) -- the
# implementation's when MPI.jl loads it directly, the ABI's when it loads
# libmpi_abi. Two calls: doubles at an aligned offset, and the call that
# first crashed (TreeIOHDF5's bench/checkpoint_stress.jl, its coords column:
# 3 UInt32 a row at the unaligned offset 94549).
#
#     mpiexec -n 8 julia --project=<env> status_guard.jl <file on the FS under test>
using MPI
MPI.Init()
comm = MPI.COMM_WORLD
r = MPI.Comm_rank(comm)
path = ARGS[1]
fh = MPI.File.open(comm, path; write=true, create=true)
n = 395
S = sizeof(MPI.Status)
dbl = fill(Float64(r), 1000)
crd = fill(UInt32(r), 3, n)
cases = (("double-aligned", dbl, Float64, Int64(r * 8000)),
         ("coords-unaligned", crd, UInt32, Int64(94549 + r * 12n)))
for (what, buf, T, off) in cases,
    (name, f) in (("write_at_all", MPI.API.MPI_File_write_at_all),
                  ("write_at", MPI.API.MPI_File_write_at))
    guard = fill(0xAA, 8S)
    GC.@preserve guard buf begin
        f(fh, off, buf, length(buf), MPI.Datatype(T), pointer(guard))
    end
    last = something(findlast(!=(0xAA), guard), 0)
    println("$what $name sizeof(MPI_Status)=$S last byte written=$last",
            last > S ? "  OVERRUN" : "")
    MPI.Barrier(comm)
end
close(fh)
MPI.Barrier(comm)
r == 0 && rm(path; force=true)
MPI.Finalize()
