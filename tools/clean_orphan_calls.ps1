$filePath = "ESPAtomizer\ESPAtomizer.ino"
$lines = @(Get-Content $filePath)
$newLines = @()

# These are function calls to remove because the functions no longer exist
$functionsToRemove = @(
    'handleConfirmationDialog\(.*\);$',
    'checkBatterySafety\(.*\);$',
    'updateSensorFaultState\(.*\);$',
    'checkThermalRunaway\(.*\);$',
    'checkSystemHealth\(.*\);$',
    'checkWatchdog\(.*\);$',
    '^\s+logError\(',  # logError calls - can be removed or left to build error
    '^\s+printHelp\(\);$'  # Only in setup context where function might exist
)

$removedCount = 0

foreach ($line in $lines) {
    $skip = $false
    
    # Check if this line should be removed (is a call to a deleted function)
    foreach ($pattern in $functionsToRemove) {
        if ($line -match $pattern) {
            $skip = $true
            $removedCount++
            break
        }
    }
    
    if (!$skip) {
        $newLines += $line
    }
}

# Write back to file
$newLines -join "`n" | Set-Content $filePath
Write-Host "Removed $removedCount lines with orphaned function calls"
Write-Host "New line count: $($newLines.Count)"
