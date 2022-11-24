#include <stdint.h>
#include <sel4cp.h>

void
init(void)
{
    sel4cp_dbg_puts("ping: initialized!\n");
}

// A simple ping program that responds on the channel with
// the same id as it received a message on.
void
notified(sel4cp_channel channel)
{
    sel4cp_dbg_puts("ping: received message on channel ");
    sel4cp_dbg_puthex64(channel);
    sel4cp_dbg_puts("\n");
    
    sel4cp_dbg_puts("ping: pinging the same channel\n");
    
    sel4cp_notify(channel);
}

seL4_MessageInfo_t
protected(sel4cp_channel ch, sel4cp_msginfo msginfo)
{
    sel4cp_dbg_puts("ping: received protected message\n");

    return sel4cp_msginfo_new(0, 0);
}

void
fault(sel4cp_pd pd, sel4cp_msginfo msginfo)
{
    sel4cp_dbg_puts("ping: received fault message!\n");
}

