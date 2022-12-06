#include <stdint.h>
#include <sel4cp.h>

#include "uart.h"

#define PING_CHANNEL_ID 1
#define IRQ_CHANNEL_ID 4

#define VADDR 0x5000000
uintptr_t uart_base_vaddr = 0x2000000;

void
init(void)
{
    uint8_t *memory = (uint8_t *)VADDR;
    sel4cp_dbg_puts("hello_world: initialized!\n");
    sel4cp_dbg_puts("hello_world: reading value: ");
    sel4cp_dbg_puthex64(*memory);
    sel4cp_dbg_puts("\nhello_world: sending ping!\n");
    sel4cp_notify(PING_CHANNEL_ID);
}

void
notified(sel4cp_channel channel)
{
    if (channel == IRQ_CHANNEL_ID) {
        uart_handle_irq();
        char c = uart_get_char();
        sel4cp_irq_ack(channel);
    
        sel4cp_dbg_puts("hello_world: ");
        sel4cp_dbg_putc(c);
        sel4cp_dbg_puts("\n");
    }
    else {
        sel4cp_dbg_puts("hello_world: got notified on channel ");
        sel4cp_dbg_puthex64(channel);
        sel4cp_dbg_puts("\n");
    }
}
