#include <stdint.h>
#include <sel4cp.h>

#include "uart.h"

#define UART_IRQ_CHANNEL_ID 0

#define CHILD_PD_ID 1
#define CHILD_PD_ENTRY_POINT 0x200000

#define HELLO_WORLD_ELF_SIZE 136408

uint8_t *elf_buffer_vaddr;
uint8_t *elf_current_vaddr;
uint64_t elf_size = 0;

uint8_t *loaded_elf_vaddr;

void
init(void)
{
    uart_init();
    elf_current_vaddr = elf_buffer_vaddr;
    uart_put_str("root: initialized!\n");
}

void
notified(sel4cp_channel channel)
{
    if (channel != UART_IRQ_CHANNEL_ID) {
        uart_put_str("root: got notified by unknown channel!\n");
        return;
    }
    
    uart_handle_irq();
    char c = uart_get_char();
    
    *elf_current_vaddr = c;
    elf_current_vaddr++;
    elf_size++;
    
    if (elf_size >= HELLO_WORLD_ELF_SIZE) {
        uart_put_str("root: finished reading file!\n");
        uart_put_str("root: loading ELF segments for child\n");
        
        elf_current_vaddr = elf_buffer_vaddr + 0x10000;
        
        for (uint64_t i = 0; i < 0x364; i++) {
            *loaded_elf_vaddr = *elf_current_vaddr;
            loaded_elf_vaddr++;
            elf_current_vaddr++;
        }
        uart_put_str("root: loaded segment 1; loading segment 2\n");
        
        elf_current_vaddr = elf_buffer_vaddr + 0x11000;
        for (uint64_t i = 0; i < 0x8; i++) {
            *loaded_elf_vaddr = *elf_current_vaddr;
            loaded_elf_vaddr++;
            elf_current_vaddr++;
        }
        uart_put_str("root: loaded first part of segment 2; missing zeros\n");
        for (uint64_t i = 0; i < (0x1020 - 0x8); i++) {
            *loaded_elf_vaddr = 0;
            loaded_elf_vaddr++;
        }
        uart_put_str("root: loaded all ELF segments; starting program!\n");
        
        
        sel4cp_pd_restart(CHILD_PD_ID, CHILD_PD_ENTRY_POINT);
    }
    
    sel4cp_irq_ack(channel);
}

seL4_MessageInfo_t
protected(sel4cp_channel ch, sel4cp_msginfo msginfo)
{
    uart_put_str("root: received protected message\n");

    return sel4cp_msginfo_new(0, 0);
}

void
fault(sel4cp_pd pd, sel4cp_msginfo msginfo)
{
    uart_put_str("root: received fault message for pd: ");
    uart_put_hex64(pd);
    uart_put_str("\n");
    uart_put_str("root: label = ");
    uart_put_hex64(sel4cp_msginfo_get_label(msginfo));
    uart_put_str("\n");
    uart_put_str("root: fault_addr = ");
    uart_put_hex64(seL4_GetMR(seL4_CapFault_Addr));
    uart_put_str("\n");
}

