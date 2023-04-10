#define ELF_BUFFER_SIZE 0x50000

static uint8_t elf_buffer[ELF_BUFFER_SIZE];
static uint8_t *elf_current_vaddr = elf_buffer;
static uint64_t elf_size = 0;

static char size_buffer[16];
static uint8_t size_buffer_idx = 0;

/**
 *  Converts the given hexadecimal number with num_digits digits
 *  into a uint64_t.  
 *  
 *  The hexadecimal number is assumed to be stored with the most significant
 *  digit at the lowest index.
 */
static uint64_t 
elf_loader_parse_hex64(char *digits, uint8_t num_digits) 
{
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
            sel4cp_dbg_puts("elf_loader: invalid hexadecimal digit encountered!\n");
            return 0;
        }
        
        result += value * (1 << (4 * (num_digits - 1 - i)));
    }
    return result;
}

/**
 *  Handles the given input character in the process of loading an ELF file.
 *
 *  If the ELF file has been loaded successfully, a pointer to the loaded ELF
 *  file is returned.
 *  Otherwise, NULL is returned.
 */
static uint8_t *
elf_loader_handle_input(char c)
{
    if (elf_size == 0) { // We are still reading the size of the ELF file to load.
        if (c == '\n') {
            elf_size = elf_loader_parse_hex64(size_buffer, size_buffer_idx);
            
            size_buffer_idx = 0;
            
            if (elf_size > ELF_BUFFER_SIZE) {
                sel4cp_dbg_puts("elf_loader: cannot read ELF files larger than ");
                sel4cp_dbg_puthex64(ELF_BUFFER_SIZE);
                sel4cp_dbg_puts(" bytes\n");
                return NULL; 
            }
        }
        else if (size_buffer_idx >= 16) {
            sel4cp_dbg_puts("elf_loader: the size of an ELF file can not be larger than 64 bytes (16 hexadecimal digits)\n");
            return NULL;
        }
        else {
            size_buffer[size_buffer_idx] = c;
            size_buffer_idx++;
        }
        
        return NULL;
    }
        
    // Write to the ELF buffer
    *elf_current_vaddr = c;
    elf_current_vaddr++;
    
    if (elf_current_vaddr - elf_buffer >= elf_size) {
        elf_current_vaddr = elf_buffer;
        elf_size = 0;
        return elf_buffer;
    }
    
    return NULL;
}


