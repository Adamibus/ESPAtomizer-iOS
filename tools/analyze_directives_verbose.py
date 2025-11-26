import re
from pathlib import Path
p = Path(r"c:\Users\adinj\OneDrive\Projects\Coding\ESPAtomizer\ESPAtomizer\ESPAtomizer.ino")
lines = p.read_text(encoding='utf-8', errors='replace').splitlines()
pat = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")
stack = []
print(f"Directives in order (line: token rest):\n")
for i, line in enumerate(lines, start=1):
    m = pat.match(line)
    if m:
        tok = m.group(1)
        rest = m.group(2).strip()
        print(f"{i:4}: #{tok} {rest}")
print('\nNow show stack operations and unmatched...')
stack = []
for i, line in enumerate(lines, start=1):
    m = pat.match(line)
    if not m: continue
    tok = m.group(1)
    rest = m.group(2).strip()
    if tok in ('if','ifdef','ifndef'):
        stack.append((tok,rest,i))
        print(f"PUSH  {tok} (line {i})")
    elif tok in ('elif','else'):
        if not stack:
            print(f"ISSUE: {tok} at {i} with empty stack")
        else:
            print(f"BRANCH {tok} at {i} (top is line {stack[-1][2]})")
    elif tok == 'endif':
        if not stack:
            print(f"ISSUE: endif at {i} with empty stack")
        else:
            otop = stack.pop()
            print(f"POP   endif at {i} closes {otop[0]} from line {otop[2]}")

if stack:
    print('\nUnclosed items:')
    for tok,rest,ln in stack:
        print(f"  {tok} at line {ln}: {rest}")
else:
    print('\nAll balanced according to this pass.')
