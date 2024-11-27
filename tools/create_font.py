import freetype
import sys
import struct
from common import read_encoding_table

def create_glyphs(font_path, size, code_table):
    face = freetype.Face(font_path)
    face.set_char_size(size * 64)
    
    characters = []
    for custom_code, character in code_table.items():
        face.load_char(character)
        
        bitmap = face.glyph.bitmap
        width = bitmap.width
        rows = bitmap.rows
        pitch = bitmap.pitch
        buffer = bytes(bitmap.buffer)
        output_buffer = bytearray(12 * 12 // 8)
        
        # 需要将1bpp的字符buffer转换为16x12的1bpp的buffer
        # 如果不为1bpp，跳过
        if face.glyph.bitmap.pixel_mode != freetype.FT_PIXEL_MODE_MONO:
            print(f"Code {custom_code:04x} Character {character} is not 1bpp, skipping")
            continue
        else:
            for row in range(rows):
                line = buffer[row * pitch: (row + 1) * pitch]
                if pitch == 1:
                    line += b'\x00'
                # 翻转字节的bit序
                # 例如：0b10000000 -> 0b00000001
                new_line_bytes = bytearray()
                for byte in line:
                    reversed_byte = 0
                    for i in range(8):
                        reversed_byte |= ((byte >> i) & 1) << (7 - i)
                    new_line_bytes.append(reversed_byte)
                if row % 2 == 0:
                    output_buffer[row // 2 * 3] = new_line_bytes[0]
                    output_buffer[row // 2 * 3 + 1] = new_line_bytes[1] & 0x0F 
                else:
                    output_buffer[row // 2 * 3 + 1] = output_buffer[row // 2 * 3 + 1] | ((new_line_bytes[0] << 4) & 0xF0)
                    output_buffer[row // 2 * 3 + 2] = ((new_line_bytes[0] & 0xF0) >> 4) | ((new_line_bytes[1] & 0x0F) << 4)
                
        character_info = {
        'code': custom_code,
        'width': width,
        'flag': 0,
        'bitmap_data': bytes(output_buffer)
        }
    
        characters.append(character_info)
    return characters

def main(code_table_path, font_path):
    output_file = 'font.bin'
    font_size = 12
    
    code_table = read_encoding_table(code_table_path)
    characters = create_glyphs(font_path, font_size, code_table)
    
    unique_characters = {}
    for char in reversed(characters):
        unique_characters[char['code']] = char
    characters = list(unique_characters.values())
    characters = sorted(characters, key=lambda x: x['code'])
    
    # 写入文件，先写入len(characters)，再写入每个character的信息
    with open(output_file, 'wb') as f:
        f.write(struct.pack('I', len(characters)))
        for character in characters:
            f.write(struct.pack('<HBB18s', character['code'], character['width'], character['flag'], character['bitmap_data']))
            
if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python create_font.py <code_table_path> <font_path>")
        sys.exit(1)
    code_table_path = sys.argv[1]
    font_path = sys.argv[2]
    main(code_table_path, font_path)