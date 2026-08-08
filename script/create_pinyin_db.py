"""Build the compact, read-only pinyin database used by the NDS plugin.

File layout:

    32-byte file header
    8-byte first-level entries (all kept in the first 4 KiB)
    512-byte-aligned second-level blocks (at most 16 KiB each)

Each second-level block contains an 8-byte header, an array of 8-byte pinyin
entries, NUL-terminated pinyin suffixes, and length-prefixed game-code words.
"""

import os
import struct
import sys
from collections import defaultdict

from common import read_encoding_table_reverse


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PINYIN_FILE = os.path.join(SCRIPT_DIR, "pinyin.txt")
DEFAULT_RIME_FILE = os.path.normpath(
    os.path.join(
        SCRIPT_DIR, "..", "third_party", "rime-pinyin-simp", "pinyin_simp.dict.yaml"
    )
)
OUTPUT_FILE = "pinyin_db.bin"

MAGIC = b"PYDB1\0\0\0"
VERSION = 1
STARTUP_SIZE = 4096
SECTOR_SIZE = 512
MAX_BLOCK_SIZE = 16 * 1024
MAX_PREFIX_LENGTH = 6
MAX_PINYIN_LENGTH = 20

# magic, version, header size, block count, max prefix length,
# first-level offset, data offset, file size, maximum block size
FILE_HEADER = struct.Struct("<8sHHHHIIII")
# packed prefix, absolute block offset; block size comes from the next offset
LEVEL1_ENTRY = struct.Struct("<II")
# pinyin count, candidate count, candidate-data offset
BLOCK_HEADER = struct.Struct("<HHI")
# pinyin-suffix offset, candidate offset (u16 units), candidate count
PINYIN_ENTRY = struct.Struct("<IHH")

MAX_LEVEL1_ENTRIES = (STARTUP_SIZE - FILE_HEADER.size) // LEVEL1_ENTRY.size

if (
    FILE_HEADER.size != 32
    or LEVEL1_ENTRY.size != 8
    or BLOCK_HEADER.size != 8
    or PINYIN_ENTRY.size != 8
):
    raise RuntimeError("pinyin database structure sizes changed")


def align(value, alignment):
    return (value + alignment - 1) & ~(alignment - 1)


def check_pinyin(pinyin, filename, line_number):
    if (
        not 1 <= len(pinyin) <= MAX_PINYIN_LENGTH
        or not pinyin.isascii()
        or not pinyin.isalpha()
        or not pinyin.islower()
    ):
        raise ValueError(f"{filename}:{line_number}: invalid pinyin {pinyin!r}")


def read_pinyin_file(filename):
    table = {}
    with open(filename, "r", encoding="utf-16") as source:
        for line_number, line in enumerate(source, 1):
            fields = line.strip().split(None, 1)
            if not fields:
                continue
            if len(fields) != 2:
                raise ValueError(f"{filename}:{line_number}: invalid pinyin row")
            pinyin, characters = fields
            check_pinyin(pinyin, filename, line_number)
            table[pinyin] = "".join(characters.split())
    return table


def read_rime_file(filename):
    table = defaultdict(dict)
    syllables = set()
    in_dictionary = False

    with open(filename, "r", encoding="utf-8-sig") as source:
        for line_number, raw_line in enumerate(source, 1):
            line = raw_line.rstrip("\r\n")
            if not in_dictionary:
                if line == "...":
                    in_dictionary = True
                continue
            if not line or line.startswith("#"):
                continue

            fields = line.split("\t", 2)
            if len(fields) != 3:
                raise ValueError(f"{filename}:{line_number}: invalid dictionary row")
            word, spelling, weight_text = fields
            parts = spelling.split()
            pinyin = "".join(parts)
            check_pinyin(pinyin, filename, line_number)
            if any(not 1 <= len(part) <= MAX_PREFIX_LENGTH for part in parts):
                raise ValueError(f"{filename}:{line_number}: invalid pinyin syllable")
            if not word:
                raise ValueError(f"{filename}:{line_number}: empty candidate")
            weight = int(weight_text.strip(), 10)
            if not 0 <= weight <= 0xFFFFFFFF:
                raise ValueError(f"{filename}:{line_number}: invalid weight")
            syllables.update(parts)
            old_weight = table[pinyin].get(word)
            if old_weight is None or weight > old_weight:
                table[pinyin][word] = weight

    if not in_dictionary:
        raise ValueError(f"{filename}: dictionary marker not found")
    return table, syllables


def read_rime_files(filenames):
    table = defaultdict(dict)
    syllables = set()

    for filename in filenames:
        file_table, file_syllables = read_rime_file(filename)
        syllables.update(file_syllables)
        for pinyin, words in file_table.items():
            for word, weight in words.items():
                old_weight = table[pinyin].get(word)
                if old_weight is None or weight > old_weight:
                    table[pinyin][word] = weight

    return table, syllables


def combine_tables(pinyin_table, rime_table, encoding_table):
    combined = {}

    for pinyin in set(pinyin_table) | set(rime_table):
        words = []
        seen = set()

        # Rime's numeric weight is the primary candidate priority.
        rime_words = sorted(
            rime_table.get(pinyin, {}).items(),
            key=lambda item: (-item[1], item[0].encode("utf-8")),
        )
        for word, _weight in rime_words:
            if word not in seen and all(character in encoding_table for character in word):
                words.append(word)
                seen.add(word)

        # Keep pinyin.txt as a compatibility supplement. Characters without a
        # Rime entry retain their old relative order, but follow weighted words.
        for character in pinyin_table.get(pinyin, ""):
            if character in encoding_table and character not in seen:
                words.append(character)
                seen.add(character)

        if words:
            combined[pinyin] = words

    return combined


def encode_prefix(prefix):
    """Use the old pinyin_table.bin 6-letter, collision-free encoding."""
    code = 0
    for index, letter in enumerate(prefix):
        code |= (ord(letter) - ord("a") + 1) << (25 - index * 5)
    return code


def assign_groups(table, prefixes):
    groups = defaultdict(list)
    for pinyin, words in table.items():
        prefix = next(
            (
                pinyin[:length]
                for length in range(min(MAX_PREFIX_LENGTH, len(pinyin)), 0, -1)
                if pinyin[:length] in prefixes
            ),
            None,
        )
        if prefix is None:
            raise ValueError(f"cannot assign a first-level prefix for {pinyin!r}")
        groups[prefix].append((pinyin, words))

    for rows in groups.values():
        rows.sort(key=lambda row: row[0])
    return groups


def measure_block(rows, prefix):
    pinyin_bytes = sum(len(pinyin) - len(prefix) + 1 for pinyin, _words in rows)
    candidate_count = sum(len(words) for _pinyin, words in rows)
    candidate_bytes = sum(
        2 + len(word) * 2 for _pinyin, words in rows for word in words
    )
    if len(rows) > 0xFFFF or candidate_count > 0xFFFF:
        raise ValueError(f"block {prefix!r} exceeds a 16-bit count")

    candidate_offset = align(
        BLOCK_HEADER.size + len(rows) * PINYIN_ENTRY.size + pinyin_bytes, 2
    )
    raw_size = candidate_offset + candidate_bytes
    return raw_size, align(raw_size, SECTOR_SIZE)


def analyze_prefixes(table, syllables):
    prefixes = set(syllables)
    special_prefixes = set()

    # pinyin.txt has a few rare readings absent from the Rime syllable list.
    for pinyin in table:
        if not any(
            pinyin[:length] in prefixes
            for length in range(min(MAX_PREFIX_LENGTH, len(pinyin)), 0, -1)
        ):
            prefixes.add(pinyin[:MAX_PREFIX_LENGTH])

    if len(prefixes) > MAX_LEVEL1_ENTRIES:
        raise ValueError("base prefixes do not fit in the 4 KiB startup table")

    while True:
        groups = assign_groups(table, prefixes)
        additions = []

        for parent in sorted(groups, key=encode_prefix):
            rows = groups[parent]
            _raw_size, parent_size = measure_block(rows, parent)
            if parent_size <= MAX_BLOCK_SIZE:
                continue
            if parent in special_prefixes or len(parent) == MAX_PREFIX_LENGTH:
                raise ValueError(f"block {parent!r} exceeds {MAX_BLOCK_SIZE} bytes")

            best = None
            next_letters = sorted(
                {pinyin[len(parent)] for pinyin, _words in rows if len(pinyin) > len(parent)}
            )
            for letter in next_letters:
                child = parent + letter
                if child in prefixes:
                    continue
                bucket = [
                    row for row in rows
                    if len(row[0]) > len(parent) and row[0][len(parent)] == letter
                ]
                _child_raw, child_size = measure_block(bucket, child)
                if child_size > MAX_BLOCK_SIZE:
                    raise ValueError(
                        f"one-letter extension block {child!r} exceeds {MAX_BLOCK_SIZE} bytes"
                    )
                contribution = sum(
                    PINYIN_ENTRY.size
                    + len(pinyin) - len(parent) + 1
                    + sum(2 + len(word) * 2 for word in words)
                    for pinyin, words in bucket
                )
                choice = (contribution, -encode_prefix(child), child, child_size)
                if best is None or choice > best:
                    best = choice

            if best is None:
                raise ValueError(f"block {parent!r} has no usable split")
            contribution, _sort_key, child, child_size = best
            additions.append(child)
            print(
                f"  split {parent:<6} {parent_size:5d} B -> {child:<6} "
                f"({child_size} B, removes {contribution} B)"
            )

        if not additions:
            return groups, special_prefixes

        for child in additions:
            prefixes.add(child)
            special_prefixes.add(child)
        if len(prefixes) > MAX_LEVEL1_ENTRIES:
            raise ValueError("analyzed prefixes do not fit in the 4 KiB startup table")


def serialize_block(rows, prefix, encoding_table):
    raw_size, block_size = measure_block(rows, prefix)
    block = bytearray(block_size)
    pinyin_cursor = BLOCK_HEADER.size + len(rows) * PINYIN_ENTRY.size
    pinyin_bytes = sum(len(pinyin) - len(prefix) + 1 for pinyin, _words in rows)
    candidate_pool_offset = align(pinyin_cursor + pinyin_bytes, 2)
    candidate_cursor = 0
    candidate_count = sum(len(words) for _pinyin, words in rows)

    BLOCK_HEADER.pack_into(
        block, 0, len(rows), candidate_count, candidate_pool_offset
    )

    for index, (pinyin, words) in enumerate(rows):
        suffix = pinyin[len(prefix):].encode("ascii") + b"\0"
        PINYIN_ENTRY.pack_into(
            block,
            BLOCK_HEADER.size + index * PINYIN_ENTRY.size,
            pinyin_cursor,
            candidate_cursor,
            len(words),
        )
        block[pinyin_cursor:pinyin_cursor + len(suffix)] = suffix
        pinyin_cursor += len(suffix)

        for word in words:
            position = candidate_pool_offset + candidate_cursor * 2
            struct.pack_into("<H", block, position, len(word))
            candidate_cursor += 1
            for character in word:
                code = encoding_table[character]
                if not 0 <= code <= 0xFFFF:
                    raise ValueError(f"encoding for {character!r} exceeds 16 bits")
                struct.pack_into(
                    "<H", block, candidate_pool_offset + candidate_cursor * 2, code
                )
                candidate_cursor += 1

    if candidate_pool_offset + candidate_cursor * 2 != raw_size:
        raise ValueError(f"serialized block {prefix!r} has an invalid size")
    return bytes(block)


def create_pinyin_db(encoding_file, rime_files):
    encoding_table = read_encoding_table_reverse(encoding_file)
    pinyin_table = read_pinyin_file(PINYIN_FILE)
    rime_table, syllables = read_rime_files(rime_files)
    combined = combine_tables(pinyin_table, rime_table, encoding_table)
    if not combined:
        raise ValueError("the encoding table has no supported pinyin candidates")

    groups, special_prefixes = analyze_prefixes(combined, syllables)
    ordered_prefixes = sorted(groups, key=encode_prefix)
    if FILE_HEADER.size + len(ordered_prefixes) * LEVEL1_ENTRY.size > STARTUP_SIZE:
        raise ValueError("first-level entries do not fit in the 4 KiB startup table")

    image = bytearray(STARTUP_SIZE)
    maximum_block_size = 0
    for index, prefix in enumerate(ordered_prefixes):
        block = serialize_block(groups[prefix], prefix, encoding_table)
        LEVEL1_ENTRY.pack_into(
            image,
            FILE_HEADER.size + index * LEVEL1_ENTRY.size,
            encode_prefix(prefix),
            len(image),
        )
        image.extend(block)
        maximum_block_size = max(maximum_block_size, len(block))

    FILE_HEADER.pack_into(
        image,
        0,
        MAGIC,
        VERSION,
        FILE_HEADER.size,
        len(ordered_prefixes),
        MAX_PREFIX_LENGTH,
        FILE_HEADER.size,
        STARTUP_SIZE,
        len(image),
        maximum_block_size,
    )

    with open(OUTPUT_FILE, "wb") as output:
        output.write(image)

    print(f"Created {OUTPUT_FILE}")
    print("  Rime dictionaries:")
    for filename in rime_files:
        print(f"    {filename}")
    print(
        f"  pinyin: {len(combined)}, candidates: "
        f"{sum(len(words) for words in combined.values())}"
    )
    print(
        f"  blocks: {len(ordered_prefixes)} ({len(special_prefixes)} dynamic), "
        f"largest: {maximum_block_size} B, file: {len(image)} B"
    )


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <encoding_file> [rime_yaml ...]")
        print(f"  With no rime_yaml, uses: {DEFAULT_RIME_FILE}")
        print("  With paths, uses only the listed dictionaries.")
        sys.exit(1)

    try:
        rime_files = sys.argv[2:] or [DEFAULT_RIME_FILE]
        create_pinyin_db(sys.argv[1], rime_files)
    except (OSError, UnicodeError, ValueError, struct.error) as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
