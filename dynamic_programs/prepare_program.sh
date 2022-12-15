#!/bin/sh
if [ $# -ne 2 ]
    then 
        echo "Usage: sh prepare_program.sh <ELF-file-name> <access-rights-file-name>"
        exit 1
fi

BASE_PATH=/home/ttn/Desktop/seL4CP_poc

cp $BASE_PATH/build/$1 $BASE_PATH/dynamic_programs/$1
python3 $BASE_PATH/elf_utilities/set_up_access_rights.py $BASE_PATH/dynamic_programs/$1 $BASE_PATH/configuration.system $BASE_PATH/dynamic_programs/$2
