#!/usr/bin/env sh
# What a named ELF version node does to a client binary's portability.
#
# Run on Linux (glibc); see README.md. Everything happens in a temporary
# directory and nothing is left behind.
set -e

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work"

# A stand-in for libmpi_abi: two of the ABI's exported names plus one internal
# symbol, so the export *filtering* both scripts do is visible alongside the
# versioning only one of them does.
cat > lib.c <<'C'
int MPI_Send(void) { return 1; }
int PMPI_Send(void) { return 1; }
int internal_helper(void) { return 2; }
C

# The two version scripts, differing in exactly one identifier.
printf 'MPIABI_1 { global: MPI_*; PMPI_*; local: *; };\n' > named.ver
printf '{ global: MPI_*; PMPI_*; local: *; };\n' > anon.ver

cc -shared -fPIC -fvisibility=default -o libnamed.so lib.c \
   -Wl,--version-script=named.ver
cc -shared -fPIC -fvisibility=default -o libanon.so lib.c \
   -Wl,--version-script=anon.ver

echo "=== 1. Both scripts filter identically (internal_helper is in neither)"
echo "--- named:"; nm -D --defined-only libnamed.so | sed 's/^/    /'
echo "--- anon:";  nm -D --defined-only libanon.so  | sed 's/^/    /'

# An ordinary application, linked against each in turn.
cat > app.c <<'C'
int MPI_Send(void);
int main(void) { return MPI_Send() - 1; }
C
cc -o app_named app.c -L. -lnamed -Wl,-rpath,"$work"
cc -o app_anon  app.c -L. -lanon  -Wl,-rpath,"$work"

echo
echo "=== 2. What the CLIENT BINARY records as its undefined reference"
echo "--- built against named:"
readelf --dyn-syms -W app_named | grep ' MPI_Send' | sed 's/^/    /'
echo "--- built against anon:"
readelf --dyn-syms -W app_anon  | grep ' MPI_Send' | sed 's/^/    /'

# The swap the standard ABI exists to permit: another implementation's
# libmpi_abi, which defines the same names and has never heard of MPIABI_1.
cc -shared -fPIC -fvisibility=default -o libother.so lib.c
mkdir -p other
cp libother.so other/libnamed.so
cp libother.so other/libanon.so

echo
echo "=== 3. THE SWAP: run each app against another implementation's libmpi_abi"
for v in named anon; do
  printf '    %-6s: ' "$v"
  if LD_LIBRARY_PATH="$work/other" ./app_$v 2>&1; then
    echo "RAN"
  else
    echo "    ^ FAILED TO START (exit $?)"
  fi
done
