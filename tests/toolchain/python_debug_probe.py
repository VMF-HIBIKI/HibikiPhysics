"""A deterministic host program for debugger and language-service acceptance."""


def calculate_total(values: list[int]) -> int:
    return sum(values)


values = [2, 3, 5]
total = calculate_total(values)
identity = {"value": total}
print(identity)
