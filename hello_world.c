#include <stdint.h>
#include <sel4cp.h>

#define PING_CHANNEL_ID 1

void
init(void)
{
    sel4cp_dbg_puts("hello_world: initialized; sending ping!\n");
    sel4cp_notify(PING_CHANNEL_ID);
}

void
notified(sel4cp_channel ch)
{
    sel4cp_dbg_puts("hello_world: got notified on channel ");
    sel4cp_dbg_puthex64(ch);
    sel4cp_dbg_puts("\n");
}
