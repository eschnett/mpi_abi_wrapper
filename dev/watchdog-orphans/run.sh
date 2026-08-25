#!/usr/bin/env bash
#
# Does ci-scripts/suite/mpiexec-filter's watchdog outlive the test it guards?
#
#   dev/watchdog-orphans/run.sh [filter] [iterations]
#
# `filter` defaults to ci-scripts/suite/mpiexec-filter; point it at an older
# copy to see the pre-fix behaviour:
#
#   git show <rev>:ci-scripts/suite/mpiexec-filter >/tmp/old && chmod +x /tmp/old
#   dev/watchdog-orphans/run.sh /tmp/old
#
# Needs no MPI: the launcher is a stub, because the question is about this
# script's own process handling and not about anything MPI does.
#
# Three checks, and the fix has to pass all three -- the leak is easy to close
# in a way that breaks the two properties the watchdog exists for.

set -u

here=$(cd -- "$(dirname -- "$0")" && pwd)
repo=$(cd -- "$here/../.." && pwd)
filter=${1:-$repo/ci-scripts/suite/mpiexec-filter}
iters=${2:-5}

[ -x "$filter" ] || { echo "not executable: $filter" >&2; exit 2; }

# 9300 + the script's own 15 = 9315, a sleep length nothing else on the machine
# is plausibly using, so pgrep can count ours without matching a bystander.
marker_timeout=9300
marker_sleep=9315
hang_sleep=8600

work=$(mktemp -d) || exit 2
trap 'pkill -f "sleep $marker_sleep" 2>/dev/null; pkill -f "sleep $hang_sleep" 2>/dev/null; rm -rf "$work"' EXIT

cat > "$work/exits-at-once" <<'EOF'
#!/bin/sh
exit 0
EOF
cat > "$work/hangs" <<EOF
#!/bin/sh
sleep $hang_sleep &
wait
EOF
chmod +x "$work/exits-at-once" "$work/hangs"

export MPIEXEC_FILTER_KIND=other
fails=0
say() { printf '%-52s %s\n' "$1" "$2"; }

# 1. The leak. Each run's launcher exits at once, so the watchdog is still
#    sleeping when the filter reaps it. Signalling the subshell without its
#    process group leaves the `sleep` behind, one per test.
MPIEXEC_FILTER_LAUNCHER=$work/exits-at-once MPIEXEC_TIMEOUT=$marker_timeout
export MPIEXEC_FILTER_LAUNCHER MPIEXEC_TIMEOUT
i=0
while [ "$i" -lt "$iters" ]; do "$filter" -n 2 /bin/true >/dev/null 2>&1; i=$((i + 1)); done
sleep 1
orphans=$(pgrep -f "sleep $marker_sleep" | wc -l | tr -d ' ')
say "orphan watchdog sleeps after $iters fast runs" "$orphans (want 0)"
[ "$orphans" = 0 ] || fails=$((fails + 1))
pkill -f "sleep $marker_sleep" 2>/dev/null

# 2. The watchdog still has to fire. Killing the subshell's whole group must not
#    mean killing it before it has done its job.
MPIEXEC_FILTER_LAUNCHER=$work/hangs MPIEXEC_TIMEOUT=1
export MPIEXEC_FILTER_LAUNCHER MPIEXEC_TIMEOUT
start=$SECONDS
"$filter" -n 2 /bin/true >/dev/null 2>&1
status=$?
elapsed=$((SECONDS - start))
left=$(pgrep -f "sleep $hang_sleep" | wc -l | tr -d ' ')
say "hung launcher: exit status" "$status (want 143)"
say "hung launcher: seconds to give up" "$elapsed (want 16, = 1 + 15)"
say "hung launcher: ranks left behind" "$left (want 0)"
[ "$status" = 143 ] || fails=$((fails + 1))
[ "$elapsed" -ge 14 ] && [ "$elapsed" -le 25 ] || fails=$((fails + 1))
[ "$left" = 0 ] || fails=$((fails + 1))
pkill -f "sleep $hang_sleep" 2>/dev/null

# 4. `set -m` has to stay quiet. Job control in a non-interactive shell can
#    announce background jobs as `[1] 12345` on stderr, and the suite counts
#    stray output as a test failure -- so the fix for a leak nobody sees must not
#    buy it with output everybody sees. Nothing else in a fast run writes
#    anything, so the bar is simply: no bytes.
MPIEXEC_FILTER_LAUNCHER=$work/exits-at-once MPIEXEC_TIMEOUT=$marker_timeout
export MPIEXEC_FILTER_LAUNCHER MPIEXEC_TIMEOUT
noise=$("$filter" -n 2 /bin/true 2>&1 | wc -c | tr -d ' ')
say "bytes of stdout+stderr from a fast run" "$noise (want 0)"
[ "$noise" = 0 ] || { fails=$((fails + 1)); "$filter" -n 2 /bin/true 2>&1 | sed 's/^/    | /'; }
pkill -f "sleep $marker_sleep" 2>/dev/null

# 3. The property the redirections in that script protect, restated as a test:
#    runtests reads the filter's stdout to EOF before looking at the status, so a
#    watchdog holding the write end makes every test appear to take the full
#    timeout. Reading through `cat` is what makes that visible.
MPIEXEC_FILTER_LAUNCHER=$work/exits-at-once MPIEXEC_TIMEOUT=$marker_timeout
export MPIEXEC_FILTER_LAUNCHER MPIEXEC_TIMEOUT
start=$SECONDS
"$filter" -n 2 /bin/true 2>/dev/null | cat >/dev/null
elapsed=$((SECONDS - start))
say "fast run read to EOF: seconds" "$elapsed (want 0..2, not $marker_sleep)"
[ "$elapsed" -le 2 ] || fails=$((fails + 1))
pkill -f "sleep $marker_sleep" 2>/dev/null

echo
if [ "$fails" = 0 ]; then echo "PASS: $(basename "$filter")"; else echo "FAIL: $fails check(s), $(basename "$filter")"; fi
exit $((fails == 0 ? 0 : 1))
