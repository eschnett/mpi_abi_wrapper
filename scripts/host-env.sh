#!/usr/bin/env bash

# Everything this project needs to know about the machine it is developed on.
#
#   . scripts/host-env.sh                            # export into this shell
#   scripts/host-env.sh mpiexec -n 4 ./prog          # set them for one command
#   scripts/host-env.sh ctest --test-dir build/mpich --output-on-failure
#   eval "$(scripts/host-env.sh)"                    # for a shell that cannot source it
#
# **This is configuration for one laptop, and for nothing else.** Every variable
# below works around a property of *that machine* -- a firewall setting, a VPN
# interface -- and none of it is a statement about macOS, about MPICH, about
# Open MPI, or about any other host. It lives here, in a script this repository
# owns and each user opts into by name, rather than in ~/.pmix/mca-params.conf,
# ~/.openmpi/mca-params.conf or a shell profile, because in any of those it
# would silently reshape every MPI run by this account, including runs with
# nothing to do with this project, and nothing would record why.
#
# The two environment quirks it settles are written up in test/README.md; that
# is the place to read before changing anything here. Both are the same shape:
# an MPI picks a network interface on its own, and on this host the interface it
# picks does not work.
#
# --- Open MPI 5.0.x: pin the launcher and the TCP BTL to loopback -------------
#
# Without these, `mpiexec -n 2` yields two singletons rather than a job. This
# machine has the macOS application firewall enabled, and it blocks incoming
# connections to prte -- as it does to every locally built binary that has ever
# listened. PMIx 5 dropped its Unix-socket transport, so all client-to-server
# PMIx traffic is TCP, and PMIx omits loopback devices from its interface list
# by default, so prterun advertises the en0 address and the ranks cannot reach
# their own launcher. Loopback is exempt from the firewall.
#
#   PMIX_MCA_pif_base_retain_loopback   puts lo0 back in PMIx's interface list;
#       PMIx prefers loopback once it can see it. Enough alone for 5.0.10.
#   PMIX_MCA_ptl_base_if_include        forces that choice instead of relying on
#       the preference: 5.0.6's PRRTE has remote_connections set and takes a
#       non-loopback device even when lo0 is visible.
#   OMPI_MCA_btl_tcp_if_include         the data path, not the launcher. The TCP
#       BTL otherwise picks utun40 (198.19.254.2, a VPN interface) and fails
#       with "No route to host". On-node runs never reach it, since `sm` carries
#       them; MPI_Comm_spawn does, parent and child being separate jobs.
#
# Open MPI 6.1.0a1 needs none of this -- its PMIx keeps loopback already -- and
# is unharmed by it.
#
# --- MPICH: keep ch4:ofi off the VPN interface --------------------------------
#
#   FI_PROVIDER=tcp   MPICH's ch4:ofi picks utun0 when the VPN is up, and
#       MPI_Finalize then fails with "OFI poll failed (default nic=utun0)".
#       Same underlying fact as the OMPI_MCA_btl_tcp_if_include line above,
#       reached through libfabric instead.
#
# --- Why one script rather than two -------------------------------------------
#
# The four are set unconditionally, because each is inert for the MPIs it is not
# addressed to and the combination has been measured rather than assumed: with
# all four exported, test/'s suite passes 13/13 against MPICH 4.3.1, against
# Open MPI 5.0.6 and against Open MPI 5.0.10, and a wrapper-free stress program
# passes under Open MPI 6.1.0a1. So there is nothing to dispatch on, and a
# script that needed to know which MPI you meant would be a worse interface than
# one you can put in front of any command.

mpiabi_host_env_sourced=0
if [ -n "${ZSH_EVAL_CONTEXT:-}" ]; then
  case $ZSH_EVAL_CONTEXT in *file*) mpiabi_host_env_sourced=1 ;; esac
elif [ -n "${BASH_SOURCE[0]:-}" ] && [ "${BASH_SOURCE[0]}" != "$0" ]; then
  mpiabi_host_env_sourced=1
fi

export PMIX_MCA_pif_base_retain_loopback=1
export PMIX_MCA_ptl_base_if_include=lo0
export OMPI_MCA_btl_tcp_if_include=lo0
export FI_PROVIDER=tcp

if [ "$mpiabi_host_env_sourced" = 1 ]; then
  unset mpiabi_host_env_sourced
elif [ $# -gt 0 ]; then
  unset mpiabi_host_env_sourced
  exec "$@"
else
  echo "export PMIX_MCA_pif_base_retain_loopback=1"
  echo "export PMIX_MCA_ptl_base_if_include=lo0"
  echo "export OMPI_MCA_btl_tcp_if_include=lo0"
  echo "export FI_PROVIDER=tcp"
fi
