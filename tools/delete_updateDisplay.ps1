$lines = @(Get-Content "ESPAtomizer\ESPAtomizer.ino")
$newLines = @()
$skip = $false
$foundStart = $false

foreach ($i in 0..($lines.Length-1)) {
    $line = $lines[$i]
    
    # Check if this is the start of updateDisplay block
    if ($line -match '^#if USE_OLED$' -and $i+1 -lt $lines.Length -and $lines[$i+1] -match '^void updateDisplay\(\)') {
        $skip = $true
        $foundStart = $true
        continue
    }
    
    # Check if we've reached the end of the block
    if ($skip -and $line -match '^#endif$') {
        $skip = $false
        continue
    }
    
    # Add line if not skipping
    if (!$skip) {
        $newLines += $line
    }
}

# Write back to file
$newLines -join "`n" | Set-Content "ESPAtomizer\ESPAtomizer.ino"
Write-Host "Done. Removed updateDisplay block. Found start: $foundStart"
