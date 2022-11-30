#include "elf_loader.h"

#define EI_NIDENT 16 // the total number of bytes in the e_ident field of an ELF header.
#define EI_CAPABILITY_OFFSET_IDX 9 // the index into `e_ident` in the ELF header where the offset of the capability section is written.
#define EI_CAPABILITY_OFFSET_LEN 7 // the number of bytes used for writing the offset of the capability section.

#define PT_LOAD 1 // the identifier for a loadable ELF segment.

#define PRIORITY_ID 0
#define BUDGET_ID 1
#define PERIOD_ID 2
#define CHANNEL_ID 3
#define MEMORY_REGION_ID 4

#define DEFAULT_BUDGET 1000
#define DEFAULT_PERIOD DEFAULT_BUDGET

// TODO: Ensure that these are defined based on the system configuration.
#define POOL_NUM_PAGE_UPPER_DIRECTORIES 5
#define POOL_NUM_PAGE_DIRECTORIES 5
#define POOL_NUM_PAGE_TABLES 10
#define POOL_NUM_PAGES 100

#define BASE_VSPACE_CAP 458
#define BASE_PAGING_STRUCTURE_POOL 522
#define BASE_SHARED_MEMORY_REGION_PAGES (BASE_PAGING_STRUCTURE_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES + POOL_NUM_PAGE_DIRECTORIES + POOL_NUM_PAGE_TABLES + POOL_NUM_PAGES)

#define SEL4_ARM_DEFAULT_VMATTRIBUTES 3

typedef struct {
    uint64_t page_upper_directory_idx;
    uint64_t page_directory_idx;
    uint64_t page_table_idx;
    uint64_t page_idx;
} allocation_state;

allocation_state new_allocation_state() {
    allocation_state state;
    state.page_upper_directory_idx = 0;
    state.page_directory_idx = 0;
    state.page_table_idx = 0;
    state.page_idx = 0;
    return state;
}

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

/**
 *  Masks out the lower num_bits bits of n.
 *  I.e. the lower num_bits bits of n are set to 0.
 */
static uint64_t mask_bits(uint64_t n, uint8_t num_bits) {
    return (n >> num_bits) << num_bits;
} 

/**
 *  ARM AArch64 processors have a four-level page-table structure.
 *  The first 12 bits in a virtual address gives the offset into a page.
 *  The next 9 bits in a virtual address are used to select a page in a page table.
 *  The next 9 bits in a virtual address are used to select a page table in a page directory.
 *  The next 9 bits in a virtual address are used to select a page directory in a page upper directory.
 *  The next 9 bits in a virtual address are used to select a page upper directory in a page global directory.
 *  Note that the VSpace is a page global directory in seL4 for ARM AArch64.
 */
static int set_up_required_paging_structures(allocation_state *allocation_state, uint64_t vaddr, sel4cp_pd pd) {
    uint64_t pd_vspace_cap = BASE_VSPACE_CAP + pd;
    
    // Ensure that the required page upper directory is mapped.
    uint64_t page_upper_directory_vaddr = mask_bits(vaddr, 12 + 9 + 9 + 9);
    if (allocation_state->page_upper_directory_idx >= POOL_NUM_PAGE_UPPER_DIRECTORIES) {
        sel4cp_dbg_puts("No page upper directories are available; allocate more and try again\n");
        return -1;
    }
    seL4_Error err = seL4_ARM_PageUpperDirectory_Map(
        BASE_PAGING_STRUCTURE_POOL + allocation_state->page_upper_directory_idx,
        pd_vspace_cap,
        page_upper_directory_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        allocation_state->page_upper_directory_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page upper directory has already been mapped.
        sel4cp_dbg_puts("Failed to allocate a required page upper directory; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    // Ensure that the required page directory is mapped.
    uint64_t page_directory_vaddr = mask_bits(vaddr, 12 + 9 + 9);
    if (allocation_state->page_directory_idx >= POOL_NUM_PAGE_DIRECTORIES) {
        sel4cp_dbg_puts("No page directories are available; allocate more and try again\n");
        return -1;
    }
    err = seL4_ARM_PageDirectory_Map(
        BASE_PAGING_STRUCTURE_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES + allocation_state->page_directory_idx,
        pd_vspace_cap,
        page_directory_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        allocation_state->page_directory_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page directory has already been mapped.
        sel4cp_dbg_puts("Failed to allocate a required page directory; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    // Ensure that the required page table is mapped.
    uint64_t page_table_vaddr = mask_bits(vaddr, 12 + 9);
    if (allocation_state->page_table_idx >= POOL_NUM_PAGE_TABLES) {
        sel4cp_dbg_puts("No page tables are available; allocate more and try again\n");
        return -1;
    }
    err = seL4_ARM_PageTable_Map(
        BASE_PAGING_STRUCTURE_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES + POOL_NUM_PAGE_DIRECTORIES + allocation_state->page_table_idx,
        pd_vspace_cap,
        page_table_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        allocation_state->page_table_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page table has already been mapped.
        sel4cp_dbg_puts("Failed to allocate a required page table; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    return 0;
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
    
    // For now, it is assumed that all paging structures in the statically allocated pool are available.
    // This assumption most likely does not hold if multiple programs need to be loaded from the same PD.
    allocation_state allocation_state = new_allocation_state(); 
    
    // Setup all capabilities.
    for (uint64_t i = 0; i < num_capabilities; i++) {
        uint8_t cap_type_id = *cap_reader++;
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
                
                sel4cp_set_up_channel(pd, target_pd, own_id, target_id);
                
                sel4cp_dbg_puts("elf_loader: set up channel - pd_a = ");
                sel4cp_dbg_puthex64(pd);
                sel4cp_dbg_puts(", pd_b = ");
                sel4cp_dbg_puthex64(target_pd);
                sel4cp_dbg_puts(", channel_id_a = ");
                sel4cp_dbg_puthex64(own_id);
                sel4cp_dbg_puts(", channel_id_b = ");
                sel4cp_dbg_puthex64(target_id);
                sel4cp_dbg_puts("\n");
                break;
            case MEMORY_REGION_ID:
                uint64_t id = *((uint64_t *) cap_reader);
                cap_reader += 8;
                uint64_t vaddr = *((uint64_t *) cap_reader);
                cap_reader += 8;
                uint64_t size = *((uint64_t *) cap_reader);
                cap_reader += 8;
                uint8_t perms = *cap_reader++;
                uint8_t cached = *cap_reader++;
                
                // Setup the capability rights.
                seL4_CapRights_t rights = seL4_NoRights;
                if (perms >= 3)
                    rights = seL4_ReadWrite;
                else if (perms == 2) {
                    rights = seL4_CanWrite;
                }
                else if (perms == 0) {
                    rights = seL4_CanRead;
                }
                
                // Setup the VM attributes.
                uint8_t vm_attributes = 2; // Ensures that parity is enabled.
                if (cached == 1) {
                    vm_attributes |= 1;
                }
                if (perms < 4) {
                    vm_attributes |= 4;
                }
                
                // Map the page into the child PD's VSpace.
                uint64_t pd_vspace_cap = BASE_VSPACE_CAP + pd;
                uint64_t num_pages = size / 0x1000; // Assumes that the size is a multiple of the page size 0x1000.
                for (uint64_t j = 0; j < num_pages; j++) {
                    uint64_t page_cap = BASE_SHARED_MEMORY_REGION_PAGES + id + j;
                    uint64_t page_vaddr = vaddr + (j * 0x1000);
                    
                    // Ensure that all required higher-level paging structures are mapped before
                    // mapping this page.
                    if (set_up_required_paging_structures(&allocation_state, page_vaddr, pd)) {
                        return -1;
                    }
                    
                    seL4_Error err = seL4_ARM_Page_Map(page_cap, pd_vspace_cap, page_vaddr, rights, vm_attributes);
                    if (err != seL4_NoError) {
                        sel4cp_dbg_puts("elf_loader: failed to map page for child\n");
                        sel4cp_dbg_puthex64(err);
                        sel4cp_dbg_puts("\n");
                        return -1;
                    } 
                }                
                
                sel4cp_dbg_puts("elf_loader: set up memory region - id = ");
                sel4cp_dbg_puthex64(id);
                sel4cp_dbg_puts(", vaddr = ");
                sel4cp_dbg_puthex64(vaddr);
                sel4cp_dbg_puts(", size = ");
                sel4cp_dbg_puthex64(size);
                sel4cp_dbg_puts(", perms = ");
                sel4cp_dbg_puthex64(perms);
                sel4cp_dbg_puts(", cached = ");
                sel4cp_dbg_puthex64(cached);
                sel4cp_dbg_puts("\n");
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



