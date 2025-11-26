<#
export_ios_repo.ps1

Copies the existing `ESPAtomizer-iOS` folder into a standalone export folder,
initializes a git repo, and creates a helpful README and .gitignore.

Usage (PowerShell):
  Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
  .\export_ios_repo.ps1 -Destination "C:\path\to\export\ESPAtomizer-iOS-Repo" -InitGit

Parameters:
  -Destination: path where the exported repo will be created. Defaults to a
                new folder next to this workspace named `ESPAtomizer-iOS-Repo`.
  -Source:      path to the source `ESPAtomizer-iOS` folder. Defaults to the
                sibling folder in the workspace.
  -InitGit:     switch. If present, run `git init` and make an initial commit.
  -CreateRemote: optional URL to add as `origin` (e.g. https://github.com/you/repo.git).
                 If provided and `git` is available, this will set remote but
                 will NOT push automatically.
#>
param(
    [string]$Destination = "${PWD}\..\ESPAtomizer-iOS-Repo",
    [string]$Source = "${PWD}\ESPAtomizer-iOS",
    [switch]$InitGit,
    [string]$CreateRemote
)

Write-Host "Source: $Source"
Write-Host "Destination: $Destination"

if (-not (Test-Path $Source)) {
    Write-Error "Source folder not found: $Source"
    exit 1
}

if (Test-Path $Destination) {
    Write-Host "Destination already exists. Backing up by appending .bak"
    $Destination = $Destination + ".bak"
    $i = 1
    while (Test-Path $Destination) {
        $Destination = $Destination + "_$i"
        $i++
    }
}

New-Item -ItemType Directory -Path $Destination -Force | Out-Null

Write-Host "Copying files..."
# Use robocopy to preserve metadata where possible; fallback to Copy-Item
$robocopy = Get-Command robocopy -ErrorAction SilentlyContinue
if ($robocopy) {
    # /E copy subdirs including empty ones; /NFL /NDL reduce logging; /NJH /NJS avoid headers
    $rc = robocopy $Source $Destination /E /NFL /NDL /NJH /NJS
    if ($LASTEXITCODE -ge 8) {
        Write-Warning "robocopy reported an error (exit $LASTEXITCODE). Some files may be missing."
    }
} else {
    Copy-Item -Path (Join-Path $Source '*') -Destination $Destination -Recurse -Force
}

# Create a simple .gitignore for Xcode / Swift
$gitignorePath = Join-Path $Destination ".gitignore"
@"
# Xcode
build/
DerivedData/
*.xcworkspace
xcuserdata/
*.moved-aside
*.xcuserstate

# Swift Package Manager
.build/

# CocoaPods
Pods/

# Carthage
Carthage/Build/

# Misc
.DS_Store
*.swp
.idea/
.vscode/
"@ | Out-File -FilePath $gitignorePath -Encoding utf8

# Create an export README (if not present, we'll create a small note — main README is created separately)
$exportNotePath = Join-Path $Destination "EXPORT_NOTE.txt"
"Exported iOS app code from workspace. See README.md for push instructions." | Out-File -FilePath $exportNotePath -Encoding utf8

if ($InitGit) {
    Write-Host "Initializing git repository..."
    Push-Location $Destination
    git init | Out-Null
    git add .
    git commit -m "Initial import: ESPAtomizer iOS app" | Out-Null
    if ($CreateRemote) {
        git remote add origin $CreateRemote
        Write-Host "Set remote origin to $CreateRemote (no push performed)."
    }
    Pop-Location
    Write-Host "Git repo initialized at: $Destination"
} else {
    Write-Host "Skipped git init (pass -InitGit to initialize).
To initialize the repo manually run:
  cd '$Destination'
  git init
  git add .
  git commit -m 'Initial import: ESPAtomizer iOS app'
"
}

Write-Host "Export complete. Open the destination in Finder/Explorer or Xcode."