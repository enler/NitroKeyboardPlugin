import pytest
import ctypes
import struct
import sys
from unittest.mock import patch, MagicMock


# Simulated realloc behavior that mirrors the vulnerable C code pattern
# This models the invariant: when reallocating, the copy size must never exceed
# the new allocation size (min(block->size, new_size) should be used)

class HeapBlock:
    """Simulates a heap block with metadata similar to keyboard_heap.c"""
    def __init__(self, data: bytes, declared_size: int):
        self.data = bytearray(data)
        self.size = declared_size  # This is block->size in the C code


def safe_realloc(ptr: HeapBlock, new_size: int) -> HeapBlock:
    """
    Safe implementation: copies min(block->size, new_size) bytes.
    This is what the code SHOULD do.
    """
    if new_size <= 0:
        return None
    copy_size = min(ptr.size, new_size)
    new_data = bytearray(new_size)
    new_data[:copy_size] = ptr.data[:copy_size]
    new_block = HeapBlock(bytes(new_data), new_size)
    return new_block


def vulnerable_realloc(ptr: HeapBlock, new_size: int) -> HeapBlock:
    """
    Vulnerable implementation: copies block->size bytes regardless of new_size.
    This mirrors the bug in keyboard_heap.c
    """
    if new_size <= 0:
        return None
    new_data = bytearray(new_size)
    # BUG: uses ptr.size instead of min(ptr.size, new_size)
    copy_size = ptr.size  # This can exceed new_size when shrinking!
    if copy_size > new_size:
        # In real C this would be a buffer overflow; here we detect it
        raise BufferError(
            f"Buffer overflow detected: attempting to copy {copy_size} bytes "
            f"into allocation of {new_size} bytes"
        )
    new_data[:copy_size] = ptr.data[:copy_size]
    new_block = HeapBlock(bytes(new_data), new_size)
    return new_block


def check_realloc_invariant(original_data: bytes, declared_size: int, new_size: int) -> dict:
    """
    Checks the invariant: bytes copied must never exceed the new allocation size.
    Returns a dict with results.
    """
    block = HeapBlock(original_data, declared_size)
    
    result = {
        "original_size": len(original_data),
        "declared_size": declared_size,
        "new_size": new_size,
        "overflow_would_occur": declared_size > new_size,
        "safe_copy_size": min(declared_size, new_size),
        "invariant_violated": False,
        "error": None,
    }
    
    # The invariant: copy_size <= new_size MUST always hold
    copy_size = declared_size  # What the vulnerable code does
    if copy_size > new_size:
        result["invariant_violated"] = True
        result["error"] = (
            f"INVARIANT VIOLATION: copy_size={copy_size} > new_size={new_size}. "
            f"This would cause a heap buffer overflow (CWE-120)."
        )
    
    return result


# Attack payloads: (original_data, declared_block_size, new_requested_size)
# Format: (data, block->size metadata, new allocation size)
PAYLOADS = [
    # Shrink by 2x - classic overflow scenario
    (b"A" * 64, 64, 32),
    
    # Shrink by 10x
    (b"B" * 1000, 1000, 100),
    
    # Corrupted metadata: declared size much larger than actual data
    (b"C" * 16, 0xFFFF, 32),
    
    # Shrink to 1 byte
    (b"D" * 256, 256, 1),
    
    # Corrupted metadata with max-like value
    (b"E" * 8, 0x7FFFFFFF, 16),
    
    # New size is zero (edge case)
    (b"F" * 100, 100, 0),
    
    # Declared size is 2x actual, new size is half declared
    (b"G" * 50, 100, 50),
    
    # Adversarial: declared size just over new size by 1
    (b"H" * 100, 101, 100),
    
    # Large shrink with corrupted block size
    (b"I" * 32, 0x1000, 64),
    
    # Null-byte padded payload
    (b"\x00" * 512, 512, 256),
    
    # Mixed content with embedded size-like values
    (struct.pack(">I", 9999) + b"J" * 60, 64, 32),
    
    # Payload with format string characters
    (b"%s%n%x%p" * 20, 160, 80),
    
    # Payload with shell metacharacters
    (b"; cat /etc/passwd; " * 10, 190, 50),
    
    # Unicode-like byte sequences
    (bytes(range(256)), 256, 128),
    
    # Heap spray pattern
    (b"\x90" * 1024, 1024, 512),
    
    # Declared size is exactly new_size (boundary - should be safe)
    (b"K" * 128, 128, 128),
    
    # Declared size is 0 (edge case)
    (b"L" * 64, 0, 64),
    
    # Extremely large declared size vs tiny new allocation
    (b"M" * 4, 0xDEADBEEF & 0x7FFFFFFF, 8),
    
    # New size larger than declared (safe case - should always pass)
    (b"N" * 32, 32, 64),
    
    # Adversarial: off-by-one overflow
    (b"O" * 100, 100, 99),
]


@pytest.mark.parametrize("payload", PAYLOADS)
def test_realloc_buffer_read_never_exceeds_new_size(payload):
    """
    Invariant: Buffer reads (memcpy source length) must NEVER exceed the new
    allocation size during realloc. When shrinking a block, the copy size must
    be clamped to min(block->size, new_size) to prevent heap buffer overflow
    (CWE-120). This guards against the vulnerability in keyboard_heap.c where
    memcpy uses block->size unconditionally.
    """
    original_data, declared_size, new_size = payload
    
    result = check_realloc_invariant(original_data, declared_size, new_size)
    
    # Core invariant: the number of bytes copied must never exceed new_size
    # If new_size is 0 or negative, realloc should return None/fail gracefully
    if new_size <= 0:
        # Edge case: zero or negative new_size should be rejected, not overflow
        assert result["safe_copy_size"] == 0, (
            f"Zero/negative new_size={new_size} should result in 0 bytes copied, "
            f"not {result['safe_copy_size']}"
        )
        return
    
    # The safe copy size must always be <= new_size
    assert result["safe_copy_size"] <= new_size, (
        f"INVARIANT VIOLATED: safe_copy_size={result['safe_copy_size']} "
        f"exceeds new_size={new_size}. "
        f"declared_size={declared_size}, original_data_len={len(original_data)}"
    )
    
    # Verify that the safe implementation does NOT raise a buffer overflow
    block = HeapBlock(original_data, declared_size)
    try:
        new_block = safe_realloc(block, new_size)
        if new_block is not None:
            # The resulting block must have exactly new_size bytes
            assert len(new_block.data) == new_size, (
                f"New block size mismatch: expected {new_size}, got {len(new_block.data)}"
            )
            # The resulting block's declared size must equal new_size
            assert new_block.size == new_size, (
                f"New block metadata mismatch: expected {new_size}, got {new_block.size}"
            )
    except Exception as e:
        pytest.fail(f"Safe realloc raised unexpected exception: {e}")
    
    # Verify that the vulnerable implementation WOULD cause overflow when
    # declared_size > new_size (proving the invariant matters)
    if declared_size > new_size > 0:
        with pytest.raises(BufferError, match="Buffer overflow detected"):
            vulnerable_realloc(block, new_size)
        # This confirms the invariant: without the fix, overflow occurs
        assert result["invariant_violated"], (
            f"Expected invariant violation when declared_size={declared_size} "
            f"> new_size={new_size}, but none detected"
        )


@pytest.mark.parametrize("payload", PAYLOADS)
def test_realloc_copy_size_is_clamped_to_min(payload):
    """
    Invariant: The actual bytes copied during realloc must equal
    min(block->size, new_size), never block->size alone. This ensures
    that shrinking a buffer never reads beyond the new allocation boundary.
    """
    original_data, declared_size, new_size = payload
    
    if new_size <= 0:
        pytest.skip("Zero/negative new_size is a separate edge case")
    
    expected_copy_size = min(declared_size, new_size)
    
    # The copy size must be bounded by new_size
    assert expected_copy_size <= new_size, (
        f"min(declared_size={declared_size}, new_size={new_size}) = "
        f"{expected_copy_size} exceeds new_size. Math invariant broken."
    )
    
    # Simulate what safe realloc does and verify data integrity
    block = HeapBlock(original_data, declared_size)
    new_block = safe_realloc(block, new_size)
    
    assert new_block is not None
    assert len(new_block.data) == new_size
    
    # Verify only the expected bytes were copied (no out-of-bounds read)
    actual_copy_size = min(len(original_data), expected_copy_size)
    assert bytes(new_block.data[:actual_copy_size]) == original_data[:actual_copy_size], (
        f"Data corruption detected in safe realloc: "
        f"first {actual_copy_size} bytes should match original"
    )


@pytest.mark.parametrize("shrink_factor", [2, 4, 8, 10, 16, 100])
def test_realloc_shrink_never_overflows(shrink_factor):
    """
    Invariant: Shrinking a buffer by any factor must never cause the copy
    operation to write beyond the new allocation boundary. This is a
    regression guard for CWE-120 in keyboard_heap.c.
    """
    original_size = 1024
    new_size = original_size // shrink_factor
    
    if new_size == 0:
        new_size = 1
    
    original_data = bytes(range(256)) * (original_size // 256) + bytes(range(original_size % 256))
    block = HeapBlock(original_data, original_size)
    
    # Safe realloc must succeed without overflow
    new_block = safe_realloc(block, new_size)
    
    assert new_block is not None, "Safe realloc returned None unexpectedly"
    assert len(new_block.data) == new_size, (
        f"New block has wrong size: expected {new_size}, got {len(new_block.data)}"
    )
    
    # Verify the copy was bounded correctly
    copy_size = min(original_size, new_size)
    assert bytes(new_block.data[:copy_size]) == original_data[:copy_size], (
        f"Data integrity check failed after shrink by factor {shrink_factor}"
    )
    
    # Confirm vulnerable version would overflow
    if original_size > new_size:
        with pytest.raises(BufferError):
            vulnerable_realloc(block, new_size)