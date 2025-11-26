<#
check_nimble_symbols.ps1

Scans the local Arduino15 esp32 package tree to find toolchain `nm` utilities,
searches static libraries (.a) for a specified symbol (default: ble_gatts_set_attr_perm),
and searches headers for the symbol or related header names.

Usage (PowerShell):
  cd "C:\Users\adinj\OneDrive\Projects\Coding\ESPAtomizer"
  .\tools\check_nimble_symbols.ps1                  # default symbol
  .\tools\check_nimble_symbols.ps1 -Symbol ble_store_clear

Outputs a concise report of findings.
#>
param(
    [string]$Symbol = 'ble_gatts_set_attr_perm'
)

$ErrorActionPreference = 'SilentlyContinue'

$arduino15 = "$env:USERPROFILE\AppData\Local\Arduino15"
if (-not (Test-Path $arduino15)) {
    Write-Error "Arduino15 folder not found at $arduino15"
    exit 2
}

Write-Host "Scanning Arduino15 packages under: $arduino15" -ForegroundColor Cyan
Write-Host "Searching for symbol: $Symbol" -ForegroundColor Cyan

# Find nm executables (various toolchains)
$nmCandidates = Get-ChildItem $arduino15 -Recurse -Filter '*-esp-elf-nm.exe' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName -Unique
if (-not $nmCandidates) {
    Write-Warning "No toolchain nm executables found under Arduino15. Search may still proceed using any nm on PATH."
    $nmCandidates = @()
}
else {
    Write-Host "Found nm toolchain executables:" -ForegroundColor Green
    $nmCandidates | ForEach-Object { Write-Host "  $_" }
}

# Find candidate static libraries (.a) under esp32 toolchain/lib paths and arduino-esp32 libs
$libCandidates = Get-ChildItem $arduino15 -Recurse -Filter '*.a' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
Write-Host "Found $($libCandidates.Count) .a libraries (scanning may take a while)..." -ForegroundColor Yellow

$found = @()
if ($nmCandidates.Count -gt 0) {
    foreach ($nm in $nmCandidates) {
        Write-Host "\nUsing nm: $nm" -ForegroundColor Cyan
        foreach ($lib in $libCandidates) {
            try {
                $out = & $nm --defined-only $lib 2>$null | Select-String -Pattern $Symbol -SimpleMatch
                if ($out) {
                    $found += [PSCustomObject]@{ Nm = $nm; Library = $lib; Match = $out.Line }
                }
            } catch {
                # ignore
            }
        }
    }
} else {
    # Try using nm from PATH if available
    $nmPath = (Get-Command nm.exe -ErrorAction SilentlyContinue).Source
    if ($nmPath) {
        Write-Host "Using nm from PATH: $nmPath" -ForegroundColor Cyan
        foreach ($lib in $libCandidates) {
            try {
                $out = & $nmPath --defined-only $lib 2>$null | Select-String -Pattern $Symbol -SimpleMatch
                if ($out) {
                    $found += [PSCustomObject]@{ Nm = $nmPath; Library = $lib; Match = $out.Line }
                }
            } catch {}
        }
    } else {
        Write-Warning "No nm tool available to inspect .a libraries. Skipping binary symbol search." -ForegroundColor Yellow
    }
}

if ($found.Count -gt 0) {
    Write-Host "\nSymbol '$Symbol' found in the following libraries:" -ForegroundColor Green
    $found | Select-Object Nm,Library -Unique | ForEach-Object { Write-Host "  $_.Nm  =>  $_.Library" }
} else {
    Write-Host "\nSymbol '$Symbol' not found in scanned .a libraries." -ForegroundColor Red
}

# Header search (text search for symbol name or common headers)
Write-Host "\nSearching headers for symbol/headers (may list many matches)..." -ForegroundColor Cyan
$headerMatches = Select-String -Path "$arduino15\**\*.h" -Pattern "$Symbol|ble_gatts.h|ble_store.h|NimBLESecurity.h" -SimpleMatch -ErrorAction SilentlyContinue | Select-Object Path,LineNumber,Line -First 200
if ($headerMatches) {
    Write-Host "Header matches (up to 200 shown):" -ForegroundColor Green
    $headerMatches | ForEach-Object { Write-Host "  $($_.Path):$($_.LineNumber) -> $($_.Line)" }
} else {
    Write-Host "No header matches found for symbol or common headers." -ForegroundColor Yellow
}

Write-Host "\nReport complete." -ForegroundColor Cyan

# Exit codes: 0 = ran, 2 = missing Arduino15 path
exit 0
