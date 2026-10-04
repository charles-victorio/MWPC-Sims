#!/bin/sh

VOLTAGE="$1"

for f in charges_track_${VOLTAGE}V_*.csv; do
  awk -F, 'BEGIN{s=0} {s+=$2} END{print s}' "$f"
done