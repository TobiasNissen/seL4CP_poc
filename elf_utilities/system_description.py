from dataclasses import dataclass
from typing import Optional

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
    # all list elements are stored in the order the elements are encountered by a DFS of the XML system configuration.
    protection_domains: list[ProtectionDomain]
    memory_regions: list[MemoryRegion]
    channels: list[Channel]


