import re
def get_code_char_pattern():
    return re.compile(r'([0-9A-Fa-f]{2,4})=(.)')

def read_encoding_table(filename):
    encoding_table = {}
    with open(filename, 'r', encoding='utf-8') as file:
        pattern = get_code_char_pattern()
        for line in file:
            if not line.strip():
                continue
            match = pattern.match(line)
            if match:
                code = int(match.group(1), 16)
                character = match.group(2)
                if code not in encoding_table:
                    encoding_table[code] = character
                else:
                    print(f'Duplicate code: {code:#04x} for character {character}')
            else:
                print(f'Invalid line: {line.strip()}')
    return encoding_table

def read_encoding_table_reverse(filename):
    encoding_table = {}
    with open(filename, 'r', encoding='utf-8') as file:
        pattern = get_code_char_pattern()
        for line in file:
            if not line.strip():
                continue
            match = pattern.match(line)
            if match:
                code = int(match.group(1), 16)
                character = match.group(2)
                if character not in encoding_table:
                    encoding_table[character] = code
                else:
                    print(f'Duplicate character: {character} for code {code:#04x}')
            else:
                print(f'Invalid line: {line.strip()}')
    return encoding_table