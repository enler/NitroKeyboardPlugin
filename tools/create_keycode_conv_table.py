import re
import os
import sys
from common import read_encoding_table_reverse

def parse_keycodes(file_path):
    keycode_dict = {}
    pattern = re.compile(r'KEYCODE_\w+\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*,')

    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            match = pattern.search(line)
            if match:
                keycode_str = match.group(0)
                keycode_name = keycode_str.split('=')[0].strip()
                keycode_value = match.group(1).strip()
                
                # 识别数字格式
                if keycode_value.startswith('0x'):
                    keycode_value = int(keycode_value, 16)
                else:
                    keycode_value = int(keycode_value)
                
                keycode_dict[keycode_value] = keycode_name

    return keycode_dict

def filter_keycodes(keycode_dict):
    filtered_dict = {}
    for keycode_value, keycode_name in keycode_dict.items():
        # 跳过半角和全角的数字、字母
        if not (0x30 <= keycode_value <= 0x39 or 0xFF10 <= keycode_value <= 0xFF19 or
                0x41 <= keycode_value <= 0x5A or 0xFF21 <= keycode_value <= 0xFF3A or
                0x61 <= keycode_value <= 0x7A or 0xFF41 <= keycode_value <= 0xFF5A or
                0x00 <= keycode_value <= 0x1F):
            filtered_dict[keycode_name] = keycode_value
    return filtered_dict

def main():
    # 获取脚本参数
    if len(sys.argv) != 2:
        print("Usage: python create_keycode_conv_table.py <code_table_path>")
        sys.exit(1)

    char_table_path = sys.argv[1]

    # 计算keyboard.h的路径
    script_dir = os.path.dirname(os.path.abspath(__file__))
    keyboard_h_path = os.path.join(script_dir, '../include/keyboard.h')
    keycode_dict = parse_keycodes(keyboard_h_path)
    filtered_keycode_dict = filter_keycodes(keycode_dict)
    
    encoding_table = read_encoding_table_reverse(char_table_path)
    
    keycode_to_custom_code = {}
    for keycode_name, keycode_value in filtered_keycode_dict.items():
        keycode_value_unicode = chr(keycode_value)
        if keycode_value_unicode in encoding_table:
            keycode_to_custom_code[keycode_name] = encoding_table[keycode_value_unicode]
        else:
            print(f"Character '{keycode_value_unicode}' not found in encoding table")
    
    sorted_keycode_to_custom_code = dict(sorted(keycode_to_custom_code.items(), key=lambda item: filtered_keycode_dict[item[0]]))

    # 输出C数组
    print("const struct KeycodeConvItem gKeycodeConvTable[] = {")
    for keycode_name, custom_code in sorted_keycode_to_custom_code.items():
        print(f'    {{{keycode_name}, 0x{custom_code:04X}}},')
    print("};")
    
if __name__ == "__main__":
    main()