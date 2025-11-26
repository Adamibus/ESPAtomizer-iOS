from pathlib import Path
p=Path(r"c:\Users\adinj\OneDrive\Projects\Coding\ESPAtomizer\ESPAtomizer\ESPAtomizer.ino")
lines=p.read_text(encoding='utf-8',errors='replace').splitlines()
start=1100
end=1610
depth=0
for i in range(1,start):
    for ch in lines[i-1]:
        if ch=='{': depth+=1
        elif ch=='}': depth-=1
# Now print lines with depth and changes
for i in range(start,end+1):
    line=lines[i-1]
    # compute changes in this line
    for ch in line:
        if ch=='{': depth+=1
        elif ch=='}': depth-=1
    print(f"{i:4}: depth={depth:3} | {line}")
    if depth<0:
        print('  -> depth went negative here')
        break
print('\nFinal depth at line', end, 'is', depth)
