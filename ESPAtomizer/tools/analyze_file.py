#!/usr/bin/env python3
"""Analyze ESPAtomizer.ino for non-ASCII characters and F() macro issues."""

import os
import re

def main():
    ino_path = os.path.join(os.path.dirname(__file__), '..', 'ESPAtomizer.ino')
    
    with open(ino_path, 'rb') as f:
        content = f.read()
    
    print(f"File size: {len(content)} bytes")
    
    # Check first bytes for BOM
    print(f"First 10 bytes: {[hex(b) for b in content[:10]]}")
    
    # Find non-ASCII bytes
    non_ascii = [(i, b) for i, b in enumerate(content) if b > 127]
    print(f"Non-ASCII bytes: {len(non_ascii)}")
    
    if non_ascii:
        print("\nNon-ASCII locations:")
        for pos, byte in non_ascii[:30]:
            # Calculate line number
            line_num = content[:pos].count(b'\n') + 1
            # Get context
            start = max(0, pos - 20)
            end = min(len(content), pos + 20)
            context = content[start:end].decode('utf-8', errors='replace')
            print(f"  Line {line_num}, pos {pos}: byte {byte} (0x{byte:02X})")
            print(f"    Context: {repr(context)}")
    else:
        print("No non-ASCII bytes found - file is clean!")
    
    # Check F() macros
    text = content.decode('utf-8', errors='replace')
    lines = text.split('\n')
    
    print(f"\n\nTotal lines: {len(lines)}")
    
    # Find all F( occurrences and check if they're properly closed
    f_pattern = re.compile(r'F\s*\(\s*"')
    issues = []
    
    for i, line in enumerate(lines, 1):
        if 'F(' in line and '"' in line:
            # Simple check: count quotes (excluding escaped ones)
            cleaned = re.sub(r'\\"', '', line)
            quotes = cleaned.count('"')
            if quotes % 2 != 0:
                issues.append(f"Line {i}: Odd quote count ({quotes}): {line[:80]}")
    
    if issues:
        print("\nPotential F() macro issues:")
        for issue in issues:
            print(f"  {issue}")
    else:
        print("\nNo obvious F() macro issues found.")

if __name__ == '__main__':
    main()
