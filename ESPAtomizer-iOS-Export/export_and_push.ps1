param(
  [Alias("AtomizerAPP")][Parameter(Mandatory=$true)][string]$RepoName,
  [string]$Destination = "$env:USERPROFILE\Desktop\$RepoName",
  [string]$Source = (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Definition) '..\ESPAtomizer-iOS' | Resolve-Path -ErrorAction SilentlyContinue),
  [string]$RemoteUrl = "",
  [switch]$UseGH,
  [ValidateSet("public","private")][string]$Visibility = "public",
  [switch]$InitGit = $true,
  [switch]$Push = $true,
  [string]$Email = ""
)

function Write-Log { param($m) Write-Host "[EXPORT] $m" }

# Resolve source
if (-not $Source) {
  $Source = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Definition) '..\ESPAtomizer-iOS'
}
$srcResolved = Resolve-Path $Source -ErrorAction SilentlyContinue
if (-not $srcResolved) { Write-Error "Source folder not found: $Source. Edit -Source. Exiting."; exit 1 }
$Source = $srcResolved.Path

# Create destination
if (-not (Test-Path $Destination)) {
  New-Item -ItemType Directory -Path $Destination | Out-Null
}
Write-Log "Copying from $Source -> $Destination"

# Copy files (robocopy for reliability on Windows)
if (Get-Command robocopy -ErrorAction SilentlyContinue) {
  robocopy $Source $Destination /MIR /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
} else {
  Write-Log "robocopy not found; using Copy-Item"
  Copy-Item -Path (Join-Path $Source '*') -Destination $Destination -Recurse -Force
}

# Create Xcode/Swift .gitignore if not present
$gitignorePath = Join-Path $Destination '.gitignore'
if (-not (Test-Path $gitignorePath)) {
  Write-Log "Writing Xcode/Swift .gitignore"
  @"
# Xcode
.DS_Store
build/
DerivedData/
*.xcuserstate
*.xcworkspace/contents.xcworkspacedata

# Swift Package Manager
.build/
Packages/
*.xcodeproj/project.xcworkspace/xcshareddata/swiftpm

# CocoaPods
Pods/

# Carthage
Carthage/Build/

# User settings
*.pbxuser
*.mode1v3
*.mode2v3
*.perspectivev3

# Swift Package Manager
Package.resolved

# Xcode Archives
*.xcarchive

# Playgrounds
timeline.xctimeline
playground.xcworkspace

# Other
tmp/
.idea/
.vscode/
xcuserdata/
"@ | Out-File -FilePath $gitignorePath -Encoding UTF8
}

Push-Location $Destination

# Init git if requested
if ($InitGit -and -not (Test-Path (Join-Path $Destination '.git'))) {
  Write-Log "Initializing git repository"
  git init -q
  git checkout -b main 2>$null
  git config user.name "Adamibus"
  if ($Email) { git config user.email $Email }
}

# Stage and commit
Write-Log "Staging files"
git add --all
$hasChanges = ($(git status --porcelain) -ne "")
if ($hasChanges) {
  git commit -m "Add iOS app" -q
  Write-Log "Committed export"
} else {
  Write-Log "No changes to commit"
}

# Create remote via gh if requested
if ($UseGH) {
  if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    Write-Error "gh CLI not found. Install and run 'gh auth login' first."
    Pop-Location; exit 1
  }
  $fullName = "Adamibus/$RepoName"
  Write-Log "Creating repo via gh: $fullName ($Visibility)"
  gh repo create $fullName --$Visibility --source . --remote origin --push --confirm
  Write-Log "Repository created and pushed via gh"
  Pop-Location
  exit 0
}

# If RemoteUrl provided, set or update origin
if ($RemoteUrl) {
  $existingRemote = git remote get-url origin 2>$null
  if ($existingRemote) {
    if ($existingRemote -ne $RemoteUrl) {
      Write-Log "Updating origin URL to $RemoteUrl"
      git remote set-url origin $RemoteUrl
    } else {
      Write-Log "Origin already set to $RemoteUrl"
    }
  } else {
    Write-Log "Adding remote origin $RemoteUrl"
    git remote add origin $RemoteUrl
  }
}

# Push if requested
if ($Push) {
  if (-not (git remote)) {
    Write-Error "No remote configured. Use -RemoteUrl or -UseGH to provide a remote."
    Pop-Location; exit 1
  }
  Write-Log "Pushing to origin main"
  git push -u origin main
  if ($LASTEXITCODE -ne 0) {
    Write-Log "Push failed. Try: git pull --rebase origin main then push, or push to a new branch."
    Pop-Location; exit 1
  }
  Write-Log "Push completed"
}

Pop-Location
Write-Log "Export complete: $Destination"