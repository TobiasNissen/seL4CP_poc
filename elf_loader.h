#ifndef SEL4CP_POC_ELF_LOADER_H
#define SEL4CP_POC_ELF_LOADER_H

#include <stdint.h>
#include <sel4cp.h>

/**
 *  Loads the loadable segments of the ELF file at the given src into the given PD.
 */
int elf_loader_load_segments(uint8_t *src, sel4cp_pd pd);

/**
 *  Sets up the the given child PD with the capabilities that
 *  are specified in the given ELF file.
 */
int elf_loader_setup_capabilities(uint8_t *elf_file, sel4cp_pd pd);

#endif // SEL4CP_POC_ELF_LOADER_H


