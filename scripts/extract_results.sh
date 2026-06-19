#!/bin/bash

echo "=== CADRRS SUMMARY ==="

grep -A8 "CADRRS SUMMARY" \
../results/stage25_route_switch.txt

echo
echo "=== ROUTE SWITCH COUNT ==="

cat ../results/route_switch_count.txt

echo
echo "=== RELIABILITY DISTRIBUTION ==="

cat ../results/stage233_distribution.txt
