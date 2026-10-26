#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
out="${TMPDIR:-/tmp}/gnss-protocol-test"
c++ -std=c++17 -Wall -Wextra -Werror -I"$root/include" "$root/test/host/test_protocol.cpp" "$root/src/nmea.cpp" -o "$out"
"$out"
echo "host protocol tests passed"
