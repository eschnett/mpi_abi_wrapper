#!/usr/bin/env bash
# The measurements of README.md, on an Open MPI whose ROMIO is a component
# (HPC-X 2.20's Open MPI 4.1.7 on Symmetry, where they were taken). Run inside
# an allocation, with the MPI's own mpiexec and ompi_info on PATH:
#
#   dev/component-probe/run.sh <env-abi> <env-direct> <scratch dir> [ranks]
#
# <env-abi> is a Julia project whose MPIPreferences name this project's
# libmpi_abi (abi "MPIABI"); <env-direct> one that names the implementation's
# libmpi (abi "OpenMPI"). Both need MPI.jl and nothing else.
set -u
abi=$1 direct=$2 dir=$3 ranks=${4:-8}
here=$(cd "$(dirname "$0")" && pwd)
julia=$(julia --startup-file=no -e 'print(joinpath(Sys.BINDIR, "julia"))')

echo "=== which public MPI names the I/O components import, and what they link"
lib=$(dirname "$(command -v ompi_info)")/../lib/openmpi
for c in "$lib"/mca_io_*.so; do
    echo "$(basename "$c"): $(nm -D --undefined-only "$c" | grep -c -E ' P?MPI_') MPI_/PMPI_ imports;" \
         "NEEDED $(readelf -d "$c" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' | tr '\n' ' ')"
done

for env in "$abi" "$direct"; do
    "$julia" --project="$env" -e 'using MPI' >/dev/null 2>&1      # precompile once
    for io in romio321 ompio; do
        echo "=== $(basename "$env") io=$io: the status written by MPI_File_write_at(_all)"
        mpiexec -n "$ranks" --bind-to none --mca io $io -x PATH \
            "$julia" --project="$env" "$here/status_guard.jl" "$dir/status-guard-$$.bin" 2>&1 \
            | grep -o -E '(double|coords)-[a-z]+ write_at(_all)? sizeof\(MPI_Status\)=[0-9]+ last byte written=[0-9]+( +OVERRUN)?|ERROR.*|stack smashing detected' \
            | sort | uniq -c
    done
    echo "=== $(basename "$env") io=romio321: where the ROMIO component's calls bind"
    log=$(mktemp -d)
    mpiexec -n 2 --bind-to none --mca io romio321 -x PATH -x LD_DEBUG=bindings \
        -x LD_DEBUG_OUTPUT="$log/b" "$julia" --project="$env" "$here/status_guard.jl" \
        "$dir/status-guard-$$.bin" >/dev/null 2>&1
    cat "$log"/b.* | grep -E "binding file .*mca_io_romio321.so .*: normal symbol \`(MPI_Status_set_elements_x|MPI_Type_size_x)'" \
        | sed -E 's/^ *[0-9]+: *//; s|/[^ ]*/||g' | sort | uniq -c
    rm -rf "$log"
done
