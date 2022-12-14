/*
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/* seL4 Core Platform interface */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define __thread
#include <sel4/sel4.h>

// ELF-related constants
#define EI_NIDENT 16 // the total number of bytes in the e_ident field of an ELF header.
#define EI_CAPABILITY_OFFSET_IDX 9 // the index into `e_ident` in the ELF header where the offset of the capability section is written.
#define PT_LOAD 1 // the identifier for a loadable ELF segment.
#define P_FLAGS_EXECUTABLE 1 // Bit indicating that a segment should be executable.
#define P_FLAGS_WRITABLE 2 // Bit indicating that a segment should be writable.
#define P_FLAGS_READABLE 4 // Bit indicating that a segment should be readable.
#define SHT_SYMTAB 2 // the identifier for a symbol table section.


// Constants related to the protection model.
#define SCHEDULING_ID 0
#define CHANNEL_ID 1
#define MEMORY_REGION_ID 2
#define IRQ_ID 3

// Constants related to the organization of the CSpace in a PD.
#define PD_CAP_BITS 11
#define POOL_NUM_PD_TARGETS 5
#define POOL_NUM_TCBS POOL_NUM_PD_TARGETS
#define POOL_NUM_NOTIFICATIONS POOL_NUM_PD_TARGETS
#define POOL_NUM_CNODES POOL_NUM_PD_TARGETS
#define POOL_NUM_SCHEDCONTEXTS POOL_NUM_PD_TARGETS
#define POOL_NUM_VSPACES POOL_NUM_PD_TARGETS
#define POOL_NUM_PAGE_UPPER_DIRECTORIES (POOL_NUM_PD_TARGETS * 2)
#define POOL_NUM_PAGE_DIRECTORIES (POOL_NUM_PD_TARGETS * 4)
#define POOL_NUM_PAGE_TABLES (POOL_NUM_PD_TARGETS * 6)
#define POOL_NUM_PAGES (POOL_NUM_PD_TARGETS * 20)

// Constants used for addressing specific capabilities in a PD.
#define INPUT_CAP_IDX 1
#define FAULT_EP_CAP_IDX 2
#define REPLY_CAP_IDX 4
#define ASID_POOL_CAP_IDX 5
#define SCHED_CONTROL_CAP_IDX 6
#define TEMP_CAP 8
#define BASE_OUTPUT_NOTIFICATION_CAP 10
#define BASE_OUTPUT_ENDPOINT_CAP (BASE_OUTPUT_NOTIFICATION_CAP + 64)
#define BASE_IRQ_CAP (BASE_OUTPUT_ENDPOINT_CAP + 64)
#define BASE_TCB_CAP (BASE_IRQ_CAP + 64) 
#define BASE_SCHED_CONTEXT_CAP (BASE_TCB_CAP + 64)
#define BASE_UNBADGED_CHANNEL_CAP (BASE_SCHED_CONTEXT_CAP + 64)
#define BASE_CNODE_CAP (BASE_UNBADGED_CHANNEL_CAP + 64)
#define BASE_VSPACE_CAP (BASE_CNODE_CAP + 64)
#define BASE_TCB_POOL (BASE_VSPACE_CAP + 64)
#define BASE_NOTIFICATION_POOL (BASE_TCB_POOL + POOL_NUM_TCBS)
#define BASE_CNODE_POOL (BASE_NOTIFICATION_POOL + POOL_NUM_NOTIFICATIONS)
#define BASE_SCHEDCONTEXT_POOL (BASE_CNODE_POOL + POOL_NUM_CNODES)
#define BASE_VSPACE_POOL (BASE_SCHEDCONTEXT_POOL + POOL_NUM_SCHEDCONTEXTS)
#define BASE_PAGE_UPPER_DIRECTORY_POOL (BASE_VSPACE_POOL + POOL_NUM_VSPACES)
#define BASE_PAGE_DIRECTORY_POOL (BASE_PAGE_UPPER_DIRECTORY_POOL + POOL_NUM_PAGE_UPPER_DIRECTORIES)
#define BASE_PAGE_TABLE_POOL (BASE_PAGE_DIRECTORY_POOL + POOL_NUM_PAGE_DIRECTORIES)
#define BASE_PAGE_POOL (BASE_PAGE_TABLE_POOL + POOL_NUM_PAGE_TABLES)
#define BASE_SHARED_MEMORY_REGION_PAGES (BASE_PAGE_POOL + POOL_NUM_PAGES)

// General settings.
#define SEL4CP_MAX_CHANNELS 63

// Constants related to paging on ARM
#define SEL4_ARM_PAGE_CACHEABLE 1
#define SEL4_ARM_PARITY_ENABLED 2
#define SEL4_ARM_EXECUTE_NEVER 4
#define SEL4_ARM_DEFAULT_VMATTRIBUTES 3 // By default, map pages as cacheable and with parity enabled.


typedef unsigned int sel4cp_channel;
typedef unsigned int sel4cp_pd;
typedef seL4_MessageInfo_t sel4cp_msginfo;
typedef seL4_Time sel4cp_time;
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
typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} elf_section_header;
typedef struct {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} elf_symbol_table_entry;
typedef struct {
    uint64_t tcb_idx;
    uint64_t notification_idx;    
    uint64_t cnode_idx;
    uint64_t schedcontext_idx;
    uint64_t vspace_idx;    
    uint64_t page_upper_directory_idx;
    uint64_t page_directory_idx;
    uint64_t page_table_idx;
    uint64_t page_idx;
    uint8_t *temp_page_vaddr;
} allocation_state;

static allocation_state alloc_state = { 
    .tcb_idx = 0,
    .notification_idx = 0,
    .cnode_idx = 0,
    .schedcontext_idx = 0,
    .vspace_idx = 0,
    .page_upper_directory_idx = 0,
    .page_directory_idx = 0,
    .page_table_idx = 0,
    .page_idx = 0,
    .temp_page_vaddr = NULL
};

/* User-provided functions */
void init(void);
void notified(sel4cp_channel ch);
sel4cp_msginfo protected(sel4cp_channel ch, sel4cp_msginfo msginfo);
void fault(sel4cp_channel ch, sel4cp_msginfo msginfo);

// The following variables are set by the build tool.
extern char sel4cp_name[16];
extern sel4cp_pd sel4cp_current_pd_id;



// ========== PRINTING UTILITIES ==========
/*
 * Output a single character on the debug console.
 */
void sel4cp_dbg_putc(int c);

/*
 * Output a NUL terminated string to the debug console.
 */
void sel4cp_dbg_puts(const char *s);

/*
 * Output the given integer as a 64 bit hexadecimal number.
 */
void sel4cp_dbg_puthex64(uint64_t val);

// ========== END OF PRINTING UTILITIES ==========

// ========== UTILITY FUNCTIONS ==========
static inline void
sel4cp_internal_crash(seL4_Error err)
{
    /*
     * Currently crash by dereferencing NULL page
     *
     * Actually derference 'err' which means the crash reporting will have
     * `err` as the fault address. A bit of a cute hack. Not a good long term
     * solution but good for now.
     */
    int *x = (int *)(uintptr_t) err;
    *x = 0;
}

/**
 *  Ensures that the CSlot used for temporary capabilities at index TEMP_CAP is empty.
 */
static void
sel4cp_internal_delete_temp_cap(void) {
    seL4_Error err = seL4_CNode_Delete(
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        TEMP_CAP,
        PD_CAP_BITS
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_delete_temp_cap: failed to clean up the CSlot used for temporary capabilities, error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        
        sel4cp_internal_crash(err);
    }
}

static void
sel4cp_internal_set_priority(sel4cp_pd pd, uint8_t priority, uint8_t mcp)
{
    seL4_Error err = seL4_TCB_SetSchedParams(
        BASE_TCB_CAP + pd, 
        BASE_TCB_CAP + sel4cp_current_pd_id, 
        mcp, 
        priority,
        BASE_SCHEDCONTEXT_POOL + alloc_state.schedcontext_idx - 1,
        FAULT_EP_CAP_IDX
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_set_priority: error setting priority\n");
        sel4cp_internal_crash(err);
    }
}

static void
sel4cp_internal_set_sched_flags(sel4cp_pd pd, sel4cp_time budget, sel4cp_time period)
{
    seL4_Error err = seL4_SchedControl_ConfigureFlags(SCHED_CONTROL_CAP_IDX, BASE_SCHED_CONTEXT_CAP + pd,
                                           budget, period, 0, 0, 0);
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_set_sched_flags: error setting scheduling flags\n");
        sel4cp_internal_crash(err);
    }
}

static void
sel4cp_internal_set_up_channel(sel4cp_pd pd_a, sel4cp_pd pd_b, uint8_t channel_id_a, uint8_t channel_id_b) 
{
    // Mint a notification capability to PD a, allowing it to notify PD b.
    seL4_Error err = seL4_CNode_Mint(
        BASE_CNODE_CAP + pd_a, 
        BASE_OUTPUT_NOTIFICATION_CAP + channel_id_a,
        PD_CAP_BITS,
        BASE_CNODE_CAP + pd_b,
        BASE_UNBADGED_CHANNEL_CAP + pd_b,
        PD_CAP_BITS,
        seL4_AllRights,
        1 << channel_id_b
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_channel: failed set up channel capability for PD ");
        sel4cp_dbg_puthex64(pd_a);
        sel4cp_dbg_puts("\n");
        sel4cp_internal_crash(err);
    }
    
    // Mint a notification capability to PD b, allowing it to notify PD a.
    err = seL4_CNode_Mint(
        BASE_CNODE_CAP + pd_b, 
        BASE_OUTPUT_NOTIFICATION_CAP + channel_id_b,
        PD_CAP_BITS,
        BASE_CNODE_CAP + pd_a,
        BASE_UNBADGED_CHANNEL_CAP + pd_a,
        PD_CAP_BITS,
        seL4_AllRights,
        1 << channel_id_a
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_channel: failed set up channel capability for PD ");
        sel4cp_dbg_puthex64(pd_b);
        sel4cp_dbg_puts("\n");
        sel4cp_internal_crash(err);
    }
}

static void
sel4cp_internal_set_up_irq(sel4cp_pd pd, uint8_t parent_irq_channel_id, uint8_t child_irq_channel_id) 
{
    // Ensure that the CSlot used for temporary capabilities is empty.
    sel4cp_internal_delete_temp_cap();

    // Create a badged capability to the channel object of the child PD,
    // ensuring that the child is notified with the correct channel id.
    seL4_Error err = seL4_CNode_Mint(
        BASE_CNODE_CAP + sel4cp_current_pd_id, 
        TEMP_CAP,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        BASE_UNBADGED_CHANNEL_CAP + pd,
        PD_CAP_BITS,
        seL4_AllRights,
        1 << child_irq_channel_id
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_irq: failed to create a badged capability to the child PD to be used for IRQ handling\n");
        sel4cp_internal_crash(err);
    }

    // Register the channel object of the child PD to be notified for the given IRQ,
    // using the badged capability created above.
    err = seL4_IRQHandler_SetNotification(
        BASE_IRQ_CAP + parent_irq_channel_id,
        TEMP_CAP
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_irq: failed to register child as the handler of irq\n");
        sel4cp_internal_crash(err);
    }
    
    // Move the IRQHandler capability into the CSpace of the child PD.
    err = seL4_CNode_Move(
        BASE_CNODE_CAP + pd,
        BASE_IRQ_CAP + child_irq_channel_id,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        BASE_IRQ_CAP + parent_irq_channel_id,
        PD_CAP_BITS
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_irq: failed to move the IRQHandler capability to the child PD\n");
        sel4cp_internal_crash(err);
    }
}

/**
 *  Masks out the lower num_bits bits of n.
 *  I.e. the lower num_bits bits of n are set to 0.
 */
static uint64_t 
sel4cp_internal_mask_bits(uint64_t n, uint8_t num_bits) 
{
    return (n >> num_bits) << num_bits;
}

/**
 *  Parses the given memory_flags into an appropriate seL4_CapRights_t object.
 *  The bits (0-7) in the memory_flags are given meaning in the following way:
 *      1: write
 *      2: read
 *  All bits not mentioned above are ignored and, thus, not given any meaning.
 */
static seL4_CapRights_t 
sel4cp_internal_parse_cap_rights(uint8_t memory_flags)
{
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
static seL4_ARM_VMAttributes 
sel4cp_internal_parse_vm_attributes(uint8_t memory_flags, bool cached) 
{
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
 *  Returns true if the given strings are equal.
 *  Precondition: The two strings are both 0-terminated.
 */
static bool
sel4cp_internal_are_equal(char *a, char *b) {
    if (a == NULL || b == NULL) {
        return false;
    }
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

/**
 *  Returns the virtual address of the IPC buffer for the ELF file
 *  pointed to by the given src.
 */
static uint8_t *
sel4cp_internal_get_ipc_buffer_vaddr(uint8_t *src) 
{
    elf_header *elf_hdr = (elf_header *)src;
    
    // Find the symbol table.
    elf_section_header *symbol_table_hdr = NULL;
    for (uint64_t i = 0; i < elf_hdr->e_shnum; i++) {
        elf_section_header *section_hdr = (elf_section_header *)(src + elf_hdr->e_shoff + (i * elf_hdr->e_shentsize));
        
        if (section_hdr->sh_type == SHT_SYMTAB) {
            symbol_table_hdr = section_hdr;
            break;
        }
    }
    if (symbol_table_hdr == NULL) {
        sel4cp_dbg_puts("sel4cp_internal_get_ipc_buffer_vaddr: failed to find the symbol table\n");
        return NULL;
    }
    
    // Get the associated string table.
    elf_section_header *string_table_hdr = (elf_section_header *)(src + elf_hdr->e_shoff + (symbol_table_hdr->sh_link * elf_hdr->e_shentsize));
    
    // Find the __sel4_ipc_buffer_obj symbol.
    uint8_t *ipc_buffer_vaddr = NULL; 
    elf_symbol_table_entry *symbol_table_entry = (elf_symbol_table_entry *)(src + symbol_table_hdr->sh_offset);
    uint8_t *symbol_table_end = src + symbol_table_hdr->sh_offset + symbol_table_hdr->sh_size; 
    while (((uint8_t *)symbol_table_entry) < symbol_table_end) { 
        uint8_t *symbol_name = src + string_table_hdr->sh_offset + symbol_table_entry->st_name;   
        if (sel4cp_internal_are_equal("__sel4_ipc_buffer_obj", (char *)symbol_name)) {
            ipc_buffer_vaddr = (uint8_t *)symbol_table_entry->st_value;
            break;
        }
        symbol_table_entry++;
    }
    
    return ipc_buffer_vaddr;
}

/**
 *  Ensures that all higher-level paging structures in the ARM AArch64 four-level
 *  page-table structure required to map a page at the given virtual address in the given
 *  PD VSpace are mapped.
 *
 *  The bits in a virtual address are given the following meaning:
 *      -  0-11: offset into a page.
 *      - 12-20: offset into a page table, selecting a specific page.
 *      - 21-29: offset into a page directory, selecting a specific page table.
 *      - 30-38: offset into a page upper directory, selecting a specific page directory.
 *      - 39-47: offset into a page global directory, selecting a specific page upper directory. 
 *  Note that the VSpace is a page global directory in seL4 for ARM AArch64.
 */
static int 
sel4cp_internal_set_up_required_paging_structures(uint64_t vaddr, uint64_t pd_vspace_cap) 
{    
    // Ensure that the required page upper directory is mapped.
    uint64_t page_upper_directory_vaddr = sel4cp_internal_mask_bits(vaddr, 12 + 9 + 9 + 9);
    if (alloc_state.page_upper_directory_idx >= POOL_NUM_PAGE_UPPER_DIRECTORIES) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_required_paging_structures: no page upper directories are available; allocate more and try again\n");
        return -1;
    }
    seL4_Error err = seL4_ARM_PageUpperDirectory_Map(
        BASE_PAGE_UPPER_DIRECTORY_POOL + alloc_state.page_upper_directory_idx,
        pd_vspace_cap,
        page_upper_directory_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        alloc_state.page_upper_directory_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page upper directory has already been mapped.
        sel4cp_dbg_puts("sel4cp_internal_set_up_required_paging_structures: failed to allocate a required page upper directory; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    // Ensure that the required page directory is mapped.
    uint64_t page_directory_vaddr = sel4cp_internal_mask_bits(vaddr, 12 + 9 + 9);
    if (alloc_state.page_directory_idx >= POOL_NUM_PAGE_DIRECTORIES) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_required_paging_structures: no page directories are available; allocate more and try again\n");
        return -1;
    }
    err = seL4_ARM_PageDirectory_Map(
        BASE_PAGE_DIRECTORY_POOL + alloc_state.page_directory_idx,
        pd_vspace_cap,
        page_directory_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        alloc_state.page_directory_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page directory has already been mapped.
        sel4cp_dbg_puts("sel4cp_internal_set_up_required_paging_structures: failed to allocate a required page directory; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    // Ensure that the required page table is mapped.
    uint64_t page_table_vaddr = sel4cp_internal_mask_bits(vaddr, 12 + 9);
    if (alloc_state.page_table_idx >= POOL_NUM_PAGE_TABLES) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_required_paging_structures: no page tables are available; allocate more and try again\n");
        return -1;
    }
    err = seL4_ARM_PageTable_Map(
        BASE_PAGE_TABLE_POOL + alloc_state.page_table_idx,
        pd_vspace_cap,
        page_table_vaddr,
        SEL4_ARM_DEFAULT_VMATTRIBUTES 
    );
    if (err == seL4_NoError) {
        alloc_state.page_table_idx++;
    }
    else if (err != seL4_DeleteFirst) { // if err == seL4_DeleteFirst, the required page table has already been mapped.
        sel4cp_dbg_puts("sel4cp_internal_set_up_required_paging_structures: failed to allocate a required page table; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return -1;
    }
    
    return 0;
}

/**
 *  Allocates a page and maps it at the given virtual address in the given VSpace. 
 *  The page is mapped with the given ELF program header p_flags.
 *
 *  Returns the index of the CSlot containing the allocated page in the current PD on success.
 *  Returns 0 if an error occurs.
 */
static uint64_t
sel4cp_internal_allocate_page(uint64_t vaddr, uint64_t pd_vspace_cap, uint32_t p_flags)
{
    if (sel4cp_internal_set_up_required_paging_structures(vaddr, pd_vspace_cap)) {
        return 0;
    }
    
    // Extract the rights and VM attributes to map the required page with 
    // from the given ELF program header flags.
    seL4_CapRights_t rights = sel4cp_internal_parse_cap_rights((uint8_t)p_flags);
    seL4_ARM_VMAttributes vm_attributes = sel4cp_internal_parse_vm_attributes((uint8_t)p_flags, true);
    
    // Allocate and map the required page.
    uint64_t page_vaddr = sel4cp_internal_mask_bits(vaddr, 12);
    if (alloc_state.page_idx >= POOL_NUM_PAGES) {
        sel4cp_dbg_puts("sel4cp_internal_allocate_page: no pages are available; allocate more and try again\n");
        return 0;
    }
    seL4_Error err = seL4_ARM_Page_Map(
        BASE_PAGE_POOL + alloc_state.page_idx,
        pd_vspace_cap,
        page_vaddr,
        rights,
        vm_attributes
    );
    if (err == seL4_NoError) {
        alloc_state.page_idx++;
    }
    else {
        sel4cp_dbg_puts("sel4cp_internal_allocate_page: failed to allocate a required page; error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return 0;
    }
    
    return BASE_PAGE_POOL + alloc_state.page_idx - 1;
}

/**
 *  Returns a virtual address in the current PD which
 *  can be used to write data that will be available at the
 *  given vaddr in the given pd_vspace.
 *  The required paging structures are automatically allocated,
 *  and the page is mapped with the given ELF program header p_flags.
 *
 *  Returns NULL if the allocation fails. 
 *  Nothing is done to clean up in this case.
 */
static uint8_t *
sel4cp_internal_allocate_page_with_write_handle(uint8_t *src, uint64_t vaddr, uint64_t pd_vspace_cap, uint32_t p_flags) 
{
    uint64_t allocated_page_idx = sel4cp_internal_allocate_page(vaddr, pd_vspace_cap, p_flags);
    if (allocated_page_idx == 0) {
        return NULL;
    }
    
    if (alloc_state.temp_page_vaddr == NULL) {
        uint8_t *ipc_buffer_vaddr = sel4cp_internal_get_ipc_buffer_vaddr(src);
        if (ipc_buffer_vaddr == NULL) {
            return NULL;
        }
        // The IPC buffer is the last page of the ELF, so we use the page after that for the temp loader page.
        alloc_state.temp_page_vaddr = ipc_buffer_vaddr + 0x1000; 
        if (sel4cp_internal_set_up_required_paging_structures((uint64_t)alloc_state.temp_page_vaddr, BASE_VSPACE_CAP + sel4cp_current_pd_id)) {
            return NULL;
        }
    }
    
    // Ensure that the capability slot for the page capability mapped into the current PD's VSpace is empty.
    sel4cp_internal_delete_temp_cap();
    
    // Copy the capability for the allocated page to the temporary page cap CSlot.
    seL4_Error err = seL4_CNode_Copy(
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        TEMP_CAP,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        allocated_page_idx,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_allocate_page: failed to copy page capability required to be able to load ELF file, error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return NULL;
    }
    
    // Map the copied page capability into the VSpace of the current PD.
    err = seL4_ARM_Page_Map(
        TEMP_CAP,
        BASE_VSPACE_CAP + sel4cp_current_pd_id,
        (uint64_t)alloc_state.temp_page_vaddr,
        seL4_ReadWrite,
        SEL4_ARM_DEFAULT_VMATTRIBUTES
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_allocate_page: failed to map the page via the copied page capability into the current PD's VSpace, error code = ");
        sel4cp_dbg_puthex64(err);
        sel4cp_dbg_puts("\n");
        return NULL;
    }

    return alloc_state.temp_page_vaddr + ((uint64_t)(vaddr % 0x1000));
}

/**
 *  Sets up the capabilities for the given program in the given PD.
 */
static int 
sel4cp_internal_set_up_capabilities(uint8_t *elf_file, sel4cp_pd pd) 
{
    sel4cp_dbg_puts("sel4cp_internal_set_up_capabilities: setting up capabilities!\n");
    
    // Get the offset of the capability section, 
    // taking into account that the offset is only 7 bytes long.
    uint64_t capability_offset = *((uint64_t *)(elf_file + EI_CAPABILITY_OFFSET_IDX - 1)) >> 8;
    
    uint8_t *cap_reader = elf_file + capability_offset;
    
    uint64_t num_capabilities = *((uint64_t *) cap_reader);
    cap_reader += 8;
    
    // Setup all capabilities.
    for (uint64_t i = 0; i < num_capabilities; i++) {
        uint8_t cap_type_id = *cap_reader++;
        switch (cap_type_id) {
            case SCHEDULING_ID: {
                uint8_t priority = *cap_reader++;
                uint8_t mcp = *cap_reader++;
                uint64_t budget = *((uint64_t *)cap_reader);
                cap_reader += 8;
                uint64_t period = *((uint64_t *)cap_reader);
                cap_reader += 8;
                
                sel4cp_internal_set_priority(pd, priority, mcp);
                sel4cp_internal_set_sched_flags(pd, budget, period);
                
                sel4cp_dbg_puts("sel4cp_internal_set_up_capabilities: set scheduling parameters, priority = ");
                sel4cp_dbg_puthex64(priority);
                sel4cp_dbg_puts(" , mcp = ");
                sel4cp_dbg_puthex64(mcp);
                sel4cp_dbg_puts(" , budget = ");
                sel4cp_dbg_puthex64(budget);
                sel4cp_dbg_puts(" , period = ");
                sel4cp_dbg_puthex64(period);
                sel4cp_dbg_puts("\n");
                break;
            }
            case CHANNEL_ID: {
                uint8_t target_pd = *cap_reader++;
                uint8_t target_id = *cap_reader++;
                uint8_t own_id = *cap_reader++;
                
                sel4cp_internal_set_up_channel(pd, target_pd, own_id, target_id);
                
                sel4cp_dbg_puts("sel4cp_internal_set_up_capabilities: set up channel - pd_a = ");
                sel4cp_dbg_puthex64(pd);
                sel4cp_dbg_puts(", pd_b = ");
                sel4cp_dbg_puthex64(target_pd);
                sel4cp_dbg_puts(", channel_id_a = ");
                sel4cp_dbg_puthex64(own_id);
                sel4cp_dbg_puts(", channel_id_b = ");
                sel4cp_dbg_puthex64(target_id);
                sel4cp_dbg_puts("\n");
                break;
            }
            case MEMORY_REGION_ID: {
                uint64_t id = *((uint64_t *) cap_reader);
                cap_reader += 8;
                uint64_t vaddr = *((uint64_t *) cap_reader);
                cap_reader += 8;
                uint64_t size = *((uint64_t *) cap_reader);
                cap_reader += 8;
                uint8_t perms = *cap_reader++;
                uint8_t cached = *cap_reader++;
                
                // Parse the rights and VM attributes.
                seL4_CapRights_t rights = sel4cp_internal_parse_cap_rights(perms);
                seL4_ARM_VMAttributes vm_attributes = sel4cp_internal_parse_vm_attributes(perms, cached); 
                
                // Map the page into the child PD's VSpace.
                uint64_t pd_vspace_cap = BASE_VSPACE_CAP + pd;
                uint64_t num_pages = size / 0x1000; // Assumes that the size is a multiple of the page size 0x1000.
                for (uint64_t j = 0; j < num_pages; j++) {
                    uint64_t page_cap = BASE_SHARED_MEMORY_REGION_PAGES + id + j;
                    uint64_t page_vaddr = vaddr + (j * 0x1000);
                    
                    sel4cp_dbg_puts("Mapping page at CSlot = ");
                    sel4cp_dbg_puthex64(page_cap);
                    sel4cp_dbg_puts("\n");
                    
                    // Ensure that all required higher-level paging structures are mapped before mapping this page.
                    if (sel4cp_internal_set_up_required_paging_structures(page_vaddr, BASE_VSPACE_CAP + pd)) {
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
                        sel4cp_dbg_puts("sel4cp_internal_set_up_capabilities: failed to map page for child\n");
                        sel4cp_dbg_puthex64(err);
                        sel4cp_dbg_puts("\n");
                        return -1;
                    } 
                }                
                
                sel4cp_dbg_puts("sel4cp_internal_set_up_capabilities: set up memory region - id = ");
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
            }
            case IRQ_ID: {
                uint8_t parent_irq_channel_id = *cap_reader++;
                uint8_t child_irq_channel_id = *cap_reader++;
                
                sel4cp_internal_set_up_irq(pd, parent_irq_channel_id, child_irq_channel_id);
                
                sel4cp_dbg_puts("sel4cp_internal_set_up_capabilities: set up irq - parent_irq_channel_id = ");
                sel4cp_dbg_puthex64(parent_irq_channel_id);
                sel4cp_dbg_puts(", child_irq_channel_id = ");
                sel4cp_dbg_puthex64(child_irq_channel_id);
                sel4cp_dbg_puts("\n");
                break;
            }
            default:
                sel4cp_dbg_puts("sel4cp_internal_set_up_capabilities: invalid capability type id: ");
                sel4cp_dbg_puthex64(cap_type_id);
                sel4cp_dbg_puts("\n");
                return -1;
        }
    }
    
    return 0;
}

static int
sel4cp_internal_set_up_ipc_buffer(uint8_t *src, sel4cp_pd pd)
{
    uint8_t *ipc_buffer_vaddr = sel4cp_internal_get_ipc_buffer_vaddr(src);
    if (ipc_buffer_vaddr == NULL) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_ipc_buffer: failed to find the __sel4_ipc_buffer_obj symbol\n");
        return -1;
    }
    
    // Allocate the IPC buffer.
    uint64_t ipc_buffer_cap_idx = sel4cp_internal_allocate_page(
        (uint64_t) ipc_buffer_vaddr, 
        BASE_VSPACE_CAP + pd, 
        P_FLAGS_WRITABLE | P_FLAGS_READABLE
    );
    if (ipc_buffer_cap_idx == 0) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_ipc_buffer: failed to allocate a page for the IPC buffer of the PD\n");
        return -1;
    }
    
    // Set the IPC buffer for the new PD.
    seL4_Error err = seL4_TCB_SetIPCBuffer(
        BASE_TCB_CAP + pd,
        (uint64_t) ipc_buffer_vaddr,
        ipc_buffer_cap_idx
    );
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_internal_set_up_ipc_buffer: failed to set the IPC buffer of the PD\n");
        return -1;
    }
    
    return 0;
}

// ========== END OF UTILITY FUNCTIONS ==========

// ========== PUBLIC INTERFACE ==========

static inline void
sel4cp_notify(sel4cp_channel ch)
{
    seL4_Signal(BASE_OUTPUT_NOTIFICATION_CAP + ch);
}

static inline void
sel4cp_irq_ack(sel4cp_channel ch)
{
    seL4_IRQHandler_Ack(BASE_IRQ_CAP + ch);
}

static void
sel4cp_pd_restart(sel4cp_pd pd, uintptr_t entry_point)
{
    seL4_Error err;
    seL4_UserContext ctxt = {0};
    ctxt.pc = entry_point;
    err = seL4_TCB_WriteRegisters(
        BASE_TCB_CAP + pd,
        true,
        0, /* No flags */
        1, /* writing 1 register */
        &ctxt
    );

    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_pd_restart: error writing registers\n");
        sel4cp_internal_crash(err);
    }
}

static void
sel4cp_pd_stop(sel4cp_pd pd)
{
    seL4_Error err;
    err = seL4_TCB_Suspend(BASE_TCB_CAP + pd);
    if (err != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_pd_stop: error writing registers\n");
        sel4cp_internal_crash(err);
    }
}

static inline sel4cp_msginfo
sel4cp_ppcall(sel4cp_channel ch, sel4cp_msginfo msginfo)
{
    return seL4_Call(BASE_OUTPUT_ENDPOINT_CAP + ch, msginfo);
}

static inline sel4cp_msginfo
sel4cp_msginfo_new(uint64_t label, uint16_t count)
{
    return seL4_MessageInfo_new(label, 0, 0, count);
}

static inline uint64_t
sel4cp_msginfo_get_label(sel4cp_msginfo msginfo)
{
    return seL4_MessageInfo_get_label(msginfo);
}

static void
sel4cp_mr_set(uint8_t mr, uint64_t value)
{
    seL4_SetMR(mr, value);
}

static uint64_t
sel4cp_mr_get(uint8_t mr)
{
    return seL4_GetMR(mr);
}

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
static int 
sel4cp_pd_load_elf(uint8_t *src, sel4cp_pd pd, uint64_t *entry_point) 
{
    elf_header *elf_hdr = (elf_header *)src;
    
    // Set the entry point of the given program.
    *entry_point = elf_hdr->e_entry;
    
    for (uint64_t i = 0; i < elf_hdr->e_phnum; i++) {
        elf_program_header *prog_hdr = (elf_program_header *)(src + elf_hdr->e_phoff + (i * elf_hdr->e_phentsize));
        if (prog_hdr->p_type != PT_LOAD)
            continue; // the segment should not be loaded.
        
        uint8_t *dst_write = sel4cp_internal_allocate_page_with_write_handle(src, prog_hdr->p_vaddr, BASE_VSPACE_CAP + pd, prog_hdr->p_flags);
        if (dst_write == NULL) {
            sel4cp_dbg_puts("sel4cp_pd_load_elf: failed to allocate a page required to load the ELF file\n");
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
                dst_write = sel4cp_internal_allocate_page_with_write_handle(src, current_vaddr, BASE_VSPACE_CAP + pd, prog_hdr->p_flags);
                if (dst_write == NULL) {
                    sel4cp_dbg_puts("sel4cp_pd_load_elf: failed to allocate a page required to load the ELF file\n");
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
                    dst_write = sel4cp_internal_allocate_page_with_write_handle(src, current_vaddr, BASE_VSPACE_CAP + pd, prog_hdr->p_flags);
                    if (dst_write == NULL) {
                        sel4cp_dbg_puts("sel4cp_pd_load_elf: failed to allocate a page required to load the ELF file\n");
                        return -1;
                    }
                }
            }
        }
    }
    
    if (sel4cp_internal_set_up_ipc_buffer(src, pd)) {
        sel4cp_dbg_puts("sel4cp_pd_load_elf: failed to set up the IPC buffer\n");
        return -1;
    }
    
    return sel4cp_internal_set_up_capabilities(src, pd);
}

/**
 *  Loads and runs the given program in the given PD.
 *  Precondition: The given src is assumed to point to an extended ELF file with capabilities.   
 *
 *  Returns 0 on success.
 *  Returns -1 if an error occurs.
 */
static int 
sel4cp_pd_run_elf(uint8_t *src, sel4cp_pd pd) 
{
    uint64_t entry_point;
    int result = sel4cp_pd_load_elf(src, pd, &entry_point);
    if (result)
        return result;
    
    sel4cp_pd_restart(pd, entry_point);
    
    return 0;
}


/**
 *  Creates a new PD with the given id.
 *  If src is not NULL, the ELF file pointed to by this pointer
 *  is loaded and the PD is started.
 *  Otherwise, no program is loaded for the PD, and the PD is not started.
 *  Precondition: No PD with the given id already exists in the system.
 *
 *  Returns 0 on success.
 *  Returns -1 if an error occurs.
 */
static int
sel4cp_pd_create(sel4cp_pd pd, uint8_t *src) 
{
    // Allocate a TCB for the new PD.
    if (alloc_state.tcb_idx >= POOL_NUM_TCBS) {
        return -1;
    }
    uint64_t tcb_cap = BASE_TCB_POOL + alloc_state.tcb_idx;
    alloc_state.tcb_idx++;
    
    // Allocate a notification for the new PD.
    if (alloc_state.notification_idx >= POOL_NUM_NOTIFICATIONS) {
        return -1;
    }
    uint64_t notification_cap = BASE_NOTIFICATION_POOL + alloc_state.notification_idx;
    alloc_state.notification_idx++;
    
    // Allocate a CNode for the new PD.
    if (alloc_state.cnode_idx >= POOL_NUM_CNODES) {
        return -1;
    }
    uint64_t cnode_cap = BASE_CNODE_POOL + alloc_state.cnode_idx;
    alloc_state.cnode_idx++;
    
    // Allocate a SchedContext for the new PD.
    if (alloc_state.schedcontext_idx >= POOL_NUM_SCHEDCONTEXTS) {
        return -1;
    }
    uint64_t schedcontext_cap = BASE_SCHEDCONTEXT_POOL + alloc_state.schedcontext_idx;
    alloc_state.schedcontext_idx++;

    // Allocate a VSpace for the new PD.
    if (alloc_state.vspace_idx >= POOL_NUM_VSPACES) {
        return -1;
    }
    uint64_t vspace_cap = BASE_VSPACE_POOL + alloc_state.vspace_idx;
    alloc_state.vspace_idx++;
    
    // Assign the VSpace to the same ASID pool as all other VSpaces in the system.
    seL4_Error err = seL4_ARM_ASIDPool_Assign(ASID_POOL_CAP_IDX, vspace_cap);
    if (err != seL4_NoError) {
        return -1;
    }
    
    // Mint acccess to all fixed capabilities.
    // 1. SchedControl capability.
    err = seL4_CNode_Copy(
        cnode_cap,
        SCHED_CONTROL_CAP_IDX,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        SCHED_CONTROL_CAP_IDX,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    // 2. Unbadged channel/notification capability.
    err = seL4_CNode_Copy(
        cnode_cap,
        INPUT_CAP_IDX,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        notification_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    err = seL4_CNode_Copy(
        cnode_cap,
        BASE_UNBADGED_CHANNEL_CAP + pd,
        PD_CAP_BITS,
        cnode_cap,
        INPUT_CAP_IDX,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    err = seL4_CNode_Copy(
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        BASE_UNBADGED_CHANNEL_CAP + pd,
        PD_CAP_BITS,
        cnode_cap,
        INPUT_CAP_IDX,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    // 3. ASID Pool capability.
    err = seL4_CNode_Copy(
        cnode_cap,
        ASID_POOL_CAP_IDX,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        ASID_POOL_CAP_IDX,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    // 4. TCB capability.
    err = seL4_CNode_Copy(
        cnode_cap,
        BASE_TCB_CAP + pd,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        tcb_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    err = seL4_CNode_Copy(
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        BASE_TCB_CAP + pd,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        tcb_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    // 5. SchedContext capability.
    err = seL4_CNode_Copy(
        cnode_cap,
        BASE_SCHED_CONTEXT_CAP + pd,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        schedcontext_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    err = seL4_CNode_Copy(
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        BASE_SCHED_CONTEXT_CAP + pd,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        schedcontext_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    // 6. CNode capability.
    err = seL4_CNode_Copy(
        cnode_cap,
        BASE_CNODE_CAP + pd,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        cnode_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    err = seL4_CNode_Copy(
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        BASE_CNODE_CAP + pd,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        cnode_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    // 7. VSpace capability.
    err = seL4_CNode_Copy(
        cnode_cap,
        BASE_VSPACE_CAP + pd,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        vspace_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    err = seL4_CNode_Copy(
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        BASE_VSPACE_CAP + pd,
        PD_CAP_BITS,
        BASE_CNODE_CAP + sel4cp_current_pd_id,
        vspace_cap,
        PD_CAP_BITS,
        seL4_AllRights
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    
    // Set the VSpace, CSpace, and fault endpoint.
    err = seL4_TCB_SetSpace(
        tcb_cap,
        FAULT_EP_CAP_IDX,
        cnode_cap,
        64 - PD_CAP_BITS,
        vspace_cap,
        0
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    // Bind the notification object.
    err = seL4_TCB_BindNotification(
        tcb_cap,
        notification_cap
    );
    if (err != seL4_NoError) {
        return -1;
    }
    
    if (src != NULL) {
        return sel4cp_pd_run_elf(src, pd);
    }
    
    return 0;
}


// ========== END OF PUBLIC INTERFACE ==========








