#include "elf_loader.h"

void load_elf(uint8_t *src, uint8_t *dst, uint64_t dst_vaddr_offset) {
    elf_header *elf_hdr = (elf_header *)src;
    
    for (uint64_t i = 0; i < elf_hdr->e_phnum; i++) {
        elf_program_header *p_hdr = (elf_program_header *)(src + elf_hdr->e_phoff + (i * elf_hdr->e_phentsize));
        if (p_hdr->p_type != PT_LOAD)
            continue; // the segment should not be loaded.
            
        uint8_t *src_read = src + p_hdr->p_offset;
        // TODO: ensure that p_vaddr is not smaller than dst_vaddr_offset.
        uint8_t *dst_write = dst + (p_hdr->p_vaddr - dst_vaddr_offset);
        // Copy the segment bytes from the ELF file.
        for (uint64_t j = 0; j < p_hdr->p_filesz; j++) {
            *dst_write++ = *src_read++;
        }
        // Write the required 0-initialized bytes, if needed.
        if (p_hdr->p_memsz > p_hdr->p_filesz) {
            uint64_t num_zero_bytes = p_hdr->p_memsz - p_hdr->p_filesz;
            for (uint64_t j = 0; j < num_zero_bytes; j++) {
                *dst_write++ = 0;
            }
        }
    }
}
