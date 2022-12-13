#!/bin/sh
if [ $# -ne 2 ]
    then 
        echo "Usage: sh load_program.sh <ELF-file> <target>"
        exit 1
fi

SIZE=$(ls -rtl $1 | awk '{printf "%x\n", $5}')

echo "Sending ELF file size $SIZE"
echo $SIZE > $2

echo "Sending ELF file!"
cat $1 > $2

echo "Sent ELF file!"
