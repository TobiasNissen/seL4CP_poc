#include <stdint.h>
#include <sel4cp.h>

void
init(void)
{
    int *x = 0;
    sel4cp_dbg_puts("crasher: about to crash!\n");
    *x = 42;
}

void
notified(sel4cp_channel ch)
{
}
