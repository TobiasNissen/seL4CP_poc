BOARD := qemu_arm_virt
SEL4CP_CONFIG := debug
BUILD_DIR := build
TOOLCHAIN := aarch64-linux-gnu
CPU := cortex-a53
SEL4CP_SDK := ./sel4cp_sdk

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
AS := $(TOOLCHAIN)-as
SEL4CP_TOOL := $(SEL4CP_SDK)/bin/sel4cp
BOARD_DIR := $(SEL4CP_SDK)/board/$(BOARD)/$(SEL4CP_CONFIG)

CFLAGS := -mcpu=$(CPU) -mstrict-align -nostdlib -ffreestanding -g3 -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -Werror -I$(BOARD_DIR)/include -DBOARD_$(BOARD)
LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lsel4cp -Tsel4cp.ld

IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt


all: directories $(BUILD_DIR)/hello_world.elf system.img

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

$(BUILD_DIR)/%.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/hello_world.elf: $(BUILD_DIR)/hello_world.o
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
	
system.img: $(BUILD_DIR)/hello_world.elf configuration.system
	$(SEL4CP_TOOL) configuration.system --search-path $(BUILD_DIR) --board $(BOARD) --config $(SEL4CP_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

run: $(IMAGE_FILE)
	qemu-system-aarch64 -machine virt -cpu $(CPU) -serial mon:stdio -device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 -m size=1G -nographic
