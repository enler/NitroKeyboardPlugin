#!/usr/bin/env python3
import argparse
from elftools.elf.elffile import ELFFile


def is_alloc_section(elf, shndx):
    if not isinstance(shndx, int):
        return False
    section = elf.get_section(shndx)
    if section is None:
        return False
    return (section["sh_flags"] & 0x2) != 0


def collect_all_host_symbols(elf):
    symtab = elf.get_section_by_name(".symtab")
    if not symtab:
        raise RuntimeError("missing .symtab")

    symbols = {}
    for sym in symtab.iter_symbols():
        name = sym.name
        if not name:
            continue
        if sym["st_info"]["bind"] not in ("STB_GLOBAL", "STB_WEAK"):
            continue
        if sym["st_shndx"] == "SHN_UNDEF":
            continue
        if sym["st_info"]["type"] in ("STT_FILE", "STT_SECTION"):
            continue
        if sym["st_info"]["type"] == "STT_FUNC" and sym["st_size"] == 0:
            continue
        if not is_alloc_section(elf, sym["st_shndx"]):
            continue
        symbols[name] = sym["st_value"]
    return symbols


def collect_named_symbols(elf, names):
    symtab = elf.get_section_by_name(".symtab")
    if not symtab:
        raise RuntimeError("missing .symtab")

    symbols = {sym.name: sym["st_value"] for sym in symtab.iter_symbols() if sym.name}
    missing = [name for name in names if name not in symbols]
    if missing:
        raise RuntimeError("missing symbols: " + ", ".join(missing))
    return {name: symbols[name] for name in names}


def main():
    parser = argparse.ArgumentParser(description="Export ELF symbols as an ld script.")
    parser.add_argument("elf")
    parser.add_argument("output")
    parser.add_argument("symbols", nargs="*")
    parser.add_argument("--thumb", action="store_true", help="Set bit 0 on exported function addresses.")
    args = parser.parse_args()

    with open(args.elf, "rb") as f:
        elf = ELFFile(f)
        if args.symbols:
            symbols = collect_named_symbols(elf, args.symbols)
        else:
            symbols = collect_all_host_symbols(elf)

    with open(args.output, "w") as f:
        f.write("/* Auto-generated from %s. */\n" % args.elf)
        for name in sorted(symbols):
            value = symbols[name]
            if args.thumb:
                value |= 1
            f.write("%s = 0x%08X;\n" % (name, value))


if __name__ == "__main__":
    main()
