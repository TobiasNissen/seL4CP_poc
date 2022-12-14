import sys
sys.modules['_elementtree'] = None
import xml.etree.ElementTree as ET

from pathlib import Path
from typing import Optional

from utilities import ceil_div
from system_parser import parse_system
from system_description import MemoryRegion, Channel, Map, Irq, ProtectionDomain, SystemDescription
from xml_utilities import LineNumberingParser, InvalidSystemFormat, InvalidXmlElement, MissingAttribute, checked_lookup, get_attribute_or_default, get_int_in_range

from protection_model.base.access_right import AccessRight
from protection_model.sel4cp.access_rights import SchedulingAccessRight, ChannelAccessRight, MemoryRegionAccessRight, IrqAccessRight
from protection_model.sel4cp.constants import EXECUTABLE_FLAG, WRITABLE_FLAG, READABLE_FLAG, PAGE_SIZE


def get_loader_pd(element: ET.Element, protection_domains: list[ProtectionDomain]) -> ProtectionDomain:
    loader_pd_name = checked_lookup(element, "loader_pd")
    loader_pd = next((pd for pd in protection_domains if pd.name == loader_pd_name), None)
    
    if loader_pd is None:
        raise InvalidXmlElement(element, "No protection domain with name '{loader_pd_name}' exists in the provided system description")
        
    return loader_pd


def parse_scheduling_access_right(element: ET.Element, loader_pd: ProtectionDomain):
    priority = get_int_in_range(element, "priority", 0, loader_pd.priority, loader_pd.priority)
    mcp = get_int_in_range(element, "mcp", 0, 254, priority)
    budget = get_int_in_range(element, "budget", 0, (1 << 64) - 1, 1000)
    period = get_int_in_range(element, "period", 0, (1 << 64) - 1, budget)
    
    return SchedulingAccessRight(priority, mcp, budget, period)
    

def get_perms(element: ET.Element) -> int:
    perms_str = checked_lookup(element, "perms")
    
    perms = 0
    for perm in perms_str:
        if perm == 'r':
            perms |= READABLE_FLAG
        elif perm == 'w':
            perms |= WRITABLE_FLAG
        elif perm == 'x':
            perms |= EXECUTABLE_FLAG
        else:
            raise InvalidXmlElement(element, "The permission '{perm}' is not a valid permission. Valid permissions are 'r', 'w', and 'x'")
            
    return perms


def parse_memory_region_access_right(element: ET.Element, maps: list[Map], memory_regions: dict[str, MemoryRegion]) -> MemoryRegionAccessRight:
    target_region_name = checked_lookup(element, "name")
    
    target_map = None
    memory_region_page_cap_index = 0
    for memory_map in maps:
        if memory_map.mr == target_region_name:
            target_map = memory_map
            break
        current_memory_region = memory_regions[memory_map.mr]
        memory_region_page_cap_index += ceil_div(current_memory_region.size, PAGE_SIZE) 
    
    if target_map is None:
        raise InvalidXmlElement(element, "The protection domain marked as the loader does not have access to a memory region with the name '{target_region_name}'")
    
    target_memory_region = memory_regions[target_map.mr] 
    size = target_memory_region.size
   
    vaddr = get_int_in_range(element, "vaddr", 0, (1 << 64) - 1)
    perms = get_perms(element)
    cached_str = get_attribute_or_default(element, "cached", "True")
    try:
        cached = bool(cached_str)
    except ValueError:
        raise InvalidXmlElement(element, "The attribute 'cached' is not a valid boolean value. Valid values are: 'True' and 'False'")
        
    return MemoryRegionAccessRight(memory_region_page_cap_index, vaddr, size, perms, cached)
    
    
def parse_channel_access_right(element: ET.Element, protection_domains: dict[str, ProtectionDomain]) -> ChannelAccessRight:
    target_pd_name = checked_lookup(element, "target_pd")
    if target_pd_name not in protection_domains:
        raise InvalidXmlException(element, "No protection domain with the name '{target_pd_name}' exists in the given system")
    
    target_pd = protection_domains[target_pd_name]
    
    target_pd_channel_id = get_int_in_range(element, "target_pd_channel_id", 0, 62)
    own_pd_channel_id = get_int_in_range(element, "own_pd_channel_id", 0, 62)
    
    return ChannelAccessRight(target_pd.pd_id, target_pd_channel_id, own_pd_channel_id)
    
    
def parse_irq_access_right(element: ET.Element, irqs: dict[int, Irq]) -> IrqAccessRight:
    irq = get_int_in_range(element, "irq", 0, (1 << 64) - 1)
    if irq not in irqs:
        raise InvalidXmlException(element, "The targeted protection domain can not provide the program to load the capability to handle the IRQ number {irq}")
    
    target_irq = irqs[irq]
    
    own_irq_channel_id = get_int_in_range(element, "channel_id", 0, 62)
    parent_irq_channel_id = target_irq.pd_channel_id
    
    return IrqAccessRight(parent_irq_channel_id, own_irq_channel_id)


def parse_access_rights(access_rights_file: Path, system_description: SystemDescription) -> list[AccessRight]:
    """
        Parses the seL4CP XML access rights description in the file at the given path.
        NB: The access rights are validated against the given system description.
    """
    try:
        tree = ET.parse(access_rights_file, parser=LineNumberingParser(access_rights_file))
    except ET.ParseError as e:
        line, column = e.position
        raise InvalidSystemFormat(line, column)
        
    root = tree.getroot()
    loader_pd = get_loader_pd(root, system_description.protection_domains)
    
    memory_regions = { memory_region.name: memory_region for memory_region in system_description.memory_regions }
    channel_protection_domains = { pd.name: pd for pd in system_description.protection_domains if pd.parent_pd_id == loader_pd.pd_id or pd.pd_id == loader_pd.pd_id}
    irqs = { irq.irq: irq for irq in loader_pd.irqs }
    
    access_rights = []
    for child in root:
        if child.tag == "scheduling":
            access_rights.append(
                parse_scheduling_access_right(child, loader_pd)
            )
        elif child.tag == "memory_region":
            access_rights.append(
                parse_memory_region_access_right(child, loader_pd.maps, memory_regions)
            )
        elif child.tag == "channel":
            access_rights.append(
                parse_channel_access_right(child, channel_protection_domains)
            )
        elif child.tag == "irq":
            access_rights.append(
                parse_irq_access_right(child, irqs)
            )
        else:
            raise InvalidXmlElement(child, "Invalid access right tag. Valid values are: 'scheduling', 'memory_region', 'channel', and 'irq'")
    
    return access_rights 
    
    

