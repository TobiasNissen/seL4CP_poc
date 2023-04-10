#!/bin/sh
if [ $# -ne 3 ]
    then 
        echo "Usage: sh prepare_program.sh <system-configuration-file> <ELF-file-name> <access-rights-file-name>"
        exit 1
fi

BASE_PATH=/home/ttn/Desktop/seL4CP_poc
LOCAL_PATH=$(python3 -m site --user-base)/bin

# Ensure that the directory, which the scripts from the protection_model module
# are added to, is part of the PATH environment variable.
if [ ! $(echo $PATH | grep -E "(^|:)$LOCAL_PATH($|:)") ]
    then
        export PATH=$PATH:$LOCAL_PATH
fi

# Ensure that the sel4cp_set_up_access_rights utility is installed.
pip3 install -q $BASE_PATH/protection_model-1.0.0-py2.py3-none-any.whl 


cp $BASE_PATH/build/$2 $BASE_PATH/dynamic_programs/$2
sel4cp_set_up_access_rights $BASE_PATH/dynamic_programs/$2 $1 $BASE_PATH/dynamic_programs/$3
