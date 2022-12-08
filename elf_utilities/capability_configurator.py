import sys
from pathlib import Path
from dataclasses import dataclass
from typing import Optional

from system_parser import parse_system, MemoryRegion, Channel, Map, Irq, ProtectionDomain, SystemDescription
from utilities import ceil_div
from access_rights import AccessRight, SchedulingAccessRight, ChannelAccessRight, MemoryRegionAccessRight, IrqAccessRight
from elf_patcher import patch_elf

PAGE_SIZE = 0x1000 # 4 KiB

EXECUTABLE_FLAG = 1
WRITABLE_FLAG = 2
READABLE_FLAG = 4

def get_int_in_range(parameter_name: str, min_value: int, max_value: int, default_value: Optional[int] = None, is_bool: bool = False) -> int:
    """
        Prompts the user for an integer value for the parameter with the given name.
        If the user doesn't provide any value and a default value is provided, the default value is used.
        It is ensured that a value provided by the user in in the range [min_value, max_value].
    """
    while True:
        try:
            if is_bool:
                input_str = f"If {parameter_name} input 1, otherwise input 0"
            else:
                input_str = f"Input the {parameter_name}"
            if default_value is not None:
                input_str += f" (default value {default_value})"
            input_str += ": "
            parameter_input = input(input_str)
            if parameter_input == "":
                if default_value is not None:
                    result = default_value
                    break
                else:
                    print("Please provide a value")
                    continue
            else:
                result = int(parameter_input, 0)
                if result < min_value or result > max_value:
                    print(f"The {parameter_name} must be in the range [{min_value}; {max_value}]")
                    continue   
                else:
                    break
        except ValueError:
            print(f"Invalid value, please try again.")
    return result


def get_target_protection_domain(system_description: SystemDescription) -> ProtectionDomain:
    print("The following protection domains are available to load the program from:")
    for i, pd in enumerate(system_description.protection_domains):
        print(f" {i}) {pd.name}")
        
    pd_option = get_int_in_range("protection domain to load the program from", 0, len(system_description.protection_domains) - 1)
    
    return system_description.protection_domains[pd_option]
    

def get_scheduling_access_right(target_pd: ProtectionDomain) -> SchedulingAccessRight:
    priority = get_int_in_range("priority", 0, target_pd.priority, target_pd.priority)
    budget = get_int_in_range("budget", 0, (1 << 64) - 1, target_pd.budget)
    period = get_int_in_range("period", budget, (1 << 64) - 1, budget)
    return SchedulingAccessRight(priority, budget, period)
    

def get_channel_access_right(protection_domains: list[ProtectionDomain]) -> ChannelAccessRight:
    print("A channel can be set up to the following protection domains:")
    for i, pd in enumerate(protection_domains):
        print(f" {i}) {pd.name}")
    pd_option = get_int_in_range("channel option", 0, len(protection_domains) - 1)    
    target_pd = protection_domains[pd_option]
    
    target_pd_channel_id = get_int_in_range("id of the channel for the selected protection domain", 0, 62)
    own_pd_channel_id = get_int_in_range("id of the channel for the protection domain of the program to load", 0, 62)
    
    return ChannelAccessRight(target_pd.pd_id, target_pd_channel_id, own_pd_channel_id)
    
    
def get_perms() -> int:
    readable = get_int_in_range("readable", 0, 1, 1, True)
    writable = get_int_in_range("writable", 0, 1, 0, True)
    executable = get_int_in_range("executable", 0, 1, 0, True)
    
    perms = 0
    if readable == 1:
        perms |= READABLE_FLAG
    if writable == 1:
        perms |= WRITABLE_FLAG
    if executable == 1:
        perms |= EXECUTABLE_FLAG
    return perms
    
    
def get_memory_region_access_right(maps: list[Map], memory_regions: dict[str, MemoryRegion]) -> MemoryRegionAccessRight:
    print("The following shared memory regions can be made available to the program to load:")
    for i, memory_map in enumerate(maps):
        print(f" {i}) {memory_map.mr}")
    map_option = get_int_in_range("memory region option", 0, len(maps) - 1)
    target_map = maps[map_option]
    if target_map.mr not in memory_regions:
        raise Exception(f"Failed to find a memory region with name {target_map.mr}; this should not be possible!")
    target_memory_region = memory_regions[target_map.mr]
    
    # Calculate the relative index of the CSlot for the first page capability.
    memory_region_page_cap_index = 0
    for memory_map in maps[0:map_option]:
        current_memory_region = memory_regions[memory_map.mr]
        memory_region_page_cap_index += ceil_div(current_memory_region.size, PAGE_SIZE) 
    size = target_memory_region.size
    
    vaddr = get_int_in_range("vaddr", 0, (1 << 64) - 1)
    perms = get_perms()
    cached = get_int_in_range("cached", 0, 1, 1, True)
    
    return MemoryRegionAccessRight(memory_region_page_cap_index, vaddr, size, perms, cached)


def get_irq_access_right(irqs: list[Irq]) -> IrqAccessRight:
    print("The following IRQ numbers can be chosen:")
    for i, irq in enumerate(irqs):
        print(f" {i}) {irq.irq}")
    irq_option = get_int_in_range("IRQ option", 0, len(irqs) - 1)
    target_irq = irqs[irq_option]
    
    own_irq_channel_id = get_int_in_range("IRQ channel id for the program to load", 0, 62)
    parent_irq_channel_id = target_irq.pd_channel_id
    
    return IrqAccessRight(parent_irq_channel_id, own_irq_channel_id)



def get_access_rights(system_description: SystemDescription) -> list[AccessRight]:
    target_pd = get_target_protection_domain(system_description)
    child_pds = [pd for pd in system_description.protection_domains if pd.parent_pd_id == target_pd.pd_id]
    memory_regions = { memory_region.name: memory_region for memory_region in system_description.memory_regions }
    
    scheduling_access_right = get_scheduling_access_right(target_pd)
    
    channel_access_rights = []
    memory_region_access_rights = []
    irq_access_rights = []
    while True:
        print()
        print("The following options are available:")
        print(" 0) Add a channel access right")
        print(" 1) Add a memory region access right")
        print(" 2) Add an IRQ access right")
        print(" 3) Finish adding access rights")
        
        option = get_int_in_range("option to choose", 0, 3)
        print()
        if option == 0:
            channel_access_rights.append(
                get_channel_access_right([target_pd] + child_pds)
            )
        elif option == 1:
            memory_region_access_rights.append(
                get_memory_region_access_right(target_pd.maps, memory_regions)
            )
        elif option == 2:
            irq_access_rights.append(
                get_irq_access_right(target_pd.irqs)
            )
        else:
            break
    
    return [scheduling_access_right] + channel_access_rights + memory_region_access_rights + irq_access_rights
    
    

if __name__ == "__main__":
    num_args = len(sys.argv)
    if num_args != 3:
        print(f"Usage: python3 capability_configurator.py <system_configuration_file> <target_ELF_file>")
    
    system_file = Path(sys.argv[1])
    target_elf = Path(sys.argv[2])
    
    system_description = parse_system(system_file)
    
    for pd in system_description.protection_domains:
        print(pd)
        
    access_rights = get_access_rights(system_description)
    
    patch_elf(target_elf, access_rights)
     
    
