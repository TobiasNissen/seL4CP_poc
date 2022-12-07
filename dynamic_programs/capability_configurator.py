# The parsing of the XML system configuration is inspired
# by the implementation in the seL4CP library: https://github.com/TobiasNissen/sel4cp/blob/main/tool/sel4coreplat/sysxml.py

import sys
import os
from pathlib import Path
from dataclasses import dataclass
from typing import Optional
from enum import Enum

sys.modules['_elementtree'] = None
import xml.etree.ElementTree as ET

PAGE_SIZE = 0x1000 # 4 KiB
EXECUTABLE_FLAG = 1
WRITABLE_FLAG = 2
READABLE_FLAG = 4

class LineNumberingParser(ET.XMLParser):
    def __init__(self, path: Path):
        super().__init__()
        self._path = path

    def _start(self, *args, **kwargs): # type: ignore
        element = super(self.__class__, self)._start(*args, **kwargs)
        element._path = self._path
        element._start_line_number = self.parser.CurrentLineNumber
        element._start_column_number = self.parser.CurrentColumnNumber
        element._loc_str = f"{element._path}:{element._start_line_number}.{element._start_column_number}"
        return element


class InvalidSystemFormat(Exception):
    def __init__(self, line: int, column: int):
        super().__init__(f"Invalid XML system configuration: line={line}, column={column}")
        
class InvalidXmlElement(Exception):
    def __init__(self, element: ET.Element, reason: str):
        super().__init__(f"Invalid XML element with tag '{element.tag}' at {element._loc_str}. Reason: {reason}")

class MissingAttribute(Exception):
    def __init__(self, attr: str, element: ET.Element):
        super().__init__(f"Missing attribute '{attr}' for XML element at {element._loc_str}")


@dataclass(frozen=True, eq=True)
class MemoryRegion:
    name: str
    size: int # in bytes
    
@dataclass(frozen=True, eq=True)
class Channel:
    pd_a_name: str
    channel_id_a: int
    pd_b_name: str
    channel_id_b: int

@dataclass(frozen=True, eq=True)
class Map:
    mr: str # the name of the targeted memory region
    vaddr: int
    perms: int # a combination of r (read), w (write), and x (execute)
    cached: bool
    
@dataclass(frozen=True, eq=True)
class Irq:
    irq: int # the hardware interrupt number
    pd_channel_id: int # the channel id used for interrupts.

@dataclass(frozen=True, eq=True)
class ProtectionDomain:
    pd_id: int
    parent_pd_id: Optional[int]
    name: str
    priority: int
    budget: int
    period: int
    maps: list[Map]
    irqs: list[Irq]
    
@dataclass(frozen=True, eq=True)
class SystemDescription: 
    # all lists are stored in the order the elements are encountered by a DFS of the XML system configuration.
    protection_domains: list[ProtectionDomain]
    memory_regions: list[MemoryRegion]
    channels: list[Channel]



def ceil_div(a: int, b: int) -> int:
    """
        Returns the result of doing ceiling division of a with b.
    """
    return -(a // -b)

def checked_lookup(element: ET.Element, attr: str) -> str:
    """
        Returns the attribute with the given name on the given element.
        Raises a MissingAttribute exception if no such attribute exists.
    """
    try:
        return element.attrib[attr]
    except KeyError:
        raise MissingAttribute(attr, element)
        
def get_attribute_or_default(element: ET.Element, attr: str, default: str) -> str:
    """
        Returns the attribute with the given name on the given element,
        if it exists.
        Otherwise, the given default value is returned
    """
    if attr in element.attrib:
        return element.attrib[attr]
    else:
        return default

def parse_memory_region(memory_region_xml: ET.Element) -> MemoryRegion:
    name = checked_lookup(memory_region_xml, "name")
    size = int(checked_lookup(memory_region_xml, "size"), 0)
    return MemoryRegion(name, size)
    
    
def parse_channel(channel_xml: ET.Element) -> Channel:
    if len(channel_xml) != 2:
        raise InvalidXmlElement(channel_xml, "The channel does not have exactly two ends")
        
    end_a = channel_xml[0]
    end_b = channel_xml[1]
    
    if end_a.tag != "end":
        raise InvalidXmlElement(end_a, "Expected 'end' tag")
    elif end_b.tag != "end":
        raise InvalidXmlElement(end_b, "Expected 'end' tag")
    
    pd_a_name = checked_lookup(end_a, "pd")
    channel_id_a = checked_lookup(end_a, "id")
    pd_b_name = checked_lookup(end_b, "pd")
    channel_id_b = checked_lookup(end_b, "id")
    
    return Channel(pd_a_name, channel_id_a, pd_b_name, channel_id_b)
    
    
def parse_map(map_xml: ET.Element) -> Map:
    mr = checked_lookup(map_xml, "mr")
    vaddr = int(checked_lookup(map_xml, "vaddr"), 0)
    perms_str = checked_lookup(map_xml, "perms")
    cached = bool(get_attribute_or_default(map_xml, "cached", "True"))
    
    perms = 0
    for perm in perms_str:
        if perm == 'x':
            perms |= EXECUTABLE_FLAG
        elif perm == 'w':
            perms |= WRITABLE_FLAG
        elif perm == 'r':
            perms |= READABLE_FLAG
        else:
            raise InvalidXmlElement(map_xml, f"The permission '{perm}' is not valid. Valid values are 'r', 'w', and 'x'")
    
    return Map(mr, vaddr, perms, cached)
    
    
def parse_irq(irq_xml: ET.Element) -> Irq:
    irq = int(checked_lookup(irq_xml, "irq"))
    pd_channel_id = int(checked_lookup(irq_xml, "id")) 
    return Irq(irq, pd_channel_id)

    
def parse_protection_domain(protection_domain_xml: ET.Element, parent_pd_id: Optional[int] = None) -> list[ProtectionDomain]:
    """
        Parses the given protection domain.
        Returns a list containing the parsed protection domain
        and all nested protection domains.
    """
    pd_id = int(checked_lookup(protection_domain_xml, "pd_id"))
    name = checked_lookup(protection_domain_xml, "name")
    priority = int(checked_lookup(protection_domain_xml, "priority"))
    budget = int(get_attribute_or_default(protection_domain_xml, "budget", "1000"))
    period = int(get_attribute_or_default(protection_domain_xml, "period", str(budget)))
    
    child_pds = []
    maps = []
    irqs = []
    for child in protection_domain_xml:
        if child.tag == "protection_domain":
            child_pds.extend(parse_protection_domain(child, pd_id))
        elif child.tag == "map":
            maps.append(parse_map(child))
        elif child.tag == "irq":
            irqs.append(parse_irq(child))
        elif child.tag == "program_image":
            continue # we ignore the program image
        else:
            raise InvalidXmlElement(child, "Invalid tag for the child of a protection domain")
    
    current_pd = ProtectionDomain(pd_id, parent_pd_id, name, priority, budget, period, maps, irqs)
    return [current_pd] + child_pds
    

def parse_system(system_file: Path) -> SystemDescription:
    """
        Parses the seL4CP XML system description in the file at the
        given path.
        NB: The XML system description is not validated.
            The validation is assumed to have been performed by 
            the sel4cp tool.
    """
    try:
        tree = ET.parse(system_file, parser=LineNumberingParser(system_file))
    except ET.ParseError as e:
        line, column = e.position
        raise InvalidSystemFormat(line, column)
        
    root = tree.getroot()
    memory_regions = []
    channels = []
    protection_domains = []
    for child in root:
        if child.tag == "memory_region":
            memory_regions.append(parse_memory_region(child))
        elif child.tag == "channel":
            channels.append(parse_channel(child))
        elif child.tag == "protection_domain":
            protection_domains.extend(parse_protection_domain(child))
        else:
            raise InvalidXmlElement(child, "Invalid tag")
    
    return SystemDescription(protection_domains, memory_regions, channels)


class AccessRight:
    type_id: int
    
class SchedulingAccessRight(AccessRight):
    priority: int
    budget: int
    period: int
    
    def __init__(self, priority: int, budget: int, period: int):
        self.type_id = 0
        self.priority = priority
        self.budget = budget
        self.period = period
    
    
class ChannelAccessRight(AccessRight):
    target_pd_id: int           # the id of the targeted PD.
    target_pd_channel_id: int   # the ID used by the targeted PD for the channel
    own_channel_id: int         # the ID used by the current PD for the channel.
    
    def __init__(self, target_pd_id: int, target_pd_channel_id: int, own_channel_id: int):
        self.type_id = 1
        self.target_pd_id = target_pd_id
        self.target_pd_channel_id = target_pd_channel_id
        self.own_channel_id = own_channel_id


class MemoryRegionAccessRight(AccessRight):
    memory_region_page_cap_index: int
    vaddr: int
    size: int
    perms: int
    cached: bool
    
    def __init__(self, memory_region_page_cap_index: int, vaddr: int, size: int, perms: int, cached: bool):
        self.type_id = 2
        self.memory_region_page_cap_index = memory_region_page_cap_index
        self.vaddr = vaddr
        self.size = size
        self.perms = perms
        self.cached = cached


class IrqAccessRight(AccessRight):
    parent_irq_channel_id: int
    own_irq_channel_id: int
    
    def __init__(self, parent_irq_channel_id: int, own_irq_channel_id: int):
        self.type_id = 3
        self.parent_irq_channel_id = parent_irq_channel_id
        self.own_irq_channel_id = own_irq_channel_id



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
    
    for access_right in access_rights:
        print(access_right)
     
    
