#include <stdint.h>
#include <sel4cp.h>

#include "uart.h"

#define UART_IRQ_CHANNEL_ID 0

#define CHILD_PD_ID 1
#define CHILD_PD_ENTRY_POINT 0x200000

static uint8_t restart_count = 0;

static char
hexchar(unsigned int v)
{
    return v < 10 ? '0' + v : ('a' - 10) + v;
}

static void
puthex64(uint64_t val)
{
    char buffer[16 + 3];
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[16 + 3 - 1] = 0;
    for (unsigned i = 16 + 1; i > 1; i--) {
        buffer[i] = hexchar(val & 0xf);
        val >>= 4;
    }
    sel4cp_dbg_puts(buffer);
}

static char
decchar(unsigned int v) {
    return '0' + v;
}

static void
put8(uint8_t x)
{
    char tmp[4];
    unsigned i = 3;
    tmp[3] = 0;
    do {
        uint8_t c = x % 10;
        tmp[--i] = decchar(c);
        x /= 10;
    } while (x);
    sel4cp_dbg_puts(&tmp[i]);
}

void
init(void)
{
    uart_init();
    uart_put_str("root: initialized!\n");
    // sel4cp_dbg_puts("root: initialized!\n");
    
    // sel4cp_pd_restart(CHILD_PD_ID, CHILD_PD_ENTRY_POINT);
}

void
notified(sel4cp_channel channel)
{
    switch (channel) {
        case UART_IRQ_CHANNEL_ID:
            uart_handle_irq();
            
            char c = uart_get_char();
            uart_put_char(c);
            
            sel4cp_irq_ack(channel);
            break;
        default:
            uart_put_str("root: got notified by unknown channel!\n");
            break;
    }
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
    put8(pd);
    sel4cp_dbg_puts("\n");
    
    /*
    restart_count++;
    if (restart_count < 10) {
        sel4cp_pd_restart(pd, 0x200000); // The entry point address can be found in the ELF header.
        sel4cp_dbg_puts("root: restarted\n");
    } else {
        sel4cp_pd_stop(pd);
        sel4cp_dbg_puts("root: too many restarts - PD stopped\n");
    }
    */
}

