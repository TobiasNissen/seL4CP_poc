#include "elf_loader.h"

#define EI_NIDENT 16 // the total number of bytes in the e_ident field of an ELF header.
#define EI_CAPABILITY_OFFSET_IDX 9 // the index into `e_ident` in the ELF header where the offset of the capability section is written.
#define EI_CAPABILITY_OFFSET_LEN 7 // the number of bytes used for writing the offset of the capability section.

#define PT_LOAD 1 // the identifier for a loadable ELF segment.

#define PRIORITY_ID 0
#define BUDGET_ID 1
#define PERIOD_ID 2
#define CHANNEL_ID 3

#define DEFAULT_BUDGET 1000
#define DEFAULT_PERIOD DEFAULT_BUDGET

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


void elf_loader_load_segments(uint8_t *src, uint8_t *dst, uint64_t dst_vaddr_offset) {
    elf_header *elf_hdr = (elf_header *)src;
    
    for (uint64_t i = 0; i < elf_hdr->e_phnum; i++) {
        elf_program_header *prog_hdr = (elf_program_header *)(src + elf_hdr->e_phoff + (i * elf_hdr->e_phentsize));
        if (prog_hdr->p_type != PT_LOAD)
            continue; // the segment should not be loaded.
            
        uint8_t *src_read = src + prog_hdr->p_offset;
        // TODO: ensure that p_vaddr is not smaller than dst_vaddr_offset.
        uint8_t *dst_write = dst + (prog_hdr->p_vaddr - dst_vaddr_offset);
        // Copy the segment bytes from the ELF file.
        for (uint64_t j = 0; j < prog_hdr->p_filesz; j++) {
            *dst_write++ = *src_read++;
        }
        // Write the required 0-initialized bytes, if needed.
        if (prog_hdr->p_memsz > prog_hdr->p_filesz) {
            uint64_t num_zero_bytes = prog_hdr->p_memsz - prog_hdr->p_filesz;
            for (uint64_t j = 0; j < num_zero_bytes; j++) {
                *dst_write++ = 0;
            }
        }
    }
}

int elf_loader_setup_capabilities(uint8_t *elf_file, uint64_t elf_file_length, sel4cp_pd pd) {
    sel4cp_dbg_puts("elf_loader: setting up capabilities!\n");
    
    // Get the offset of the capability section, 
    // taking into account that the offset is only 7 bytes long.
    uint64_t capability_offset = *((uint64_t *)(elf_file + EI_CAPABILITY_OFFSET_IDX - 1)) >> 8;
    
    uint8_t *cap_reader = elf_file + capability_offset;
    
    uint64_t num_capabilities = *((uint64_t *) cap_reader);
    cap_reader += 8;
    
    uint64_t budget = DEFAULT_BUDGET;
    uint64_t period = DEFAULT_PERIOD;
    bool period_set_explicitly = false;
    
    // Setup all capabilities.
    for (uint64_t i = 0; i < num_capabilities; i++) {
        uint8_t cap_type_id = *cap_reader++;
        // TODO: Handle more capability types.
        switch (cap_type_id) {
            case PRIORITY_ID:
                uint8_t priority = *cap_reader++;
                sel4cp_pd_set_priority(pd, priority);
                sel4cp_dbg_puts("elf_loader: set priority ");
                sel4cp_dbg_puthex64(priority);
                sel4cp_dbg_puts("\n");
                break;
            case BUDGET_ID:
                budget = *((uint64_t *)cap_reader);
                cap_reader += 8;
                break;
            case PERIOD_ID:
                period = *((uint64_t *)cap_reader);
                period_set_explicitly = true;
                cap_reader += 8;
                break;
            case CHANNEL_ID:
                uint8_t target_pd = *cap_reader++;
                uint8_t target_id = *cap_reader++;
                uint8_t own_id = *cap_reader++;
                
                sel4cp_dbg_puts("elf_loader: set up channel - pd_a = ");
                sel4cp_dbg_puthex64(pd);
                sel4cp_dbg_puts(", pd_b = ");
                sel4cp_dbg_puthex64(target_pd);
                sel4cp_dbg_puts(", channel_id_a = ");
                sel4cp_dbg_puthex64(own_id);
                sel4cp_dbg_puts(", channel_id_b = ");
                sel4cp_dbg_puthex64(target_id);
                sel4cp_dbg_puts("\n");
                
                sel4cp_set_up_channel(pd, target_pd, own_id, target_id);
                break;
            default:
                sel4cp_dbg_puts("elf_loader: invalid capability type id: ");
                sel4cp_dbg_puthex64(cap_type_id);
                sel4cp_dbg_puts("\n");
                return -1;
        }
    }
    
    // Set the scheduling flags if they have been explicitly provided.
    if (budget != DEFAULT_BUDGET || period != DEFAULT_PERIOD) {
        if (!period_set_explicitly)
            period = budget; // By default, the period is the same as the budget.
        sel4cp_dbg_puts("elf_loader: set scheduling flags: budget = ");
        sel4cp_dbg_puthex64(budget);
        sel4cp_dbg_puts(" , period = ");
        sel4cp_dbg_puthex64(period);
        sel4cp_dbg_puts("\n");
        sel4cp_pd_set_sched_flags(pd, budget, period);
    }
    
    
    return 0;
}



