import sys
import struct

def inject_ldr_loader(arm9_path, overlay_ldr_path, arm9_start_addr, overlay_ldr_inject_addr, replaced_addr, replaced_value):
    # 计算相对地址
    overlay_ldr_inject_addr -= arm9_start_addr
    replaced_addr -= arm9_start_addr

    # 读取 ARM9 文件
    with open(arm9_path, 'rb') as f:
        arm9_data = bytearray(f.read())

    # 读取 overlay_ldr 文件
    with open(overlay_ldr_path, 'rb') as f:
        overlay_ldr_data = f.read()

    # 将 overlay_ldr 写入到 ARM9 中
    arm9_data[overlay_ldr_inject_addr:overlay_ldr_inject_addr + len(overlay_ldr_data)] = overlay_ldr_data

    # 将 replaced_value 以低位在前的32位值替换掉位于 replaced_addr 的值
    replaced_value_bytes = struct.pack('<I', replaced_value)
    arm9_data[replaced_addr:replaced_addr + 4] = replaced_value_bytes

    # 写回修改后的 ARM9 文件
    with open(arm9_path, 'wb') as f:
        f.write(arm9_data)

if __name__ == '__main__':
    if len(sys.argv) != 7:
        print(f"Usage: {sys.argv[0]} <arm9_path> <overlay_ldr_path> <arm9_start_addr> <overlay_ldr_inject_addr> <replaced_addr> <replaced_value>")
        sys.exit(1)

    arm9_path = sys.argv[1]
    overlay_ldr_path = sys.argv[2]
    arm9_start_addr = int(sys.argv[3], 16)
    overlay_ldr_inject_addr = int(sys.argv[4], 16)
    replaced_addr = int(sys.argv[5], 16)
    replaced_value = int(sys.argv[6], 16)

    inject_ldr_loader(arm9_path, overlay_ldr_path, arm9_start_addr, overlay_ldr_inject_addr, replaced_addr, replaced_value|1)