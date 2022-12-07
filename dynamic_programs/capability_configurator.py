# The parsing of the XML system configuration is inspired
# by the implementation in the seL4CP library: https://github.com/TobiasNissen/sel4cp/blob/main/tool/sel4coreplat/sysxml.py

import sys
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
    page_size: int = PAGE_SIZE # in bytes
    
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
    page_size = int(get_attribute_or_default(memory_region_xml, "page_size", str(PAGE_SIZE)), 0)
    
    return MemoryRegion(name, size, page_size)
    
    
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


if __name__ == "__main__":
    num_args = len(sys.argv)
    if num_args != 3:
        print(f"Usage: python3 capability_configurator.py <system_configuration_file> <target_ELF_file>")
    
    system_file = Path(sys.argv[1])
    target_elf = Path(sys.argv[2])
    
    system_description = parse_system(system_file)
    
    for pd in system_description.protection_domains:
        print(pd)
     
    
