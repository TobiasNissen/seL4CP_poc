import os
import struct
from pathlib import Path
from access_rights import AccessRight, SchedulingAccessRight, ChannelAccessRight, MemoryRegionAccessRight, IrqAccessRight

EI_ACCESS_RIGHTS_OFFSET_IDX = 9

def patch_elf(target_elf: Path, access_rights: list[AccessRight]): # type: ignored
    with open(target_elf, "rb+") as elf:
        # Read the offset of the access rights in the given ELF file,
        # taking into account that the offset is only 7 bytes long.
        elf.seek(EI_ACCESS_RIGHTS_OFFSET_IDX - 1)
        access_rights_offset_str = elf.read(8)
        (access_rights_offset, ) = struct.unpack("<Q" , access_rights_offset_str)
        access_rights_offset >>= 8
        
        # Seek to the access rights section.
        if access_rights_offset != 0:
            elf.seek(access_rights_offset)
        else:
            elf.seek(0, os.SEEK_END)
            # Write the offset of the access rights section to the ELF file.
            elf_size = elf.tell()
            elf.seek(EI_ACCESS_RIGHTS_OFFSET_IDX)
            elf.write(struct.pack("<Q", elf_size)[0:7])
            
            elf.seek(0, os.SEEK_END)
        
        # Write the total number of access rights
        elf.write(struct.pack("<Q", len(access_rights)))
        
        # Write the serialized access rights.
        for access_right in access_rights:
            elf.write(access_right.serialize())
        
        # Ensure that any old access rights are deleted.
        elf.truncate()
           
        





