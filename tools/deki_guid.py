"""Deki's deterministic GUIDs (deki-engine/src/Guid.cpp), for baked font variants."""
M = (1 << 64) - 1


def deterministic_guid(seed: str) -> str:
    h1 = h2 = 14695981039346656037
    for i, ch in enumerate(seed.encode("utf-8")):
        if i % 2 == 0:
            h1 = ((h1 ^ ch) * 1099511628211) & M
        else:
            h2 = ((h2 ^ ch) * 1099511628211) & M
    return (f"{h1 >> 32:08x}-{(h1 >> 16) & 0xFFFF:04x}-{h1 & 0xFFFF:04x}-"
            f"{(h2 >> 48) & 0xFFFF:04x}-{(h2 >> 16) & 0xFFFFFFFF:08x}{h2 & 0xFFFF:04x}")


def font_variant(source_guid: str, size: int):
    return deterministic_guid(f"{source_guid}:{size}"), deterministic_guid(f"{source_guid}:{size}:atlas")
