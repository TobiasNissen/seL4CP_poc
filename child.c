#include <stdint.h>
#include <sel4cp.h>

#include "uart.h"
#include "elf_loader.h"

#define PING_CHANNEL_ID 1
#define IRQ_CHANNEL_ID 4
#define CHILD_PD_ID 5

#define VADDR 0x5000000
uint8_t *uart_base_vaddr = (uint8_t *)0x2000000;

void
init(void)
{
    uint8_t *memory = (uint8_t *)VADDR;
    sel4cp_dbg_puts("child: initialized!\n");
    sel4cp_dbg_puts("child: reading value (expecting 0x2a): ");
    sel4cp_dbg_puthex64(*memory);
    sel4cp_dbg_puts("\nchild: sending ping!\n");
    sel4cp_notify(PING_CHANNEL_ID);
}

void
notified(sel4cp_channel channel)
{
    if (channel == IRQ_CHANNEL_ID) {
        uart_handle_irq();
        char c = uart_get_char();
        sel4cp_irq_ack(channel);
        
        uint8_t *elf_vaddr = elf_loader_handle_input(c);
        if (elf_vaddr) {
            if (sel4cp_pd_create(CHILD_PD_ID, elf_vaddr, false)) {
                sel4cp_dbg_puts("child: failed to create a new PD with id ");
                sel4cp_dbg_puthex64(CHILD_PD_ID);
                sel4cp_dbg_puts(" and load the provided ELF file\n");
                return;
            }
            sel4cp_dbg_puts("child: successfully started the program in a new child PD\n");
        }
    }
    else {
        sel4cp_dbg_puts("child: got notified on channel ");
        sel4cp_dbg_puthex64(channel);
        sel4cp_dbg_puts("\n");
        sel4cp_dbg_puts("child: ready to receive ELF file to load dynamically!\n");
    }
}
