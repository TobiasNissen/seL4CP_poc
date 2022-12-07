def ceil_div(a: int, b: int) -> int:
    """
        Returns the result of doing ceiling division of a with b.
    """
    return -(a // -b)
