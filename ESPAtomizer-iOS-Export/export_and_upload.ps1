<#
export_and_upload.ps1

Exports the `ESPAtomizer-iOS` app to a destination folder, initializes a git repo,
creates a GitHub repository (public/private) and optionally pushes the initial commit.

Requirements:
- PowerShell 5+ on Windows
- `git` in PATH
- Either the `gh` CLI (recommended) OR a GitHub personal access token in env var GITHUB_TOKEN
  (scopes: repo for private repos or public_repo for public repos; org:repo if creating under org).

Usage examples:
# Basic export and init only (no remote)
.\export_and_upload.ps1 -Destination "C:\temp\ESPAtomizer-iOS-Repo" -InitGit

# Export and create+push using gh CLI
.\export_and_upload.ps1 -Destination "C:\temp\ESPAtomizer-iOS-Repo" -InitGit -UseGH -Visibility public -RepoName ESPAtomizer-iOS

# Export and create+push using env GITHUB_TOKEN (no gh)
$env:GITHUB_TOKEN = "ghp_xxx..."
.\export_and_upload.ps1 -Destination "C:\temp\ESPAtomizer-iOS-Repo" -InitGit -RepoName ESPAtomizer-iOS -Visibility private -Push

# Export and push to existing remote URL
.\export_and_upload.ps1 -Destination "C:\temp\ESPAtomizer-iOS-Repo" -InitGit -RemoteUrl "https://github.com/you/ESPAtomizer-iOS.git" -Push

Parameters:
-Source: Path to the local `ESPAtomizer-iOS` folder (default: sibling `ESPAtomizer-iOS`).
-Destination: Where to write the exported repo.
-InitGit: initialize git and make initial commit.
-RepoName: desired GitHub repo name (if creating on GitHub).
-Owner: GitHub owner (username or org). If omitted, creates under authenticated user.
-Visibility: public or private (default public).
-UseGH: use GitHub CLI (`gh`) to create the repo and push (preferred if available).
-GitHubToken: use this token to create the repo via REST API (or set env GITHUB_TOKEN).
-RemoteUrl: use this remote URL instead of creating a new remote.
-Push: push initial commit to remote after setting remote.

Security note: avoid embedding long-lived tokens in scripts. Prefer the `gh` CLI or short-lived tokens.
#>
param(
    [string]$Source = "${PWD}\..\ESPAtomizer-iOS",
    [string]$Destination = "${PWD}\..\ESPAtomizer-iOS-Repo",
    [switch]$InitGit,
    [string]$RepoName,
    [string]$Owner,
    [ValidateSet('public','private')][string]$Visibility = 'public',
    [switch]$UseGH,
    [string]$GitHubToken,
    [string]$RemoteUrl,
    [switch]$Push
)

function Write-ErrAndExit($m) { Write-Error $m; exit 1 }

Write-Host "Source: $Source"
Write-Host "Destination: $Destination"

if (-not (Test-Path $Source)) { Write-ErrAndExit "Source folder not found: $Source" }

# Ensure destination unique
if (Test-Path $Destination) {
    Write-Host "Destination exists; appending .bak to avoid overwrite"
    $base = $Destination
    $i = 1
    do { $Destination = "$base.bak" + (if ($i -gt 1) { "_$i" } else { "" }); $i++ } while (Test-Path $Destination)
}
New-Item -ItemType Directory -Path $Destination -Force | Out-Null

Write-Host "Copying files from $Source to $Destination..."
$robocopy = Get-Command robocopy -ErrorAction SilentlyContinue
if ($robocopy) {
    robocopy $Source $Destination /E /NFL /NDL /NJH /NJS | Out-Null
    if ($LASTEXITCODE -ge 8) { Write-Warning "robocopy reported exit code $LASTEXITCODE (some files may not have copied)." }
} else {
    Copy-Item -Path (Join-Path $Source '*') -Destination $Destination -Recurse -Force
}

# Ensure .gitignore exists (don't overwrite if user provided their own)
$gitignore = Join-Path $Destination '.gitignore'
if (-not (Test-Path $gitignore)) {
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
"@ | Out-File -FilePath $gitignore -Encoding utf8
}

if ($InitGit) {
    Push-Location $Destination
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { Pop-Location; Write-ErrAndExit 'git not found in PATH'; }
    Write-Host "Initializing git repository..."
    git init | Out-Null
    git add .
    git commit -m "Initial import: ESPAtomizer-iOS" | Out-Null

    # Determine remote strategy
    if ($RemoteUrl) {
        git remote add origin $RemoteUrl
        Write-Host "Set remote origin -> $RemoteUrl"
        if ($Push) {
            Write-Host "Pushing to remote (branch: main)..."
            git branch -M main
            git push -u origin main
            Write-Host "Pushed to $RemoteUrl"
        }
        Pop-Location
        return
    }

    # Prefer gh CLI if requested and available
    if ($UseGH -and (Get-Command gh -ErrorAction SilentlyContinue)) {
        if (-not $RepoName) { Write-ErrAndExit 'RepoName required when using gh.' }
        $ghArgs = @('repo','create',$RepoName)
        if ($Owner) { $ghArgs += "--owner"; $ghArgs += $Owner }
        $ghArgs += "--$Visibility"
        $ghArgs += "--source"; $ghArgs += "$Destination"
        $ghArgs += "--remote"; $ghArgs += "origin"
        $ghArgs += "--push"
        Write-Host "Running: gh $($ghArgs -join ' ')"
        gh @ghArgs
        Pop-Location
        return
    }

    # If GitHub token supplied (or env var set), use REST API to create repo
    $token = $GitHubToken
    if (-not $token) { $token = $env:GITHUB_TOKEN }
    if ($token) {
        if (-not $RepoName) { Write-ErrAndExit 'RepoName required to create GitHub repo via API.' }
        $apiUrlBase = 'https://api.github.com'
        # If Owner provided and is org, create in org endpoint
        if ($Owner) {
            $createUrl = "$apiUrlBase/orgs/$Owner/repos"
        } else {
            $createUrl = "$apiUrlBase/user/repos"
        }
        $body = @{ name = $RepoName; private = ($Visibility -eq 'private') } | ConvertTo-Json
        Write-Host "Creating GitHub repo via API at $createUrl"
        try {
            $hdr = @{ Authorization = "token $token"; Accept = 'application/vnd.github.v3+json' }
            $resp = Invoke-RestMethod -Method Post -Uri $createUrl -Headers $hdr -Body $body
        } catch {
            Pop-Location
            Write-ErrAndExit "GitHub API request failed: $_"
        }
        # resp.ssh_url or resp.clone_url
        $cloneUrl = $resp.clone_url
        if (-not $cloneUrl) { Pop-Location; Write-ErrAndExit 'Failed to determine clone URL from GitHub response.' }
        git remote add origin $cloneUrl
        Write-Host "Set remote origin -> $cloneUrl"
        if ($Push) {
            Write-Host "Pushing initial commit to origin/main..."
            git branch -M main
            git push -u origin main
            Write-Host "Pushed to $cloneUrl"
        }
        Pop-Location
        return
    }

    # If we reach here, no remote provided, gh not used, token not provided: we are done
    Pop-Location
    Write-Host "Git initialized locally at $Destination. No remote configured. To add a remote and push later run:
  cd $Destination
  git remote add origin <remote-url>
  git branch -M main
  git push -u origin main"
} else {
    Write-Host "Skipped git init. Run with -InitGit to initialize a local git repo."
}
