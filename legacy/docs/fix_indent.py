#!/usr/bin/env python3
"""Fix indentation in updateDisplay and remaining sections"""

filepath = r"c:\Users\Adam Dinjian\OneDrive\Projects\Coding\ESPAtomizer\ESPAtomizer\ESPAtomizer.ino"

with open(filepath, 'r') as f:
    lines = f.readlines()

# Find the updateDisplay function
in_update_display = False
fixed_lines = []
brace_count = 0
found_update = False

for i, line in enumerate(lines):
    # Check if we're entering updateDisplay
    if 'void updateDisplay()' in line:
        in_update_display = True
        found_update = True
        brace_count = 0
    
    # If we're in updateDisplay, fix indentation
    if in_update_display:
        # Count braces to know when we exit
        brace_count += line.count('{')
        brace_count -= line.count('}')
        
        # Fix indentation - remove 2 extra spaces if it starts with 4+ spaces
        if line.startswith('    ') and line.strip() and not line.strip().startswith('//'):
            # Remove 2 spaces of indentation
            line = line[2:]
        
        fixed_lines.append(line)
        
        # If braces are balanced, we've exited the function
        if in_update_display and brace_count == 0 and 'void updateDisplay()' not in line:
            in_update_display = False
    else:
        fixed_lines.append(line)

if found_update:
    print(f"Found updateDisplay at around line {i}")
    with open(filepath, 'w') as f:
        f.writelines(fixed_lines)
    print("Fixed indentation!")
else:
    print("Could not find updateDisplay function")
