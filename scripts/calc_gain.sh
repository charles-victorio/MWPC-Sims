#!/bin/bash

VOLTAGE="$1"

cat charges_track_${VOLTAGE}V_*.csv | awk -F, '{s+=$2; n++} END{print s/n; print n}'