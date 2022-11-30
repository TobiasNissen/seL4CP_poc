#include <stdint.h>
#include <sel4cp.h>

#include "uart.h"
#include "elf_loader.h"

#define UART_IRQ_CHANNEL_ID 0

#define CHILD_PD_ID 1
#define CHILD_PD_ENTRY_POINT 0x200000

#define HELLO_WORLD_ELF_SIZE 139347

uint8_t *elf_buffer_vaddr;
uint8_t *elf_current_vaddr;
uint64_t elf_size = 0;

uint8_t *loaded_elf_vaddr;

uint8_t *test_region_vaddr;

void
init(void)
{
    uart_init();
    elf_current_vaddr = elf_buffer_vaddr;
    sel4cp_dbg_puts("root: initialized!\n");
    sel4cp_dbg_puts("root: Writing 42 to shared memory region!\n");
    *test_region_vaddr = 42;
}

void
notified(sel4cp_channel channel)
{
    if (channel != UART_IRQ_CHANNEL_ID) {
        sel4cp_dbg_puts("root: got notified by unknown channel!\n");
        return;
    }
    
    if (elf_size == 0) {
        sel4cp_dbg_puts("root: starting to read ELF file!\n");
    }
    
    uart_handle_irq();
    char c = uart_get_char();
    
    *elf_current_vaddr = c;
    elf_current_vaddr++;
    elf_size++;
    
    if (elf_size >= HELLO_WORLD_ELF_SIZE) {
        sel4cp_dbg_puts("root: finished reading file!\n");     
        sel4cp_dbg_puts("root: loading ELF segments for child\n");
        
        elf_loader_load_segments(elf_buffer_vaddr, loaded_elf_vaddr, CHILD_PD_ENTRY_POINT);
   
        sel4cp_dbg_puts("root: loaded all ELF segments; setting up capabilities!\n");
        
        elf_loader_setup_capabilities(elf_buffer_vaddr, HELLO_WORLD_ELF_SIZE, CHILD_PD_ID);
        
        sel4cp_dbg_puts("root: finished setting up capabilities; executing program!\n");
        
        sel4cp_pd_restart(CHILD_PD_ID, CHILD_PD_ENTRY_POINT);
        
        sel4cp_dbg_puts("root: restarted PD!\n");
    }
    
    sel4cp_irq_ack(channel);
}

seL4_MessageInfo_t
protected(sel4cp_channel ch, sel4cp_msginfo msginfo)
{
    sel4cp_dbg_puts("root: received protected message\n");

    return sel4cp_msginfo_new(0, 0);
}

void
fault(sel4cp_pd pd, sel4cp_msginfo msginfo)
{
    sel4cp_dbg_puts("root: received fault message for pd: ");
    sel4cp_dbg_puthex64(pd);
    sel4cp_dbg_puts("\n");
    sel4cp_dbg_puts("root: label = ");
    sel4cp_dbg_puthex64(sel4cp_msginfo_get_label(msginfo));
    sel4cp_dbg_puts("\n");
    sel4cp_dbg_puts("root: fault_addr = ");
    sel4cp_dbg_puthex64(seL4_GetMR(seL4_CapFault_Addr));
    sel4cp_dbg_puts("\n");
}

