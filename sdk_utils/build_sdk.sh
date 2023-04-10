 #!/bin/sh
SEL4CP_ABS_PATH="/home/ttn/Desktop/sel4cp"
SEL4_ABS_PATH="/home/ttn/Desktop/seL4"
POC_ABS_PATH="/home/ttn/Desktop/seL4CP_poc"
 
(cd $SEL4CP_ABS_PATH && $SEL4CP_ABS_PATH/pyenv/bin/python $SEL4CP_ABS_PATH/build_sdk.py --sel4=$SEL4_ABS_PATH) 
rm -rf $POC_ABS_PATH/sel4cp-sdk-1.2.6 
cp -rf $SEL4CP_ABS_PATH/release/sel4cp-sdk-1.2.6 $POC_ABS_PATH/
