"""Generate a reviewable Rime pinyin dictionary from a plain word list."""

import os
import re
import sys


MAX_WORD_LENGTH = 4
MAX_PINYIN_LENGTH = 20
MAX_SYLLABLE_LENGTH = 6
DEFAULT_WEIGHT = 0


def is_han_character(character):
    codepoint = ord(character)
    return (
        codepoint == 0x3007
        or 0x3400 <= codepoint <= 0x4DBF
        or 0x4E00 <= codepoint <= 0x9FFF
        or 0xF900 <= codepoint <= 0xFAFF
    )


def get_dictionary_name(filename):
    basename = os.path.basename(filename)
    if basename.endswith(".dict.yaml"):
        basename = basename[:-len(".dict.yaml")]
    else:
        basename = os.path.splitext(basename)[0]
    name = re.sub(r"[^a-z0-9_]", "_", basename.lower()).strip("_")
    if not name or name[0].isdigit():
        name = f"custom_{name}" if name else "custom_pinyin"
    return name


def check_spelling(parts):
    return all(
        1 <= len(part) <= MAX_SYLLABLE_LENGTH
        and part.isascii()
        and part.isalpha()
        and part.islower()
        for part in parts
    ) and len("".join(parts)) <= MAX_PINYIN_LENGTH


def read_words(filename, lazy_pinyin, normal_style):
    rows = []
    seen = set()
    skipped = {
        "blank": 0,
        "duplicate": 0,
        "too_long": 0,
        "non_han": 0,
        "invalid_pinyin": 0,
    }

    with open(filename, "r", encoding="utf-8-sig") as source:
        for raw_line in source:
            word = raw_line.rstrip("\r\n")
            if not word:
                skipped["blank"] += 1
                continue
            if word in seen:
                skipped["duplicate"] += 1
                continue
            seen.add(word)
            if len(word) > MAX_WORD_LENGTH:
                skipped["too_long"] += 1
                continue
            if not all(is_han_character(character) for character in word):
                skipped["non_han"] += 1
                continue

            parts = lazy_pinyin(word, style=normal_style, strict=True)
            if len(parts) != len(word) or not check_spelling(parts):
                skipped["invalid_pinyin"] += 1
                continue
            rows.append((word, " ".join(parts), DEFAULT_WEIGHT))

    return rows, skipped


def create_rime_dict(word_file, output_file):
    try:
        from pypinyin import Style, lazy_pinyin
    except ImportError as error:
        raise RuntimeError(
            "pypinyin is required; install it with: "
            "python -m pip install -r script/requirements.txt"
        ) from error

    rows, skipped = read_words(word_file, lazy_pinyin, Style.NORMAL)
    dictionary_name = get_dictionary_name(output_file)
    with open(output_file, "w", encoding="utf-8", newline="\n") as output:
        output.write("# Rime dictionary\n")
        output.write("# encoding: utf-8\n")
        output.write("# Review generated pronunciations before building pinyin_db.bin.\n")
        output.write("# The third column is weight; a larger value has higher priority.\n")
        output.write("\n")
        output.write("---\n")
        output.write(f"name: {dictionary_name}\n")
        output.write('version: "1.0"\n')
        output.write("sort: by_weight\n")
        output.write("columns:\n")
        output.write("  - text\n")
        output.write("  - code\n")
        output.write("  - weight\n")
        output.write("...\n")
        output.write("\n")
        for word, spelling, weight in rows:
            output.write(f"{word}\t{spelling}\t{weight}\n")

    print(f"Created {output_file}")
    print(f"  words: {len(rows)}")
    print(
        "  skipped: "
        f"blank={skipped['blank']}, duplicate={skipped['duplicate']}, "
        f"over-{MAX_WORD_LENGTH}-characters={skipped['too_long']}, "
        f"non-Han={skipped['non_han']}, "
        f"invalid-or-over-{MAX_PINYIN_LENGTH}-letter-pinyin="
        f"{skipped['invalid_pinyin']}"
    )


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <word_file> <output_rime_yaml>")
        sys.exit(1)

    try:
        create_rime_dict(sys.argv[1], sys.argv[2])
    except (OSError, UnicodeError, RuntimeError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
