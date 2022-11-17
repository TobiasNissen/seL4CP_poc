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
void load_elf(uint8_t *src, uint8_t *dst, uint64_t dst_vaddr_offset) {
    elf_header *elf_hdr = (elf_header *)src;
    
    for (uint64_t i = 0; i < elf_hdr->e_phnum; i++) {
        elf_program_header *p_hdr = (elf_program_header *)(src + elf_hdr->e_phoff + (i * elf_hdr->e_phentsize));
        if (p_hdr->p_type != PT_LOAD)
            continue; // the segment should not be loaded.
            
        uint8_t *src_read = src + p_hdr->p_offset;
        // TODO: ensure that p_vaddr is not smaller than dst_vaddr_offset.
        uint8_t *dst_write = dst + (p_hdr->p_vaddr - dst_vaddr_offset);
        // Copy the segments bytes from the ELF file.
        for (uint64_t j = 0; j < p_hdr->p_filesz; j++) {
            *dst_write = *src_read;
            src_read++;
            dst_write++;
        }
        // Write the required 0-initialized bytes, if needed.
        if (p_hdr->p_memsz > p_hdr->p_filesz) {
            uint64_t num_zero_bytes = p_hdr->p_memsz - p_hdr->p_filesz;
            for (uint64_t j = 0; j < num_zero_bytes; j++) {
                *dst_write = 0;
                dst_write++;
            }
        }
    }
}




