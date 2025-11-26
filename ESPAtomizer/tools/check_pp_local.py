#!/usr/bin/env python3
import sys
import re

if len(sys.argv) != 2:
    print("Usage: check_pp_local.py <file>")
    sys.exit(2)

path = sys.argv[1]
stack = []
line_no = 0
errors = 0
pat = re.compile(r"^\s*#\s*(ifn?def|if|elif|else|endif)\b(.*)$")

with open(path, 'r', encoding='utf-8') as f:
    for ln, line in enumerate(f, start=1):
        m = pat.match(line)
        if not m:
            continue
        directive = m.group(1)
        rest = m.group(2).strip()
        if directive in ('if', 'ifdef', 'ifndef'):
            stack.append((directive, rest, ln))
        elif directive == 'elif':
            if not stack:
                print(f"Line {ln}: unexpected #elif (no open #if)")
                errors += 1
            else:
                # treat as staying inside same block
                pass
        elif directive == 'else':
            if not stack:
                print(f"Line {ln}: unexpected #else (no open #if)")
                errors += 1
            else:
                # mark else, but still within same block
                pass
        elif directive == 'endif':
            if not stack:
                print(f"Line {ln}: unexpected #endif (no open #if)")
                errors += 1
            else:
                stack.pop()

if stack:
    print("Unclosed conditional(s):")
    for d, cond, ln in stack:
        print(f"  Opened at line {ln}: #{d} {cond}")
    errors += len(stack)

if errors == 0:
    print("No preprocessor balance errors found.")
    sys.exit(0)
else:
    print(f"Found {errors} problem(s)")
    sys.exit(1)
