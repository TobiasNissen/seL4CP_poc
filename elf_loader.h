#ifndef SEL4CP_POC_ELF_LOADER_H
#define SEL4CP_POC_ELF_LOADER_H

#include <stdint.h>
#include <sel4cp.h>

/**
 *  Loads the loadable segments of the ELF file at the given src into the given PD.
 *  Sets up the PD according to the capabilities included in the given ELF file.
 *  
 *  NB: The program is NOT started. Use sel4cp_pd_restart to actually start the program.
 *  
 *  Returns 0 on success. In this case, the given entry_point points to the entry
 *  point of the loaded program.
 *  
 *  Returns -1 if an error occurs.
 */
int elf_loader_load(uint8_t *src, sel4cp_pd pd, uint64_t *entry_point);

/**
 *  Loads and runs the given program in the given PD.
 *  Precondition: The given src is assumed to point to an extended ELF file with capabilities.   
 *
 *  Returns 0 on success.
 *  Returns -1 if an error occurs.
 */
int elf_loader_run(uint8_t *src, sel4cp_pd pd);


#endif // SEL4CP_POC_ELF_LOADER_H


