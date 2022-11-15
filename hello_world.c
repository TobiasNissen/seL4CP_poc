#include <stdint.h>
#include <sel4cp.h>

void
init(void)
{
    sel4cp_dbg_puts("Hello world!\n");
}

void
notified(sel4cp_channel ch)
{
}
