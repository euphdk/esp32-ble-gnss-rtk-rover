#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
out="${TMPDIR:-/tmp}/gnss-protocol-test"
c++ -std=c++17 -Wall -Wextra -Werror -I"$root/include" "$root/test/host/test_protocol.cpp" "$root/src/nmea.cpp" -o "$out"
"$out"
for receiver in GNSS_RECEIVER_ZED_F9P GNSS_RECEIVER_LC29H_DA; do
  c++ -std=c++17 -Wall -Wextra -Werror -DGNSS_RECEIVER="$receiver" -I"$root/include" \
    "$root/test/host/test_receiver.cpp" -o "$out-receiver"
  "$out-receiver"
done
echo "host protocol tests passed"
