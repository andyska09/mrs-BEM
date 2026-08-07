#!/bin/bash
# ./apply.sh BASEPATH CONFIG [filelist]
#   BASEPATH  directory holding merged_*_seg_*.csv
#   CONFIG    model yaml; its `name:` field becomes store/preds/<name>/
#   filelist  optional file of <flight>_seg_X ids; default is every segment
# Existing outputs are skipped.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/cpp/build/mybem-apply"

if [ $# -lt 2 ]; then
    echo "usage: ./apply.sh BASEPATH CONFIG [filelist]"
    exit 1
fi

base="${1%/}"
config="$2"

name="$(sed -n 's/^name:[[:space:]]*//p' "$config" | head -1)"
[ -n "$name" ] || { echo "$config: no name: field"; exit 1; }
out="$ROOT/store/preds/$name"
mkdir -p "$out"

if [ -n "${3:-}" ]; then
    ids=$(cat "$3")
else
    ids=$(ls "$base"/merged_*seg*.csv | sed 's|.*/merged_||; s|\.csv$||')
fi

echo "$config -> store/preds/$name"
for id in $ids; do
    [ -f "$out/$id.csv" ] || "$BIN" "$config" "$base/merged_$id.csv" "$out/$id.csv"
done
