import re
import sys
from pathlib import Path

p = Path(r"c:\Users\adinj\OneDrive\Projects\Coding\ESPAtomizer\ESPAtomizer\ESPAtomizer.ino")
if not p.exists():
    print("File not found:", p)
    sys.exit(2)

lines = p.read_text(encoding='utf-8', errors='replace').splitlines()
pat = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")
stack = []
issues = []

for i, line in enumerate(lines, start=1):
    m = pat.match(line)
    if not m:
        continue
    tok = m.group(1)
    rest = m.group(2).strip()
    if tok in ('if', 'ifdef', 'ifndef'):
        stack.append((tok, rest, i))
    elif tok in ('elif', 'else'):
        if not stack:
            issues.append(("%s without open conditional" % tok, i, line))
        else:
            # valid in context; annotate top of stack last branch
            pass
    elif tok == 'endif':
        if not stack:
            issues.append(("endif without open conditional", i, line))
        else:
            stack.pop()

# After scan
if stack:
    for (tok, rest, ln) in stack:
        issues.append((f"Unclosed {tok}", ln, lines[ln-1]))

# Print summary
print(f"Scanned: {p}\nTotal lines: {len(lines)}")
print("\nSummary:")
print(f"  Opening conditionals left on stack: {len(stack)}")
print(f"  Issues found: {len(issues)}\n")

if issues:
    for kind, ln, txt in issues:
        print(f"Issue: {kind} at line {ln}: {txt.strip()}")
    print('\nContext around reported lines:')
    for _, ln, _ in issues:
        start = max(1, ln-3)
        end = min(len(lines), ln+3)
        print(f"\n--- Lines {start}-{end} ---")
        for j in range(start, end+1):
            marker = '>' if j==ln else ' '
            print(f"{marker:1} {j:4}: {lines[j-1]}")
else:
    print("No issues found: all #if/#endif appear balanced (local stack check).")

# Also print counts per directive
from collections import Counter
cnt = Counter()
for line in lines:
    m = pat.match(line)
    if m:
        cnt[m.group(1)] += 1
print('\nDirective counts:')
for k in ('if','ifdef','ifndef','elif','else','endif'):
    print(f"  {k}: {cnt.get(k,0)}")
