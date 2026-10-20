"""Source ownership for the scaffolded sibling executables."""
from pathlib import Path
import re

try:
    from tools import source_units as SU
except ModuleNotFoundError:
    import source_units as SU


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
