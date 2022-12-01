#!/bin/sh
SIZE=$(ls -rtl $1 | awk '{printf "%x\n", $5}')

echo "Sending ELF file size $SIZE"
echo $SIZE > $2

echo "Sending ELF file!"
cat $1 > $2
