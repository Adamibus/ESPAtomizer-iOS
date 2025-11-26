from pathlib import Path
p = Path(r"c:\Users\adinj\OneDrive\Projects\Coding\ESPAtomizer\ESPAtomizer\ESPAtomizer.ino")
text = p.read_text(encoding='utf-8', errors='replace')
lines = text.splitlines()
stack = []
for i, line in enumerate(lines, start=1):
    for j, ch in enumerate(line):
        if ch == '{':
            stack.append((i,j))
        elif ch == '}':
            if not stack:
                print(f"Unmatched closing brace '}}' at line {i}, col {j}")
                exit(0)
            stack.pop()
if stack:
    print(f"Unclosed '{{' count: {len(stack)}; first at line {stack[0][0]}")
else:
    print("Braces appear balanced.")
