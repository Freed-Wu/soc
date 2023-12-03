#!/usr/bin/env bash
cleanup() {
  # must keep order, don't `pkill -P$$`!
  pkill -9 main &&
    pkill -9 socat &&
    pkill -9 journalctl
}
trap cleanup EXIT

socat pty,rawer,link=/tmp/ttyS0 pty,rawer,link=/tmp/ttyS1 &
journalctl -tslave0 -tmaster -fn0 &
# must wait journalctl to finish
sleep 0.01
main -dv &> /dev/null &
master -v CMakeLists.txt Makefile &> /dev/null
