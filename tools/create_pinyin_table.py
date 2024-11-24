import re
import struct
import sys

def read_pinyin_table(filename):
    pinyin_table = {}
    with open(filename, 'r', encoding='utf-16') as file:
        for line in file:
            line = line.strip()
            if not line:
                continue
            parts = line.split(None, 1)
            if len(parts) != 2:
                continue
            pinyin, characters = parts
            characters = characters.replace(' ', '')
            pinyin_table[pinyin] = characters
    return pinyin_table

def read_encoding_table(filename):
    encoding_table = {}
    with open(filename, 'r', encoding='utf-8') as file:
        pattern = re.compile(r'([0-9A-Fa-f]{2,4})=(.)')
        for line in file:
            if not line:
                continue
            match = pattern.match(line)
            if match:
                code = int(match.group(1), 16)
                character = match.group(2)
                encoding_table[character] = code
            else:
                print(f'Invalid line: {line}')
    return encoding_table

def filter_pinyin_table(pinyin_table, encoding_table):
    filtered_pinyin_table = {}
    for pinyin, characters in pinyin_table.items():
        filtered_chars = ''.join([char for char in characters if char in encoding_table])
        if filtered_chars:
            filtered_pinyin_table[pinyin] = filtered_chars
    return filtered_pinyin_table

def encode_pinyin(pinyin):
    # 将字母 'a'-'z' 映射为数字 1-26
    letter_map = {chr(i + ord('a') - 1): i for i in range(1, 27)}
    pinyin = pinyin.lower()
    code = 0
    for i in range(min(len(pinyin), 6)):
        c = pinyin[i]
        value = letter_map.get(c, 0)
        if value == 0:
            raise ValueError(f"拼音 '{pinyin}' 中包含无效字符 '{c}'")
        code |= (value & 0x1F) << (25 - 5 * i)
    return code

def create_binary_pinyin_table(filtered_pinyin_table, output_filename, encoding_table):
    # 准备拼音索引表和汉字数据
    pinyin_entries = []
    all_codes = []
    offset = 0  # 第三部分的偏移量，以编码数量为单位
    max_num_chars = 0  # 用于记录单个拼音的最大汉字数量
    for pinyin, characters in filtered_pinyin_table.items():
        pinyin_code = encode_pinyin(pinyin)
        num_chars = len(characters)
        if num_chars > max_num_chars:
            max_num_chars = num_chars
        entry = {
            'pinyin_code': pinyin_code,
            'offset': offset,
            'num_chars': num_chars
        }
        pinyin_entries.append(entry)
        # 将汉字转换为编码
        for char in characters:
            code = encoding_table.get(char)
            all_codes.append(code)
        offset += num_chars
        
    # 排序拼音索引表
    pinyin_entries.sort(key=lambda x: x['pinyin_code'])

    # 计算第三部分的地址，并对齐到0x10
    num_entries = len(pinyin_entries)
    header_size = 0x10  # 第一部分的大小改为0x10
    index_entry_size = 4 + 2 + 2  # int32 + int16 + int16
    index_table_size = num_entries * index_entry_size
    third_part_offset = header_size + index_table_size
    if third_part_offset % 0x10 != 0:
        third_part_offset = (third_part_offset + 0x10) & ~0xF  # 对齐到0x10

    # 写入二进制文件
    with open(output_filename, 'wb') as f:
        # 写入头部
        f.write(struct.pack('<I', num_entries))
        f.write(struct.pack('<I', third_part_offset))
        f.write(struct.pack('<I', max_num_chars))
        f.write(b'\x00' * (header_size - 12))  # 填充到0x10字节

        # 写入拼音索引表
        for entry in pinyin_entries:
            f.write(struct.pack('<IHH', entry['pinyin_code'], entry['offset'], entry['num_chars']))

        # 填充到第三部分的偏移量
        current_offset = header_size + index_table_size
        while current_offset < third_part_offset:
            f.write(b'\x00')
            current_offset += 1

        # 写入第三部分（汉字数据）
        # 使用自定义编码表的编码，每个编码用 2 字节表示
        for code in all_codes:
            f.write(struct.pack('<H', code))

def main(encoding_file):
    pinyin_table = read_pinyin_table('pinyin.txt')
    encoding_table = read_encoding_table(encoding_file)
    filtered_pinyin_table = filter_pinyin_table(pinyin_table, encoding_table)
    create_binary_pinyin_table(filtered_pinyin_table, 'pinyin_table.bin', encoding_table)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <encoding_file>")
        sys.exit(1)
    
    encoding_file = sys.argv[1]

    main(encoding_file)