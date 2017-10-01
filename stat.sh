#!/bin/sh

# Author: Bharath Banglaore Veeranna

# File to collect memory usage statistics of the Vm

totalS="$(cat /proc/meminfo | grep "MemTotal")"

echo $totalS > mem_temp.txt

total="$(cut -d' ' -f2 mem_temp.txt)"

freeS="$(cat /proc/meminfo | grep "MemFree")"

echo $freeS > mem_temp.txt

free="$(cut -d' ' -f2 mem_temp.txt)"

used=$(expr $total - $free )

pct=$(expr $used / 41943)

echo $pct > mem_temp.txt