#!/bin/bash
# usage: apply.sh [MODEL.yaml] [OUTDIR] [IDLIST]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

MODEL="${1:-$ROOT/configs/models/bem_default.yaml}"
OUT="${2:-$ROOT/store/preds/$(basename "$MODEL" .yaml)}"
IDS="${3:-$ROOT/../NeuroBEM/testset.txt}"
SRC="$ROOT/../NeuroBEM/processed_data"

mkdir -p "$OUT"

n=0
while read -r id; do
    [ -z "$id" ] && continue
    in="$SRC/merged_$id.csv"
    if [ ! -f "$in" ]; then
        echo "missing: $in"
        continue
    fi
    "$ROOT/cpp/build/mybem-apply" "$MODEL" "$in" "$OUT/$id.csv"
    n=$((n + 1))
done < "$IDS"

echo "$n segments -> $OUT"
