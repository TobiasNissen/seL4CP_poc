#include "elf_loader.h"
#include <stddef.h>

#define EI_NIDENT 16 // the total number of bytes in the e_ident field of an ELF header.
#define EI_CAPABILITY_OFFSET_IDX 9 // the index into `e_ident` in the ELF header where the offset of the capability section is written.
#define EI_CAPABILITY_OFFSET_LEN 7 // the number of bytes used for writing the offset of the capability section.

#define PT_LOAD 1 // the identifier for a loadable ELF segment.

#define P_FLAGS_EXECUTABLE 1
#define P_FLAGS_WRITABLE 2
#define P_FLAGS_READABLE 4

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

#define LOADER_TEMP_PAGE_CAP 8
#define BASE_CNODE_CAP 394
#define BASE_VSPACE_CAP 458
#define BASE_PAGING_STRUCTURE_POOL 522
#define BASE_SHARED_MEMORY_REGION_PAGES (BASE_PAGING_STRUCTURE_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES + POOL_NUM_PAGE_DIRECTORIES + POOL_NUM_PAGE_TABLES + POOL_NUM_PAGES)

#define PD_CAP_BITS 11

#define SEL4_ARM_PAGE_CACHEABLE 1
#define SEL4_ARM_PARITY_ENABLED 2
#define SEL4_ARM_EXECUTE_NEVER 4

#define SEL4_ARM_DEFAULT_VMATTRIBUTES 3

uint8_t *__loader_temp_page_vaddr; // set by the build tool.
sel4cp_pd current_pd_id; // set by the build tool.

typedef struct {
    uint64_t page_upper_directory_idx;
    uint64_t page_directory_idx;
    uint64_t page_table_idx;
    uint64_t page_idx;
} allocation_state;

static allocation_state alloc_state = { 
    .page_upper_directory_idx = 0,
    .page_directory_idx = 0,
    .page_table_idx = 0 
};

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
 *  Masks out the lower num_bits bits of n.
 *  I.e. the lower num_bits bits of n are set to 0.
 */
static uint64_t mask_bits(uint64_t n, uint8_t num_bits) {
    return (n >> num_bits) << num_bits;
}

/**
 *  Parses the given memory_flags into an appropriate seL4_CapRights_t object.
 *  The bits (0-7) in the memory_flags are given meaning in the following way:
 *      1: write
 *      2: read
 *  All bits not mentioned above are ignored and, thus, not given any meaning.
 */
static seL4_CapRights_t parse_cap_rights(uint8_t memory_flags) {
    if (memory_flags & P_FLAGS_WRITABLE) {
        return seL4_ReadWrite;
    }
    
    return seL4_CanRead;
}

/**
 *  Parses the information in memory_flags and cached into an seL4_ARM_VMAttributes object.
 *  If the first bit (index 0) in memory_flags is set, the VM attributes will indicate
 *  that the targeted page should be executable.
 */
static seL4_ARM_VMAttributes parse_vm_attributes(uint8_t memory_flags, bool cached) {
    seL4_ARM_VMAttributes result = SEL4_ARM_PARITY_ENABLED;
    if (cached) {
        result |= SEL4_ARM_PAGE_CACHEABLE;
    }
    if (!(memory_flags & P_FLAGS_EXECUTABLE)) {
        result |= SEL4_ARM_EXECUTE_NEVER;
    }
    return result;
}

/**
 *  Ensures that all higher-level paging structures in the ARM AArch64 four-level
 *  page-table structure required to map a page at the given virtual address in the given
 *  PD are mapped.
 *
 *  The bits in a virtual address are given the following meaning:
 *      -  0-11: offset into a page.
 *      - 12-20: offset into a page table, selecting a specific page.
 *      - 21-29: offset into a page directory, selecting a specific page table.
 *      - 30-38: offset into a page upper directory, selecting a specific page directory.
 *      - 39-47: offset into a page global directory, selecting a specific page upper directory. 
 *  Note that the VSpace is a page global directory in seL4 for ARM AArch64.
 */
static int set_up_required_paging_structures(uint64_t vaddr, sel4cp_pd pd) {
    uint64_t pd_vspace_cap = BASE_VSPACE_CAP + pd;
    
    // Ensure that the required page upper directory is mapped.
    uint64_t page_upper_directory_vaddr = mask_bits(vaddr, 12 + 9 + 9 + 9);
    if (alloc_state.page_upper_directory_idx >= POOL_NUM_PAGE_UPPER_DIRECTORIES) {
        sel4cp_dbg_puts("elf_loader: no page upper directories are available; allocate more and try again\n");
        return -1;
    }
    seL4_Error err = seL4_ARM_PageUpperDirectory_Map(
        BASE_PAGING_STRUCTURE_POOL + alloc_state.page_upper_directory_idx,
        pd_vspace_cap,
        page_upper_directory_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        alloc_state.page_upper_directory_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page upper directory has already been mapped.
        sel4cp_dbg_puts("elf_loader: failed to allocate a required page upper directory; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    // Ensure that the required page directory is mapped.
    uint64_t page_directory_vaddr = mask_bits(vaddr, 12 + 9 + 9);
    if (alloc_state.page_directory_idx >= POOL_NUM_PAGE_DIRECTORIES) {
        sel4cp_dbg_puts("elf_loader: no page directories are available; allocate more and try again\n");
        return -1;
    }
    err = seL4_ARM_PageDirectory_Map(
        BASE_PAGING_STRUCTURE_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES + alloc_state.page_directory_idx,
        pd_vspace_cap,
        page_directory_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        alloc_state.page_directory_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page directory has already been mapped.
        sel4cp_dbg_puts("elf_loader: failed to allocate a required page directory; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    // Ensure that the required page table is mapped.
    uint64_t page_table_vaddr = mask_bits(vaddr, 12 + 9);
    if (alloc_state.page_table_idx >= POOL_NUM_PAGE_TABLES) {
        sel4cp_dbg_puts("elf_loader: no page tables are available; allocate more and try again\n");
        return -1;
    }
    err = seL4_ARM_PageTable_Map(
        BASE_PAGING_STRUCTURE_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES + POOL_NUM_PAGE_DIRECTORIES + alloc_state.page_table_idx,
        pd_vspace_cap,
        page_table_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        alloc_state.page_table_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page table has already been mapped.
        sel4cp_dbg_puts("elf_loader: failed to allocate a required page table; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    return 0;
}

/**
 *  Returns a virtual address in the current PD which
 *  can be used to write data that will be available at the
 *  given vaddr in the given pd.
 *  The required paging structures are automatically allocated,
 *  and the page is mapped with the given ELF program header p_flags.
 *
 *  Returns NULL if the allocation fails. 
 *  Nothing is done to clean up in this case.
 */
static uint8_t *allocate_page(uint64_t vaddr, sel4cp_pd pd, uint32_t p_flags) {
    if (set_up_required_paging_structures(vaddr, pd)) {
        return NULL;
    }
    
    // Extract the rights and VM attributes to map the required page with 
    // from the given ELF program header flags.
    seL4_CapRights_t rights = parse_cap_rights((uint8_t)p_flags);
    seL4_ARM_VMAttributes vm_attributes = parse_vm_attributes((uint8_t)p_flags, true);
    
    // Allocate and map the required page.
    uint64_t page_vaddr = mask_bits(vaddr, 12);
    if (alloc_state.page_idx >= POOL_NUM_PAGES) {
        sel4cp_dbg_puts("elf_loader: no pages are available; allocate more and try again\n");
        return NULL;
    }
    seL4_Error err = seL4_ARM_Page_Map(
        BASE_PAGING_STRUCTURE_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES + POOL_NUM_PAGE_DIRECTORIES + POOL_NUM_PAGE_TABLES + alloc_state.page_idx,
        BASE_VSPACE_CAP + pd,
        page_vaddr,
        rights,
        vm_attributes
    );
    if (err == seL4_NoError) {
        alloc_state.page_idx++;
    }
    else {
        sel4cp_dbg_puts("elf_loader: failed to allocate a required page; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return NULL;
    }
    
    // TODO: Implement a more clever way of doing this, ensuring that this is always only done once.
    //       Potentially just be setting a boolean variable.
    if (set_up_required_paging_structures((uint64_t)__loader_temp_page_vaddr, current_pd_id)) {
        return NULL;
    }
    
    // Ensure that the capability slot for the page capability mapped into the current PD's VSpace is empty.
    err = seL4_CNode_Delete(
        BASE_CNODE_CAP + current_pd_id,
        LOADER_TEMP_PAGE_CAP,
        PD_CAP_BITS
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("elf_loader: failed to clean up the CSlot containing the temporary page cap used for loading ELF files, error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return NULL;
    }
    
    // Copy the capability for the allocated page to the temporary page cap CSlot.
    err = seL4_CNode_Copy(
        BASE_CNODE_CAP + current_pd_id,
        LOADER_TEMP_PAGE_CAP,
        PD_CAP_BITS,
        BASE_CNODE_CAP + current_pd_id,
        BASE_PAGING_STRUCTURE_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES + POOL_NUM_PAGE_DIRECTORIES + POOL_NUM_PAGE_TABLES + alloc_state.page_idx - 1,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("elf_loader: failed to copy page capability required to be able to load ELF file, error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return NULL;
    }
    
    // Map the copied page capability into the VSpace of the current PD.
    err = seL4_ARM_Page_Map(
        LOADER_TEMP_PAGE_CAP,
        BASE_VSPACE_CAP + current_pd_id,
        (uint64_t)__loader_temp_page_vaddr,
        seL4_ReadWrite,
        SEL4_ARM_DEFAULT_VMATTRIBUTES
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("elf_loader: failed to map the page via the copied page capability into the current PD's VSpace, error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return NULL;
    }

    return __loader_temp_page_vaddr + ((uint64_t)(vaddr % 0x1000));
}

/**
 *  Sets up the capabilities for the given program in the given PD.
 */
static int set_up_capabilities(uint8_t *elf_file, sel4cp_pd pd) {
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
                
                // Parse the rights and VM attributes.
                seL4_CapRights_t rights = parse_cap_rights(perms);
                seL4_ARM_VMAttributes vm_attributes = parse_vm_attributes(perms, cached); 
                
                // Map the page into the child PD's VSpace.
                uint64_t pd_vspace_cap = BASE_VSPACE_CAP + pd;
                uint64_t num_pages = size / 0x1000; // Assumes that the size is a multiple of the page size 0x1000.
                for (uint64_t j = 0; j < num_pages; j++) {
                    uint64_t page_cap = BASE_SHARED_MEMORY_REGION_PAGES + id + j;
                    uint64_t page_vaddr = vaddr + (j * 0x1000);
                    
                    // Ensure that all required higher-level paging structures are mapped before
                    // mapping this page.
                    if (set_up_required_paging_structures(page_vaddr, pd)) {
                        return -1;
                    }
                    
                    seL4_Error err = seL4_ARM_Page_Map(
                        page_cap, 
                        pd_vspace_cap, 
                        page_vaddr, 
                        rights, 
                        vm_attributes
                    );
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


int elf_loader_load(uint8_t *src, sel4cp_pd pd, uint64_t *entry_point) {
    elf_header *elf_hdr = (elf_header *)src;
    
    // Set the entry point of the given program.
    *entry_point = elf_hdr->e_entry;
    
    for (uint64_t i = 0; i < elf_hdr->e_phnum; i++) {
        elf_program_header *prog_hdr = (elf_program_header *)(src + elf_hdr->e_phoff + (i * elf_hdr->e_phentsize));
        if (prog_hdr->p_type != PT_LOAD)
            continue; // the segment should not be loaded.
        
        uint8_t *dst_write = allocate_page(prog_hdr->p_vaddr, pd, prog_hdr->p_flags);
        if (dst_write == NULL) {
            sel4cp_dbg_puts("elf_loader: failed to allocate a page required to load the ELF file\n");
            return -1;
        }
        
        uint8_t *src_read = src + prog_hdr->p_offset;
        uint64_t current_vaddr = prog_hdr->p_vaddr;
        
        // Copy the segment bytes from the ELF file.
        for (uint64_t j = 0; j < prog_hdr->p_filesz; j++) {
            *dst_write++ = *src_read++;
            
            current_vaddr++;
            if (current_vaddr % 0x1000 == 0) { // assuming a page size of 0x1000 bytes (4 KiB).
                // Allocate a new page.
                dst_write = allocate_page(current_vaddr, pd, prog_hdr->p_flags);
                if (dst_write == NULL) {
                    sel4cp_dbg_puts("elf_loader: failed to allocate a page required to load the ELF file\n");
                    return -1;
                }
            }
        }
        // Write the required 0-initialized bytes, if needed.
        if (prog_hdr->p_memsz > prog_hdr->p_filesz) {
            uint64_t num_zero_bytes = prog_hdr->p_memsz - prog_hdr->p_filesz;
            for (uint64_t j = 0; j < num_zero_bytes; j++) {
                *dst_write++ = 0;
                
                current_vaddr++;
                if (current_vaddr % 0x1000 == 0) { // assuming a page size of 0x1000 bytes (4 KiB).
                    // Allocate a new page.
                    dst_write = allocate_page(current_vaddr, pd, prog_hdr->p_flags);
                    if (dst_write == NULL) {
                        sel4cp_dbg_puts("elf_loader: failed to allocate a page required to load the ELF file\n");
                        return -1;
                    }
                }
            }
        }
    }
    
    return set_up_capabilities(src, pd);
}

int elf_loader_run(uint8_t *src, sel4cp_pd pd) {
    uint64_t entry_point;
    int result = elf_loader_load(src, pd, &entry_point);
    if (result)
        return result;
    
    sel4cp_pd_restart(pd, entry_point);
    
    return 0;
}








