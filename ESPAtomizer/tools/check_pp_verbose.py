#!/usr/bin/env python3
import sys,re
if len(sys.argv)!=2:
    print('Usage: check_pp_verbose.py <file>')
    sys.exit(2)
pat=re.compile(r"^\s*#\s*(ifn?def|if|elif|else|endif)\b(.*)$")
stack=[]
with open(sys.argv[1],'r',encoding='utf-8') as f:
    for ln,line in enumerate(f, start=1):
        m=pat.match(line)
        if not m: continue
        d=m.group(1); rest=m.group(2).strip()
        print(f"Line {ln}: #{d} {rest}")
        if d in ('if','ifdef','ifndef'):
            stack.append((d,rest,ln))
            print('  PUSH -> depth', len(stack))
        elif d=='elif':
            print('  ELIF -> depth', len(stack))
        elif d=='else':
            print('  ELSE -> depth', len(stack))
        elif d=='endif':
            if stack:
                ot=stack.pop()
                print('  POP (matched opened at line', ot[2],') -> depth', len(stack))
            else:
                print('  POP ERROR (no open #if)')
print('\nFinal stack:')
for d,cond,ln in stack:
    print(f"  Open at {ln}: #{d} {cond}")
print('Depth=', len(stack))
if stack:
    sys.exit(1)
else:
    sys.exit(0)
