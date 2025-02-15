import os
import re
import shutil
import struct
import subprocess
import sys

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

def run_armips(asm_file):
    # Define paths
    armips_exe = os.path.join('tools', 'armips') 
    
    # Validate files exist
    if not os.path.isfile(armips_exe):
        print(f"Error: armips not found at {armips_exe}")
        return False
        
    if not os.path.isfile(asm_file):
        print(f"Error: assembly file not found at {asm_file}")
        return False

    # Ensure executable permission
    os.chmod(armips_exe, 0o755)
    
    try:
        result = subprocess.run(
            [armips_exe, asm_file],
            check=True,
            capture_output=True,
            text=True
        )
        
        if result.stdout:
            print(result.stdout)
            
        return True
        
    except subprocess.CalledProcessError as e:
        print(f"Execution failed: {e.stderr}")
        return False
        
    except Exception as e:
        print(f"Error occurred: {e}")
        return False

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
    
    patch_util = PatchUtil(base_rom_path)
    patch_util.add_overlay_entry(overlay_elf, overlay_id, overlay_addr)
    patch_util.modify_arena_lo(overlay_elf, overlay_addr)
    patch_util.save_arm9_binary(os.path.join('rom','arm9.bin'))
    patch_util.save_overlay_table(os.path.join('rom','overlay_table.bin'))
    
    run_armips(os.path.join('overlay_ldr', 'patch.S'))
    
    dest_dir = os.path.join('rom', 'overlay')
    shutil.copy2(overlay_bin, os.path.join(dest_dir, os.path.basename(overlay_bin)))
    
