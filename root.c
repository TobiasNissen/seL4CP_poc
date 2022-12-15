#include <stdint.h>
#include <sel4cp.h>

#include "uart.h"
#include "elf_loader.h"

#define UART_IRQ_CHANNEL_ID 0
#define CHILD_PD_ID 1

uint8_t *test_region_vaddr;
uint8_t *uart_base_vaddr;

void
init(void)
{
    uart_init();
    sel4cp_dbg_puts("root: initialized!\n");
    sel4cp_dbg_puts("root: writing 42 (0x2a) to shared memory region!\n");
    *test_region_vaddr = 42;
    
    sel4cp_dbg_puts("root: ready to receive ELF file to load dynamically!\n");
}

void
notified(sel4cp_channel channel)
{
    if (channel != UART_IRQ_CHANNEL_ID) {
        sel4cp_dbg_puts("root: got notified by unknown channel!\n");
        return;
    }
    
    uart_handle_irq();
    char c = uart_get_char();
    sel4cp_irq_ack(channel);
    
    uint8_t *elf_vaddr = elf_loader_handle_input(c);
    if (elf_vaddr) {
        if (sel4cp_pd_create(CHILD_PD_ID, elf_vaddr, true)) {
            sel4cp_dbg_puts("root: failed to create a new PD with id ");
            sel4cp_dbg_puthex64(CHILD_PD_ID);
            sel4cp_dbg_puts(" and load the provided ELF file\n");
            return;
        }
        sel4cp_dbg_puts("root: successfully started the program in a new child PD\n");
    }
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

