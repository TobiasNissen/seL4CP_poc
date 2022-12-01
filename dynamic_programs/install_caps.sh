#!/bin/sh
ELF_NAME=hello_world.elf
ELF_NAME_LOW_PRIORITY=hello_world_low_priority.elf
ELF_NAME_HIGH_PRIORITY=hello_world_high_priority.elf

cp ../build/$ELF_NAME .
cp $ELF_NAME $ELF_NAME_LOW_PRIORITY
./elf_patcher $ELF_NAME_LOW_PRIORITY low_priority.txt

cp $ELF_NAME $ELF_NAME_HIGH_PRIORITY
./elf_patcher $ELF_NAME_HIGH_PRIORITY high_priority.txt

rm $ELF_NAME

