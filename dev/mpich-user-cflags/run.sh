#!/usr/bin/env bash

# Does a user's CFLAGS reach MPICH's embedded modules? See README.md.
#
#   dev/mpich-user-cflags/run.sh [<tag>...]     # default: 5.0.1 5.0.2rc1
#
# The probe is `configure` alone -- no build -- because the question is what each
# sub-configure is *handed*, and configure prints that. That makes it a few
# minutes per tag rather than the half-hour a 32-bit build costs, and it needs no
# 32-bit anything: the dropped flag is architecture-independent, and the ILP32
# row is only where the consequence happens to be fatal.

set -uo pipefail

tags=${*:-"5.0.1 5.0.2rc1"}
marker=-Wno-error=incompatible-pointer-types
work=${MPIABI_WORK:-$(mktemp -d)} || exit 1
mkdir -p "$work" || exit 1       # mktemp -d makes its own; an override may not
echo "work: $work"
status=0

# The modules to report on, and which bracket each one is configured inside.
# "reset" means the sub-configure is wrapped in PAC_PUSH_ALL_FLAGS /
# PAC_RESET_ALL_FLAGS / PAC_POP_ALL_FLAGS and is therefore handed $USER_CFLAGS;
# "plain" means it sees MPICH's accumulated $CFLAGS.
modules="src/mpl:plain src/pmi:plain src/mpi/romio:plain src/pm/hydra:plain
         modules/libfabric:reset modules/hwloc:reset modules/json-c:reset
         src/mpi/datatype/typerep/yaksa:reset"

for tag in $tags; do
  src=$work/mpich-$tag
  tarball=$work/mpich-$tag.tar.gz
  [ -f "$tarball" ] || curl -fsSL -o "$tarball" \
      "https://www.mpich.org/static/downloads/$tag/mpich-$tag.tar.gz" \
    || { echo "download failed for $tag" >&2; status=1; continue; }
  [ -d "$src" ] || tar -C "$work" -xzf "$tarball" \
    || { echo "unpack failed for $tag" >&2; status=1; continue; }

  echo
  echo "=============== mpich $tag"
  echo "--- PAC_PREFIX_FLAG, which is what takes the snapshot:"
  sed -n '/PAC_PREFIX_FLAG - /,/^])/p' "$src/confdb/aclocal_util.m4" | sed 's/^/    /'

  b=$work/build-$tag
  mkdir -p "$b"
  ( cd "$b" && "$src/configure" --prefix="$work/opt-$tag" \
        CFLAGS="-g -O2 $marker" >configure.log 2>&1 )
  echo "--- configure exited $? (log: $b/configure.log)"

  echo "--- what each sub-configure was handed:"
  for entry in $modules; do
    mod=${entry%:*}; kind=${entry#*:}
    line=$(grep -a "running.*/$mod/configure" "$b/configure.log" | tail -1)
    case $line in
      "")            verdict="not configured" ;;
      *"$marker"*)   verdict="HAS the flag" ;;
      *)             verdict="**flag dropped**" ;;
    esac
    printf '    %-8s %-40s %s\n' "$kind" "$mod" "$verdict"
  done
done

echo
echo "Expect: 5.0.1 has the flag everywhere; 5.0.2rc1 drops it for every"
echo "'reset' module and keeps it for every 'plain' one."
exit $status
