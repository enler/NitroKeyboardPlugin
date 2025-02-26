import os
import re
import shutil
import struct

from script.patch_util import PatchUtil

def read_config_mk(file_path):
    config = {}
    pattern = r'([A-Z_]+)\s*:=\s*(.+)$'
    
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line:
                match = re.match(pattern, line)
                if match:
                    key, value = match.groups()
                    
                    if value.startswith('0x'):
                        config[key] = int(value, 16)
                    
                    elif value.isdigit():
                        config[key] = int(value)
                    else:
                        config[key] = value.strip()
                        
    return config
    

if __name__ == '__main__':
    config_mk_path = 'common/config.mk'
    base_rom_path = 'rom/base_rom.nds'
    overlay_ldr_elf = 'overlay_ldr.elf'
    overlay_ldr_bin = 'overlay_ldr.bin'
    config = read_config_mk(config_mk_path)
    overlay_id = config['OVERLAY_ID']
    overlay_elf = config['OVERLAY_NAME'] + '.elf'
    overlay_bin = config['OVERLAY_NAME'] + '.bin'
    overlay_addr = config['OVERLAY_ADDR']
    overlay_ldr_addr = config['OVERLAY_LDR_ADDR']
    inject_overlay_id = config['INJECT_OVERLAY_ID']
    
    patch_util = PatchUtil(base_rom_path)
    patch_util.add_overlay_entry(overlay_elf, overlay_id, overlay_addr)
    patch_util.modify_overlay_init_functions(inject_overlay_id, overlay_ldr_elf)
    patch_util.modify_arena_lo(overlay_elf, overlay_addr)
    patch_util.modify_arena_lo(overlay_elf, overlay_addr, patch_twl=True)
    patch_util.inject_overlay_ldr(overlay_ldr_bin, overlay_ldr_addr)
    patch_util.patch_word(0x0200C538, overlay_id + 1)
    patch_util.save_arm9_binary(os.path.join('rom','arm9.bin'))
    patch_util.save_overlay_table(os.path.join('rom','overlay_table.bin'))
    
    dest_dir = os.path.join('rom', 'overlay')
    shutil.copy2(overlay_bin, os.path.join(dest_dir, os.path.basename(overlay_bin)))
    
        
    
        