#ifndef SEL4CP_POC_ELF_LOADER_H
#define SEL4CP_POC_ELF_LOADER_H

#include <stdint.h>
#include <sel4cp.h>

/**
 *  Loads the loadable segments of the ELF file at the given src
 *  into the given dst, assuming that dst points to the 
 *  virtual address dst_vaddr_offset in the target PD.
 */
void elf_loader_load_segments(uint8_t *src, uint8_t *dst, uint64_t dst_vaddr_offset);

/**
 *  Sets up the the given child PD with the capabilities that
 *  are specified in the given ELF file, which is assumed to
 *  be elf_file_length bytes long.
 */
int elf_loader_setup_capabilities(uint8_t *elf_file, uint64_t elf_file_length, sel4cp_pd pd);

#endif // SEL4CP_POC_ELF_LOADER_H


