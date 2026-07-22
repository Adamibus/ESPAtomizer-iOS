# fetch-digikey-skus.ps1
# Fetches DigiKey SKUs and pricing for connectors and updates BOM CSVs
# Run from: hardware/ESPAtomizer_PCB/
# Usage: .\fetch-digikey-skus.ps1

$ErrorActionPreference = "Stop"

Write-Host "=== DigiKey Connector SKU Fetcher ===" -ForegroundColor Cyan
Write-Host "This script will query DigiKey product pages and extract SKUs/pricing." -ForegroundColor Gray
Write-Host ""

# Define connector searches with DigiKey direct product URLs
$connectorParts = @(
    @{
        Designator = "J3"
        Description = "JST-PH 2-pin battery connector"
        MPN = "B2B-PH-K-S(LF)(SN)"
        DigiKeyURL = "https://www.digikey.com/en/products/detail/jst-sales-america-inc/B2B-PH-K-S-LF-SN/926626"
    },
    @{
        Designator = "J5"
        Description = "JST-PH 2-pin heater connector"
        MPN = "B2B-PH-K-S(LF)(SN)"
        DigiKeyURL = "https://www.digikey.com/en/products/detail/jst-sales-america-inc/B2B-PH-K-S-LF-SN/926626"
    },
    @{
        Designator = "J2"
        Description = "Pin socket 1x4"
        MPN = "M20-7820445"
        DigiKeyURL = "https://www.digikey.com/en/products/detail/harwin-inc/M20-7820445/3728226"
    },
    @{
        Designator = "J4"
        Description = "Pin header 1x5"
        MPN = "M20-7820545"
        DigiKeyURL = "https://www.digikey.com/en/products/detail/harwin-inc/M20-7820545/3728327"
    },
    @{
        Designator = "SW2"
        Description = "Slide switch SPST"
        MPN = "JS102011SAQN"
        DigiKeyURL = "https://www.digikey.com/en/products/detail/c-k/JS102011SAQN/1640100"
    },
    @{
        Designator = "SW3"
        Description = "Rotary encoder"
        MPN = "EC11E"
        DigiKeySearchTerm = "EC11E Alps encoder"
        DigiKeyURL = "https://www.digikey.com/en/products/filter/encoders/102?s=N4IgTCBcDaIBIEMC0AYAXATGGA2YB2Aziug"
    },
    @{
        Designator = "POGO1"
        Description = "Pogo pin P75"
        MPN = "P75-E2"
        DigiKeySearchTerm = "P75 pogo pin 1.3mm"
        DigiKeyURL = "https://www.digikey.com/en/products/filter/test-points-contact-probes/613"
    },
    @{
        Designator = "POGO2"
        Description = "Pogo pin P75"
        MPN = "P75-E2"
        DigiKeySearchTerm = "P75 pogo pin 1.3mm"
        DigiKeyURL = "https://www.digikey.com/en/products/filter/test-points-contact-probes/613"
    }
)

$results = @()

foreach ($part in $connectorParts) {
    Write-Host "Fetching: $($part.Designator) - $($part.Description)" -ForegroundColor Yellow
    Write-Host "  URL: $($part.DigiKeyURL)" -ForegroundColor Gray
    
    try {
        # Fetch product page
        $response = Invoke-WebRequest -Uri $part.DigiKeyURL -UseBasicParsing -ErrorAction Stop
        
        # Try to extract DigiKey part number from page
        # DigiKey pages typically have the PN in meta tags or in a span with specific id
        $html = $response.Content
        
        # Pattern 1: Try meta tag
        if ($html -match 'property="og:product:sku"\s+content="([^"]+)"') {
            $digiKeyPN = $Matches[1]
        }
        # Pattern 2: Try product detail area
        elseif ($html -match 'DigiKey Part Number[^>]*>([^<]+)<') {
            $digiKeyPN = $Matches[1].Trim()
        }
        # Pattern 3: Try alternative pattern
        elseif ($html -match 'data-digi-key-part-number="([^"]+)"') {
            $digiKeyPN = $Matches[1]
        }
        else {
            $digiKeyPN = "MANUAL_LOOKUP_REQUIRED"
        }
        
        # Try to extract price (unit price)
        $price = "N/A"
        if ($html -match '\$(\d+\.\d{2})\s*(?:USD|each)') {
            $price = "`$$($Matches[1])"
        }
        
        # Try to extract stock status
        $stock = "Check DigiKey"
        if ($html -match '(\d{1,}[,\d]*)\s*(?:-\s*)?in\s+stock', [System.Text.RegularExpressions.RegexOptions]::IgnoreCase) {
            $stock = $Matches[1]
        }
        
        $results += [PSCustomObject]@{
            Designator = $part.Designator
            Description = $part.Description
            MPN = $part.MPN
            DigiKeyPN = $digiKeyPN
            Price = $price
            Stock = $stock
            URL = $part.DigiKeyURL
        }
        
        Write-Host "  -> DigiKey PN: $digiKeyPN | Price: $price | Stock: $stock" -ForegroundColor Green
        
    } catch {
        Write-Host "  ERROR: Failed to fetch - $($_.Exception.Message)" -ForegroundColor Red
        $results += [PSCustomObject]@{
            Designator = $part.Designator
            Description = $part.Description
            MPN = $part.MPN
            DigiKeyPN = "FETCH_FAILED"
            Price = "N/A"
            Stock = "N/A"
            URL = $part.DigiKeyURL
        }
    }
    
    # Rate limit: wait 2 seconds between requests
    Start-Sleep -Seconds 2
}

Write-Host ""
Write-Host "=== Fetched Results ===" -ForegroundColor Cyan
$results | Format-Table -AutoSize

# Save results to CSV
$outputPath = Join-Path $PSScriptRoot "digikey-connector-results.csv"
$results | Export-Csv -Path $outputPath -NoTypeInformation -Encoding UTF8
Write-Host "Results saved to: $outputPath" -ForegroundColor Green

# Now update bom-connectors-detailed.csv with DigiKey PNs
Write-Host ""
Write-Host "=== Updating bom-connectors-detailed.csv ===" -ForegroundColor Cyan

$bomDetailedPath = Join-Path $PSScriptRoot "bom-connectors-detailed.csv"
if (Test-Path $bomDetailedPath) {
    $bomDetailed = Import-Csv $bomDetailedPath -Encoding UTF8
    
    foreach ($row in $bomDetailed) {
        $match = $results | Where-Object { $_.Designator -eq $row.Designator }
        if ($match) {
            $row.DigiKey_PN = $match.DigiKeyPN
            Write-Host "  Updated $($row.Designator): DigiKey_PN = $($match.DigiKeyPN)" -ForegroundColor Yellow
        }
    }
    
    # Save updated BOM
    $bomDetailed | Export-Csv -Path $bomDetailedPath -NoTypeInformation -Encoding UTF8
    Write-Host "Updated BOM saved to: $bomDetailedPath" -ForegroundColor Green
} else {
    Write-Host "WARNING: bom-connectors-detailed.csv not found at $bomDetailedPath" -ForegroundColor Red
}

# Also update bom-manufacturable.csv if it exists
$bomManufPath = Join-Path $PSScriptRoot "bom-manufacturable.csv"
if (Test-Path $bomManufPath) {
    Write-Host ""
    Write-Host "=== Updating bom-manufacturable.csv ===" -ForegroundColor Cyan
    
    $bomManuf = Import-Csv $bomManufPath -Encoding UTF8
    
    foreach ($row in $bomManuf) {
        $match = $results | Where-Object { $_.Designator -eq $row.Designator }
        if ($match) {
            $row.DigiKey_PN = $match.DigiKeyPN
            Write-Host "  Updated $($row.Designator): DigiKey_PN = $($match.DigiKeyPN)" -ForegroundColor Yellow
        }
    }
    
    # Save updated BOM
    $bomManuf | Export-Csv -Path $bomManufPath -NoTypeInformation -Encoding UTF8
    Write-Host "Updated BOM saved to: $bomManufPath" -ForegroundColor Green
}

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Cyan
Write-Host "Next steps:" -ForegroundColor Gray
Write-Host "1. Review digikey-connector-results.csv for accuracy" -ForegroundColor Gray
Write-Host "2. Manually verify any 'MANUAL_LOOKUP_REQUIRED' or 'FETCH_FAILED' entries on DigiKey.com" -ForegroundColor Gray
Write-Host "3. Check bom-connectors-detailed.csv and bom-manufacturable.csv for updated DigiKey_PN columns" -ForegroundColor Gray
