"""Source ownership for the scaffolded sibling executables."""
from pathlib import Path
import re

try:
    from tools import source_units as SU
except ModuleNotFoundError:
    import source_units as SU


# MENU and ENDING's game/engine spans end at AdtSelect; Exec starts SDK/CRT.
# See docs/build-system.md for the binary comparison and call-target audit.
SDK_STARTS = {"menu.exe": 0x80027824, "ending.exe": 0x8001DFC8}


def code_buckets(target, functions, matched):
    """Split reviewed targets without changing their whole-image totals."""
    if target not in SDK_STARTS:
        return {}
    boundary = SDK_STARTS[target]
    buckets = {
        name: {"fns": 0, "fns_matched": 0, "bytes": 0, "bytes_matched": 0}
        for name in ("game", "sdk")
    }
    for address, size, name in functions:
        if address < boundary < address + size:
            raise ValueError(f"{target}: {name} crosses the game/SDK boundary")
        bucket = buckets["game" if address < boundary else "sdk"]
        bucket["fns"] += 1
        bucket["bytes"] += size
        if address in matched:
            bucket["fns_matched"] += 1
            bucket["bytes_matched"] += size
    return buckets


def textbin_bytes(config):
    """Count raw spans, including boundaries supplied by inline rodata entries."""
    segments = []
    for line in config.splitlines():
        match = re.match(
            r"\s+- \[(0x[\da-fA-F]+)(?:,\s*(\w+)[^\]]*)?\]", line
        )
        if match:
            segments.append((int(match[1], 16), match[2]))
            continue
        match = re.match(
            r"\s+- \{\s*start:\s*(0x[\da-fA-F]+),\s*type:\s*([\w.]+)", line
        )
        if match:
            segments.append((int(match[1], 16), match[2]))
    return sum(
        next_address - address
        for (address, kind), (next_address, _) in zip(segments, segments[1:])
        if kind == "textbin"
    )


def load_target_units(target, root=Path(".")):
    """Return function rows, C-owned addresses, and address-to-source paths.

    Each executable has its own translation-unit manifest. A shared source
    wrapper only counts when its included implementation exists and has no
    assembly fallback for the function being reported.
    """
    root = Path(root)
    inventory = root / f"config/functions.{target}.tsv"
    if not inventory.exists():
        return None
    functions = []
    for line in inventory.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        address, size, name = line.split("\t")
        functions.append((int(address, 16), int(size), name))
    symbols = {
        name: int(address, 16)
        for name, address in re.findall(
            r"^([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;",
            (root / f"config/symbols.{target}.txt").read_text(), re.M
        )
    }
    for address, _, name in functions:
        symbols.setdefault(name, address)
    carved = set(re.findall(
        r"^\s+- \[0x[0-9A-Fa-f]+,\s*c,\s*(\S+)\]",
        (root / f"config/splat.{target}.yaml").read_text(), re.M
    ))
    source_dir = root / "src" / target
    manifest = root / f"config/translation-units.{target}.json"
    matched, sources = set(), {}
    if source_dir.exists():
        for name, path, unit in SU.iter_function_sources(source_dir, manifest):
            if unit.stem not in carved or name not in symbols:
                continue
            if SU.source_has_asm_fallback(path, name):
                continue
            matched.add(symbols[name])
            sources[symbols[name]] = str(path)
    return functions, matched, sources
