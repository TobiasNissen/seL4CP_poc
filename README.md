# Overview
This repository contains a small proof-of-concept for dynamically loading ELF programs 
in new protection domains in a system built with a modified version of the seL4 Core Platform.
In particular, the modified version of the seL4 Core Platform has support for the ELF Access Right Extension.


# The seL4 Core Platform (seL4CP)
The seL4CP has been modified to support dynamic loading of ELF files in empty protection domains.
See the `poc` branch at https://github.com/TobiasNissen/sel4cp for the source code of the modified seL4CP SDK.

For simplicity, a build of the modified SDK has been included directly in this repository.
The modified seL4CP SDK can be built and copied into this project with the utilities in the `sdk_utils` directory.
However, note that the paths defined in the variables of the scripts in this directory most likely have to be changed.


# System Overview
The file `configuration.system` describes the initial system configuration, which contains the protection domains `root_domain` and `pong`.

This system can be compiled by running `make` in the root of this repository.
The system can then be started by running `make run` in the root of this repository.
When running the system, a character device on the Linux host should automatically be assigned to the seL4CP system.
 
After system initialization, the protection domain `root_domain` expects to receive an ELF program via the assigned character device.

All system output can be read through the assigned character device.


# Dynamically Loading `child.elf`
When the system was compiled by running `make`, the program `child.c` was compiled into `build/child.elf`.
This program can now be dynamically loaded.

Before dynamically loading `child.elf`, the ELF program must be patched with an access right table.
In particular, the file `dynamic_programs/child_access_rights.xml` contains an XML description of the access rights required by `child.elf` to work. For instance, `child.elf` must have a channel to the `pong` protection domain, and it must get the IRQ access right to handle input from the character device from `root_domain`.

To patch `child.elf` with an access right table according to `dynamic_programs/child_access_rights.xml`, the following command can be run from the root of this directory:
```
sh ./dynamic_programs/prepare_program.sh ./configuration.system child.elf child_access_rights.xml
```

The `child` program can now be dynamically loaded by running:
```
sh ./dynamic_programs/load_program.sh ./dynamic_programs/child.elf <char_device>
```
Here, `<char_device>` is the character device that was assigned to the system when running `make run` earlier.
Note that loading the program might take some time.

When the program has been loaded, output similar to the following should be seen:
```
root: successfully started the program in a new child PD
child: initialized!
child: sending ping!
pong: received message on channel 0x0000000000000001
pong: ponging the same channel
child: received pong!
child: ready to receive ELF file to load dynamically!
```

# Dynamically Loading `memory_reader.elf`
As shown above, the `child` protection domain is now ready to dynamically load an ELF program.

When the system was compiled by running `make`, the program `memory_reader.c` was compiled into `build/memory_reader.elf`.
This program can now be dynamically loaded.

Similar to `child.elf`, `memory_reader.elf` must be patched with an access right table before it is loaded.
This can be done by running the following command:
```
sh ./dynamic_programs/prepare_program.sh ./dynamic_programs/configuration_with_child.system memory_reader.elf memory_reader_access_rights.xml
```
Observe that an updated system configuration file is provided, which takes the dynamically loaded `child` protection domain into account.

The ELF program `dynamic_programs/memory_reader.elf`, which has been patched with an access right table, can now be dynamically loaded by running:
```
sh ./dynamic_programs/load_program.sh ./dynamic_programs/memory_reader.elf <char_device>
```
Once again, `<char_device>` is the character device that was assigned to the system when running `make run` earlier.

When the program has been loaded, output similar to the following should be seen:
```
child: successfully started the program in a new child PD
memory_reader: initialized!
memory_reader: reading value (expecting 0x2a): 0x000000000000002a
```


# Alternative Access Rights
Instead of patching the dynamically loaded programs `child.elf` and `memory_reader.elf` with the access rights in `dynamic_programs/child_access_rights.xml` and `dynamic_programs/memory_reader_access_rights.xml`, respectively, other access right configurations can be tried. The purpose of this is to highlight that a protection domain is not able to perform an action that it does not have the required access rights to perform.

For instance, the ELF program `dynamic_programs/child_without_channel.elf` has been patched based on `child_access_rights_without_channel.xml`, which does not specify that `child` requires a channel to the `pong` protection domain.
Thus, if this ELF program is loaded instead of `dynamic_programs/child.elf`, output similar to the following should be seen:
```
root: successfully started the program in a new child PD
child: initialized!
child: sending ping!
<<seL4(CPU 0) [decodeInvocation/637 T0xffffff80402ba400 "child of: 'rootserver'" @2008fc]: Attempted to invoke a null cap #11.>>
```
As shown above, `child` is not able to ping the `pong` protection domain since it is missing the required access right.

As another example, the ELF program `dynamic_programs/child_without_memory_region.elf` has been patched based on `child_access_rights_without_memory_region.xml`, which does not specify that `child` requires access to the shared memory region `test_region`. Access to this memory region is required by `memory_reader`, but not directly by `child`. Thus, no error should occur when dynamically loading `child_without_memory_region.elf`. However, when trying to dynamically load `memory_reader.elf` afterwards, output similar to the following should be seen:
```
<<seL4(CPU 0) [decodeInvocation/637 T0xffffff80402ba400 "child of: 'rootserver'" @2016e0]: Attempted to invoke a null cap #758.>>
sel4cp_internal_set_up_access_rights: failed to map page for child
0x0000000000000002
sel4cp_internal_pd_load_elf: failed to set up access rights
child: failed to create a new PD with id 0x0000000000000005 and load the provided ELF file
```
As shown above, `child` is not able to provide access to `test_region` when trying to dynamically load `memory_reader.elf`.



    
