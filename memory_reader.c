#include <stdint.h>
#include <sel4cp.h>

#define VADDR 0x5000000

void
init(void)
{
    uint8_t *memory = (uint8_t *)VADDR;
    sel4cp_dbg_puts("memory_reader: initialized!\n");
    sel4cp_dbg_puts("memory_reader: reading value (expecting 0x2a): ");
    sel4cp_dbg_puthex64(*memory);
    sel4cp_dbg_puts("\n");
}

void
notified(sel4cp_channel channel)
{
}
