import struct
import sys
import argparse

class FSOverlayInfoHeader:
    def __init__(self):
        self.id = 0
        self.ram_address = 0
        self.ram_size = 0
        self.bss_size = 0
        self.sinit_init = 0
        self.sinit_init_end = 0
        self.file_id = 0
        self.compressed = 0
        self.flag = 0
        
def read_overlay_table(file_name):
    overlay_table, size = load_file(file_name)
    if not overlay_table or size != (size & ~0x1F):
        return []

    overlay_entry_size = struct.calcsize('IIIIIIII')
    num_entries = size // overlay_entry_size
    overlay_entries = []

    for i in range(num_entries):
        entry_data = overlay_table[i * overlay_entry_size:(i + 1) * overlay_entry_size]
        overlay_entry = FSOverlayInfoHeader()
        (overlay_entry.id, overlay_entry.ram_address, overlay_entry.ram_size, overlay_entry.bss_size, 
         overlay_entry.sinit_init, overlay_entry.sinit_init_end, overlay_entry.file_id, flags) = struct.unpack('IIIIIIII', entry_data)
        overlay_entry.compressed = (flags >> 8) & 0xFF
        overlay_entry.flag = flags & 0xFF
        overlay_entries.append(overlay_entry)

    return overlay_entries

def save_file(fname, fbuf):
    with open(fname, 'wb') as f:
        f.write(fbuf)

def load_file(fname):
    try:
        with open(fname, 'rb') as f:
            fbuf = f.read()
            size = len(fbuf)
            return fbuf, size
    except IOError:
        print(f"open {fname} failed")
        return None, 0
    
def extend_overlay_table(overlay_table, id, entry_size):
    required_size = (id + 1) * entry_size
    current_size = len(overlay_table)
    if required_size > current_size:
        overlay_table += b'\x00' * (required_size - current_size)
    return overlay_table

def modify_overlay_entry(overlay_entry, id, vm_addr, fsize, bss_size, init_functions_begin, init_functions_end):
    overlay_entry.id = id
    overlay_entry.ram_address = vm_addr
    overlay_entry.ram_size = fsize
    overlay_entry.bss_size = bss_size
    overlay_entry.sinit_init = init_functions_begin
    overlay_entry.sinit_init_end = init_functions_end
    overlay_entry.file_id = id
    overlay_entry.compressed = 0
    overlay_entry.flag = 0

def add_new_overlay(args):
    file_name = args.file_name
    overlay_table, size = load_file(file_name)
    if overlay_table and size == (size & ~0x1F):
        id = int(args.overlay_id)
        vm_addr = int(args.addr, 16)
        fsize = int(args.file_size, 16)
        bss_size = int(args.bss_size, 16)
        init_functions_begin = int(args.init_functions_begin, 16)
        init_functions_end = int(args.init_functions_end, 16)
        
        overlay_entry_size = struct.calcsize('IIIIIIII')
        overlay_table = extend_overlay_table(overlay_table, id, overlay_entry_size)
        entry_data = overlay_table[id * overlay_entry_size:(id + 1) * overlay_entry_size]
        overlay_entry = FSOverlayInfoHeader()
        overlay_entry.id, overlay_entry.ram_address, overlay_entry.ram_size, overlay_entry.bss_size, overlay_entry.sinit_init, overlay_entry.sinit_init_end, overlay_entry.file_id, flags = struct.unpack('IIIIIIII', entry_data)

        modify_overlay_entry(overlay_entry, id, vm_addr, fsize, bss_size, init_functions_begin, init_functions_end)

        new_entry_data = struct.pack('IIIIIIII', overlay_entry.id, overlay_entry.ram_address, overlay_entry.ram_size, overlay_entry.bss_size, overlay_entry.sinit_init, overlay_entry.sinit_init_end, overlay_entry.file_id, (overlay_entry.compressed << 8) | overlay_entry.flag)
        overlay_table = overlay_table[:id * overlay_entry_size] + new_entry_data + overlay_table[(id + 1) * overlay_entry_size:]

        save_file(file_name, overlay_table)
    return 0

def modify_exist_overlay(args):
    file_name = args.file_name
    overlay_table, size = load_file(file_name)
    if overlay_table and size == (size & ~0x1F):
        id = int(args.overlay_id)
        sinit_init = int(args.sinit_init, 16)
        
        overlay_entry_size = struct.calcsize('IIIIIIII')
        entry_data = overlay_table[id * overlay_entry_size:(id + 1) * overlay_entry_size]
        overlay_entry = FSOverlayInfoHeader()
        overlay_entry.id, overlay_entry.ram_address, overlay_entry.ram_size, overlay_entry.bss_size, overlay_entry.sinit_init, overlay_entry.sinit_init_end, overlay_entry.file_id, flags = struct.unpack('IIIIIIII', entry_data)

        overlay_entry.sinit_init = sinit_init
        overlay_entry.sinit_init_end = sinit_init + 4

        new_entry_data = struct.pack('IIIIIIII', overlay_entry.id, overlay_entry.ram_address, overlay_entry.ram_size, overlay_entry.bss_size, overlay_entry.sinit_init, overlay_entry.sinit_init_end, overlay_entry.file_id, flags)
        overlay_table = overlay_table[:id * overlay_entry_size] + new_entry_data + overlay_table[(id + 1) * overlay_entry_size:]

        save_file(file_name, overlay_table)
    return 0

def main():
    parser = argparse.ArgumentParser(description="Apply overlay modifications.")
    subparsers = parser.add_subparsers()

    # Add new overlay
    parser_add = subparsers.add_parser('add-new-overlay', help="Add a new overlay.")
    parser_add.add_argument('file_name', help="Overlay table file name.")
    parser_add.add_argument('overlay_id', help="Overlay ID.")
    parser_add.add_argument('addr', help="Virtual memory address.")
    parser_add.add_argument('file_size', help="File size.")
    parser_add.add_argument('bss_size', help="BSS size.")
    parser_add.add_argument('init_functions_begin', help="Initialization functions begin address.")
    parser_add.add_argument('init_functions_end', help="Initialization functions end address.")
    parser_add.set_defaults(func=add_new_overlay)

    # Modify existing overlay
    parser_modify = subparsers.add_parser('modify-exist-overlay', help="Modify an existing overlay.")
    parser_modify.add_argument('file_name', help="Overlay table file name.")
    parser_modify.add_argument('overlay_id', help="Overlay ID.")
    parser_modify.add_argument('sinit_init', help="Initialization function address.")
    parser_modify.set_defaults(func=modify_exist_overlay)

    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()