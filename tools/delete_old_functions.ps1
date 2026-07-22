$filePath = "ESPAtomizer\ESPAtomizer.ino"
$lines = @(Get-Content $filePath)
$newLines = @()
$skip = $false
$skipUntilBrace = $false
$braceDepth = 0

for ($i = 0; $i -lt $lines.Length; $i++) {
    $line = $lines[$i]
    
    # Check if we should start skipping - for functions that are now handled by managers
    if ($line -match '(void|static void) (printHelp|printStatus|printDiagnostics|resetErrorCounters|logError|checkSystemHealth|applyPidMode|updateSensorFaultState|checkWatchdog|checkThermalRunaway|checkBatterySafety|handleConfirmationDialog)\(' -or 
        $line -match '^class ChCallbacks' -or
        $line -match '(void|static void) setupBLE\(') {
        $skip = $true
        $braceDepth = 0
        continue
    }
    
    # If skipping, track braces until we're back to depth 0
    if ($skip) {
        $braceDepth += ($line | Select-String -Pattern '{' -AllMatches).Matches.Count
        $braceDepth -= ($line | Select-String -Pattern '}' -AllMatches).Matches.Count
        
        # If we've closed all braces, stop skipping
        if ($braceDepth -le 0) {
            $skip = $false
        }
        continue
    }
    
    # Add line if not skipping
    $newLines += $line
}

# Write back to file
$newLines -join "`n" | Set-Content $filePath
Write-Host "Deleted old handler functions. New line count: $($newLines.Count)"
