#!/usr/bin/env python3
import argparse
import os
import struct
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

R_ARM_ABS32 = 2
R_ARM_TARGET1 = 38
R_ARM_TARGET2 = 41


def get_symbol_value(elf, name):
    symtab = elf.get_section_by_name(".symtab")
    if not symtab:
        raise RuntimeError("missing .symtab")
    for sym in symtab.iter_symbols():
        if sym.name == name:
            return sym["st_value"]
    raise RuntimeError(f"missing symbol {name}")


def is_alloc_section(section):
    return (section["sh_flags"] & 0x2) != 0


def is_nobits_section(section):
    return section["sh_type"] == "SHT_NOBITS"


def collect_image(elf, link_address, image_size):
    image = bytearray(image_size)
    for section in elf.iter_sections():
        if not is_alloc_section(section) or is_nobits_section(section):
            continue
        start = section["sh_addr"] - link_address
        data = section.data()
        if start < 0 or start + len(data) > image_size:
            raise RuntimeError(f"section {section.name} is outside the load image")
        image[start:start + len(data)] = data
    return image


def is_internal_symbol(elf, sym):
    shndx = sym["st_shndx"]
    if shndx in ("SHN_UNDEF", "SHN_ABS", "SHN_COMMON"):
        return False
    if not isinstance(shndx, int):
        return False
    section = elf.get_section(shndx)
    return section is not None and is_alloc_section(section)


def collect_relocs(elf, link_address, image_size):
    relocs = set()
    symtabs = {}

    for section in elf.iter_sections():
        if not isinstance(section, RelocationSection):
            continue

        target = elf.get_section(section["sh_info"])
        if target is None or not is_alloc_section(target):
            continue

        symtab_index = section["sh_link"]
        if symtab_index not in symtabs:
            symtabs[symtab_index] = elf.get_section(symtab_index)
        symtab = symtabs[symtab_index]

        for reloc in section.iter_relocations():
            reloc_type = reloc["r_info_type"]
            offset = target["sh_addr"] + reloc["r_offset"] - link_address
            if offset < 0 or offset + 4 > image_size:
                continue

            sym_index = reloc["r_info_sym"]
            if sym_index == 0:
                continue

            sym = symtab.get_symbol(sym_index)
            if reloc_type in (R_ARM_ABS32, R_ARM_TARGET1, R_ARM_TARGET2):
                if is_internal_symbol(elf, sym):
                    relocs.add(offset)

    return sorted(relocs)


def write_u32(image, link_address, symbol, value):
    offset = symbol - link_address
    if offset < 0 or offset + 4 > len(image):
        raise RuntimeError("symbol patch target is outside the load image")
    image[offset:offset + 4] = struct.pack("<I", value)


def main():
    parser = argparse.ArgumentParser(description="Create a tiny relocatable keyboard module image.")
    parser.add_argument("elf")
    parser.add_argument("output")
    parser.add_argument("--link-address-symbol", default="KeyboardModule_LinkAddress")
    parser.add_argument("--reloc-offset-symbol", default="KeyboardModule_RelocOffset")
    parser.add_argument("--reloc-count-symbol", default="KeyboardModule_RelocCount")
    parser.add_argument("--module-size-symbol", default="KeyboardModule_ModuleSize")
    args = parser.parse_args()

    with open(args.elf, "rb") as f:
        elf = ELFFile(f)
        link_address = get_symbol_value(elf, "__module_link_address__")
        image_size = get_symbol_value(elf, "__module_image_size__")
        bss_size = get_symbol_value(elf, "__module_bss_size__")
        link_address_symbol = get_symbol_value(elf, args.link_address_symbol)
        reloc_offset_symbol = get_symbol_value(elf, args.reloc_offset_symbol)
        reloc_count_symbol = get_symbol_value(elf, args.reloc_count_symbol)
        module_size_symbol = get_symbol_value(elf, args.module_size_symbol)

        image = collect_image(elf, link_address, image_size)
        relocs = collect_relocs(elf, link_address, image_size)

    reloc_offset = image_size + bss_size
    module_size = reloc_offset + len(relocs) * 4
    write_u32(image, link_address, link_address_symbol, link_address)
    write_u32(image, link_address, reloc_offset_symbol, reloc_offset)
    write_u32(image, link_address, reloc_count_symbol, len(relocs))
    write_u32(image, link_address, module_size_symbol, module_size)

    output_dir = os.path.dirname(args.output)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    with open(args.output, "wb") as f:
        f.write(image)
        f.write(bytes(bss_size))
        for reloc in relocs:
            f.write(struct.pack("<I", reloc))

    print(f"{args.output}: image={image_size} bss={bss_size} relocs={len(relocs)} file={module_size}")


if __name__ == "__main__":
    main()
