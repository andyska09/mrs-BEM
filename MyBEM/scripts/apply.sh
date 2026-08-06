#!/bin/bash
# ./apply.sh BASEPATH MODEL CONFIG [filelist]
#   BASEPATH  directory holding merged_*_seg_*.csv
#   MODEL     output subfolder, created under MyBEM/store/preds/
#   CONFIG    model yaml
#   filelist  optional file of <flight>_seg_X ids; default is every segment
# Existing outputs are skipped.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/cpp/build/mybem-apply"

if [ $# -lt 3 ]; then
    echo "usage: ./apply.sh BASEPATH MODEL CONFIG [filelist]"
    exit 1
fi

base="${1%/}"
out="$ROOT/store/preds/$2"
config="$3"
mkdir -p "$out"

if [ -n "${4:-}" ]; then
    ids=$(cat "$4")
else
    ids=$(ls "$base"/merged_*seg*.csv | sed 's|.*/merged_||; s|\.csv$||')
fi

for id in $ids; do
    [ -f "$out/$id.csv" ] || "$BIN" "$config" "$base/merged_$id.csv" "$out/$id.csv"
done
