# ESPAtomizer Project Structure

## Directory Layout

```
ESPAtomizer/
├── ESPAtomizer.ino          # Main Arduino sketch
├── *.h                      # Header files (in root for Arduino IDE)
├── src/                     # Implementation files (.cpp)
│   └── ads1115_driver.cpp
├── examples/                # Example files (guarded with #if 0, not compiled)
│   └── ads1115_integration_example.cpp
├── backup/                  # Backup files
│   └── ESPAtomizer.ino.bak  # Original sketch backup
└── tools/                   # Utility scripts
```

## Why This Structure?

- **Clean root directory**: Only .ino and .h files in the main folder
- **Organized code**: Implementation files (.cpp) in dedicated src/ directory
- **Examples separated**: Example code in examples/ directory (won't compile)
- **Backups isolated**: Backup files in dedicated backup/ directory
- **Arduino IDE compatible**: IDE automatically finds .cpp files in subdirectories
- **Build optimization**: Only compiles necessary files

## Compilation

The Arduino IDE will automatically:
1. Compile ESPAtomizer.ino
2. Find and compile all .cpp files in src/
3. Link with .h files from root directory
4. Ignore files in examples/ (they're guarded) and backup/ directories

No special configuration needed!