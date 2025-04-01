import struct
import ndspy.rom
from elftools.elf.elffile import ELFFile
from ndspy.code import MainCodeFile
from ndspy.code import Overlay
from ndspy.rom import NintendoDSRom
from .ndspy_hotfix import NdspyHotfix

class PatchUtil:
    codefile: MainCodeFile
    overlay_table: dict[int, Overlay]
    def __init__(self, base_rom_path):
        NdspyHotfix.apply()
        rom = NintendoDSRom.fromFile(base_rom_path)
        self.codefile = rom.loadArm9()
        self.overlay_table = rom.loadArm9Overlays(rom.arm9OverlayTable)
    
    def add_overlay_entry(self, overlay_elf_path, overlay_id, overlay_addr):
        with open(overlay_elf_path, 'rb') as f:
            overlay_elf = ELFFile(f)
            symbol_table = overlay_elf.get_section_by_name('.symtab')
            if symbol_table:
                for symbol in symbol_table.iter_symbols():
                    if symbol.name == '__ram_size__':
                        overlay_ram_size = symbol['st_value']
                    elif symbol.name == '__bss_size__':
                        overlay_bss_size = symbol['st_value']
                    elif symbol.name == '__init_functions_begin__':
                        init_functions_begin = symbol['st_value']
                    elif symbol.name == '__init_functions_end__':
                        init_functions_end = symbol['st_value']
        
        if overlay_id != len(self.overlay_table):
            while overlay_id > len(overlay_table):
               self.overlay_table[len(overlay_table)] = ndspy.rom.code.Overlay()
        self.overlay_table[overlay_id] = ndspy.rom.code.Overlay(bytes(), 
                                                                overlay_addr, 
                                                                overlay_ram_size, 
                                                                overlay_bss_size, 
                                                                init_functions_begin, 
                                                                init_functions_end, 
                                                                overlay_id, 0, 0)
        
    def add_overlay_as_section(self, overlay_elf_path, overlay_bin_path, overlay_addr):
        with open(overlay_elf_path, 'rb') as f:
            overlay_elf = ELFFile(f)
            symbol_table = overlay_elf.get_section_by_name('.symtab')
            if symbol_table:
                for symbol in symbol_table.iter_symbols():
                    if symbol.name == '__ram_size__':
                        overlay_ram_size = symbol['st_value']
                    elif symbol.name == '__bss_size__':
                        overlay_bss_size = symbol['st_value']

        with open(overlay_bin_path, 'rb') as f:
            overlay_data = f.read()

        ram_buffer = bytearray(overlay_ram_size)
        ram_buffer[:len(overlay_data)] = overlay_data

        self.codefile.sections.append(ndspy.code.MainCodeFile.Section(
            data=bytes(ram_buffer),
            ramAddress=overlay_addr,
            bssSize=overlay_bss_size
        ))
                        
        
    def modify_overlay_init_functions(self, overlay_id, overlay_ldr_elf_path):
        with open(overlay_ldr_elf_path, 'rb') as f:
            overlay_ldr_elf = ELFFile(f)
            symbol_table = overlay_ldr_elf.get_section_by_name('.symtab')
            if symbol_table:
                for symbol in symbol_table.iter_symbols():
                    if symbol.name == 'OverlayStaticInitFunc':
                        overlay_ldr_static_init_func = symbol['st_value']
                        
        self.overlay_table[overlay_id].staticInitStart = overlay_ldr_static_init_func
        self.overlay_table[overlay_id].staticInitEnd = overlay_ldr_static_init_func + 4
        
    def patch_word(self, offset, value):
        for section in self.codefile.sections:
            if section.ramAddress <= offset < section.ramAddress + len(section.data):
                offset = offset - section.ramAddress
                struct.pack_into('<I', section.data, offset, value)
                break
        
    def modify_arena_lo(self, overlay_elf_path, overlay_addr, patch_twl: bool = False):
        arena_symbol = 'PtrToArenaLo_twl' if patch_twl else 'PtrToArenaLo'
        arena_lo = None
        overlay_size = None

        with open(overlay_elf_path, 'rb') as f:
            overlay_elf = ELFFile(f)
            symbol_table = overlay_elf.get_section_by_name('.symtab')
            if symbol_table:
                for symbol in symbol_table.iter_symbols():
                    if symbol.name == arena_symbol:
                        arena_lo = symbol['st_value']
                    elif symbol.name == '__size__':
                        overlay_size = symbol['st_value']
        
        if arena_lo is None or overlay_size is None:
            raise ValueError(f"Required symbols {arena_symbol} or __size__ not found")

        for section in self.codefile.sections:
            if section.ramAddress <= arena_lo < section.ramAddress + len(section.data):
                offset = arena_lo - section.ramAddress
                struct.pack_into('<I', section.data, offset, (overlay_addr + overlay_size + 0xF) & ~0xF)
                break
            
    def inject_overlay_ldr(self, overlay_ldr_bin_path, overlay_ldr_addr):
        for section in self.codefile.sections:
            if section.ramAddress <= overlay_ldr_addr < section.ramAddress + len(section.data):
                offset = overlay_ldr_addr - section.ramAddress
                buff = open(overlay_ldr_bin_path, 'rb').read()
                section.data = section.data[:offset] + buff + section.data[offset + len(buff):]
                break
            
    def reload_arm9_binary(self, arm9_path):
        self.codefile = MainCodeFile.fromFile(arm9_path, self.codefile.ramAddress)
        
    def save_arm9_binary(self, output_path, skip_compression: bool = False):
        require_compressed = False
        if self.codefile.codeSettingsOffs is not None:
            require_compressed = struct.unpack_from('<I', self.codefile.sections[0].data, self.codefile.codeSettingsOffs + 0x14)[0] != 0
        if skip_compression:
            require_compressed = False
        codefile_bytes = self.codefile.save(compress=require_compressed)
        with open(output_path, 'wb') as f:
            f.write(codefile_bytes)
            
    def save_overlay_table(self, output_path):
        overlay_table_bytes = ndspy.rom.code.saveOverlayTable(self.overlay_table)
        with open(output_path, 'wb') as f:
            f.write(overlay_table_bytes)