#ifndef SEL4CP_POC_ELF_LOADER_H
#define SEL4CP_POC_ELF_LOADER_H

#include <stdint.h>

#define EI_NIDENT 16 // the total number of bytes in the e_ident field of an ELF header.

#define PT_LOAD 1 // the identifier for a loadable segment.

typedef struct {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf_header;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf_program_header;


/**
 *  Loads the loadable segments of the ELF file at the given src
 *  into the given dst, assuming that dst points to the 
 *  virtual address dst_vaddr_offset in the target PD.
 */
void load_elf(uint8_t *src, uint8_t *dst, uint64_t dst_vaddr_offset);

#endif // SEL4CP_POC_ELF_LOADER_H




