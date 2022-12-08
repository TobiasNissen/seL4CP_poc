from pathlib import Path
from typing import Optional
import xml.etree.ElementTree as ET

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
        

def get_int_in_range(element: ET.Element, attribute_name: str, min_value: int, max_value: int, default_value: Optional[int] = None) -> int:
    """
        Reads the attribute with the given attribute_name from the
        given element. Ensures that the value is in the range
        [min_value; max_value]. 
        Raises an exception if a valid integer can not be extracted.
    """
    if default_value is None:
        attribute_str_value = checked_lookup(element, attribute_name)
    else:
        attribute_str_value = get_attribute_or_default(element, attribute_name, "")
        if attribute_str_value == "":
            return default_value
    try:
        attribute_value = int(attribute_str_value, 0)
        if attribute_value < min_value or attribute_value > max_value:
            raise InvalidXmlElement(element, f"The attribute '{attribute_name}' must be in the range [{min_value}; {max_value}]")
        return attribute_value
    except ValueError:
        raise InvalidXmlElement(element, f"The attribute '{attribute_name}' is not an integer")   
        
        
