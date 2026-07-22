#!/usr/bin/env python3
import sys

filepath = r"c:\Users\Adam Dinjian\OneDrive\Projects\Coding\ESPAtomizer\ESPAtomizer\ESPAtomizer.ino"

with open(filepath, 'r') as f:
    lines = f.readlines()

in_loop = False
brace_count = 0
loop_start_line = 0

for i, line in enumerate(lines):
    line_num = i + 1
    if 'void loop() {' in line:
        in_loop = True
        loop_start_line = line_num
        brace_count = 1
        print(f"Loop starts at line {line_num}")
        continue
    
    if in_loop:
        # Simple brace counting, ignoring comments and strings for now
        # (though this might be risky if there are braces in strings)
        
        # Remove comments
        clean_line = line.split('//')[0]
        # This doesn't handle /* */ but let's hope there aren't many
        
        for char in clean_line:
            if char == '{':
                brace_count += 1
                # print(f"Line {line_num}: brace_count={brace_count} (added {{)")
            elif char == '}':
                brace_count -= 1
                # print(f"Line {line_num}: brace_count={brace_count} (removed }})")
        
        if brace_count == 0:
            print(f"Loop ends at line {line_num}")
            in_loop = False
            # break # Don't break, see if there are more loops or if it restarts

if in_loop:
    print(f"Loop never ended! Final brace_count={brace_count}")
