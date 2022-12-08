import struct
from abc import ABC, abstractmethod

PAGE_SIZE = 0x1000 # 4 KiB

EXECUTABLE_FLAG = 1
WRITABLE_FLAG = 2
READABLE_FLAG = 4

# TODO: Put this into a separate file.
class AccessRight(ABC):
    type_id: int
    
    def serialize(self) -> bytes:
        return self.serialize_type_id() + self.serialize_metadata()
        
    @abstractmethod
    def serialize_type_id(self) -> bytes:
        pass
        
    @abstractmethod
    def serialize_metadata(self) -> bytes:
        pass
    
    
class SeL4CPAccessRight(AccessRight):
    def serialize_type_id(self) -> bytes:
        return struct.pack("<B", self.type_id)
    
    
class SchedulingAccessRight(SeL4CPAccessRight):
    priority: int
    budget: int
    period: int
    
    def __init__(self, priority: int, budget: int, period: int):
        self.type_id = 0
        self.priority = priority
        self.budget = budget
        self.period = period
        
    def serialize_metadata(self) -> bytes:
        return struct.pack("<B", self.priority) + \
               struct.pack("<Q", self.budget) + \
               struct.pack("<Q", self.period)
    
    
class ChannelAccessRight(SeL4CPAccessRight):
    target_pd_id: int           # the id of the targeted PD.
    target_pd_channel_id: int   # the ID used by the targeted PD for the channel
    own_channel_id: int         # the ID used by the current PD for the channel.
    
    def __init__(self, target_pd_id: int, target_pd_channel_id: int, own_channel_id: int):
        self.type_id = 1
        self.target_pd_id = target_pd_id
        self.target_pd_channel_id = target_pd_channel_id
        self.own_channel_id = own_channel_id
        
    def serialize_metadata(self) -> bytes:
        return struct.pack("<B", self.target_pd_id) + \
               struct.pack("<B", self.target_pd_channel_id) + \
               struct.pack("<B", self.own_channel_id)


class MemoryRegionAccessRight(SeL4CPAccessRight):
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
    
    def serialize_metadata(self) -> bytes:
        return struct.pack("<Q", self.memory_region_page_cap_index) + \
               struct.pack("<Q", self.vaddr) + \
               struct.pack("<Q", self.size) + \
               struct.pack("<B", self.perms) + \
               struct.pack("<B", self.cached)


class IrqAccessRight(SeL4CPAccessRight):
    parent_irq_channel_id: int
    own_irq_channel_id: int
    
    def __init__(self, parent_irq_channel_id: int, own_irq_channel_id: int):
        self.type_id = 3
        self.parent_irq_channel_id = parent_irq_channel_id
        self.own_irq_channel_id = own_irq_channel_id
        
    def serialize_metadata(self) -> bytes:
        return struct.pack("<B", self.parent_irq_channel_id) + \
               struct.pack("<B", self.own_irq_channel_id)


