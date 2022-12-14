#include <stdint.h>
#include <sel4cp.h>

#include "uart.h"

#define UART_IRQ_CHANNEL_ID 0

#define CHILD_PD_ID 1

#define ELF_BUFFER_SIZE 0x30000

uint8_t elf_buffer[ELF_BUFFER_SIZE];

uint8_t *elf_current_vaddr;
uint8_t *test_region_vaddr;

uint64_t elf_size = 0;

char size_buffer[16];
uint8_t size_buffer_idx = 0;

uintptr_t uart_base_vaddr;

/**
 *  Converts the given hexadecimal number with num_digits digits
 *  into a uint64_t.  
 *  
 *  The hexadecimal number is assumed to be stored with the most significant
 *  digit at the lowest index.
 */
uint64_t parse_hex64(char *digits, uint8_t num_digits) {
    uint64_t result = 0;
    for (int i = num_digits - 1; i >= 0; i--) {
        uint64_t value = 0;
        
        char digit = digits[i];
        if (digit >= '0' && digit <= '9') {
            value = digit - '0';
        }
        else if (digit >= 'a' && digit <= 'f') {
            value = digit - 'a' + 10;
        }
        else if (digit >= 'A' && digit <= 'F') {
            value = digit - 'A' + 10;
        }
        else {
            sel4cp_dbg_puts("root: invalid hexadecimal digit encountered!\n");
            return 0;
        }
        
        result += value * (1 << (4 * (num_digits - 1 - i)));
    }
    return result;
} 

void
init(void)
{
    uart_init();
    elf_current_vaddr = elf_buffer;
    sel4cp_dbg_puts("root: initialized!\n");
    sel4cp_dbg_puts("root: writing 42 (0x2a) to shared memory region!\n");
    *test_region_vaddr = 42;
    
    sel4cp_dbg_puts("root: ready to receive ELF file to load dynamically; start by sending the size of the file as a hexadecimal string ended by newline\n");
}

void
notified(sel4cp_channel channel)
{
    if (channel != UART_IRQ_CHANNEL_ID) {
        sel4cp_dbg_puts("root: got notified by unknown channel!\n");
        return;
    }
    
    uart_handle_irq();
    char c = uart_get_char();
    sel4cp_irq_ack(channel);
    
    if (elf_size == 0) { // We are still reading the size of the ELF file to load.
        if (c == '\n') {
            elf_size = parse_hex64(size_buffer, size_buffer_idx);
            
            size_buffer_idx = 0;
            
            if (elf_size > ELF_BUFFER_SIZE) {
                sel4cp_dbg_puts("root: cannot read ELF files larger than ");
                sel4cp_dbg_puthex64(ELF_BUFFER_SIZE);
                sel4cp_dbg_puts(" bytes\n");
                return;
            }
        
            sel4cp_dbg_puts("root: ready to read ELF file of size ");
            sel4cp_dbg_puthex64(elf_size);
            sel4cp_dbg_puts(" bytes!\n");
        }
        else if (size_buffer_idx >= 16) {
            sel4cp_dbg_puts("root: the size of an ELF file can not be larger than 64 bytes (16 hexadecimal digits)\n");
            return;
        }
        else {
            size_buffer[size_buffer_idx] = c;
            size_buffer_idx++;
        }
        
        return;
    }
        
    // Write to the ELF buffer
    *elf_current_vaddr = c;
    elf_current_vaddr++;
    
    if (elf_current_vaddr - elf_buffer >= elf_size) {
        sel4cp_dbg_puts("root: finished reading ELF file!\n");   
        
        elf_current_vaddr = elf_buffer;
        elf_size = 0;
        
        if (sel4cp_pd_create(CHILD_PD_ID, elf_buffer)) {
            sel4cp_dbg_puts("root: failed to create a new PD with id ");
            sel4cp_dbg_puthex64(CHILD_PD_ID);
            sel4cp_dbg_puts(" and load the provided ELF file\n");
            return;
        }
        sel4cp_dbg_puts("root: successfully started the program in a new child PD\n");
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
    sel4cp_dbg_puthex64(pd);
    sel4cp_dbg_puts("\n");
    sel4cp_dbg_puts("root: label = ");
    sel4cp_dbg_puthex64(sel4cp_msginfo_get_label(msginfo));
    sel4cp_dbg_puts("\n");
    sel4cp_dbg_puts("root: fault_addr = ");
    sel4cp_dbg_puthex64(seL4_GetMR(seL4_CapFault_Addr));
    sel4cp_dbg_puts("\n");
}

