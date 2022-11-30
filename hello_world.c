#include <stdint.h>
#include <sel4cp.h>

#define PING_CHANNEL_ID 1

#define VADDR 0x209000

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
notified(sel4cp_channel ch)
{
    sel4cp_dbg_puts("hello_world: got notified on channel ");
    sel4cp_dbg_puthex64(ch);
    sel4cp_dbg_puts("\n");
}
