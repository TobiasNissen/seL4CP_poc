# Overview
This repository contains a small proof-of-concept for dynamically loading a program into a protection domain
in a system based on the seL4 Core Platform.

# seL4 Core Platform (seL4CP)
The seL4 Core Platform has been modified to support dynamic loading of ELF files in empty protection domains.
For simplicity, a build of the modified SDK has been included directly in this repository.
See the `poc` branch at https://github.com/TobiasNissen/sel4cp for the source code of the modified SDK.

# Building and running
The seL4CP-based system can be built by running `make` in the root of this repository.

The system can be started by running `make run`.

The protection domain `root_domain` uses the assigned character device to listen for input, expecting an ELF file extended with access rights.

To extend an ELF file with access rights, the `set_up_access_rights.py` utility can be used. This utility can be invoked in two ways:

1. Specifying the desired access rights dynamically via user input. To use this option, run:
`python3 ./elf_utilities/set_up_access_rights.py <path_to_ELF_file> ./configuration.system`
2. Providing the desired access rights from an XML file. To use this option, run:
`python3 ./elf_utilities/set_up_access_rights.py <path_to_ELF_file> ./configuration.system <path_to_access_rights_file>`

Before the utility program is run, the Python library `protection_model` must be installed with:
`pip3 install ./elf_utilities/protection_model-1.0.0-py2.py3-none-any.whl`. This library has been built from `https://github.com/TobiasNissen/protection_model`.

Once an ELF file extended with access rights has been created, the file can be loaded by running:
`sh ./dynamic_programs/load_program.sh <path_to_extended_ELF_file> <assigned_char_device>`
