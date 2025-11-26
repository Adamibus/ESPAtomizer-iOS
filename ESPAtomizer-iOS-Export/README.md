ESPAtomizer iOS Export

This folder contains helper files to export the `ESPAtomizer-iOS` app into a standalone
Git repository that can be opened in Xcode or shared independently.

Files
- `export_ios_repo.ps1` — PowerShell helper that copies `ESPAtomizer-iOS` into a new
  destination folder and (optionally) initializes a local Git repository with an
  initial commit. It also creates a `.gitignore` tuned for Xcode/Swift projects.

How to use
1. Open PowerShell and change to the workspace folder (where this script lives):

```powershell
cd "c:\Users\adinj\OneDrive\Projects\Coding\ESPAtomizer"
```

2. Run the export script. Example (create export next to workspace and initialize Git):

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\ESPAtomizer-iOS-Export\export_ios_repo.ps1 -Destination "c:\Users\adinj\Desktop\ESPAtomizer-iOS-Repo" -InitGit
```

3. (Optional) Create a remote repository on GitHub (via the website) or with the GitHub CLI:

```powershell
# Using GitHub CLI (gh). Install 'gh' and authenticate first.
gh repo create your-username/ESPAtomizer-iOS --public --source "c:\Users\adinj\Desktop\ESPAtomizer-iOS-Repo" --remote origin --push
```

If you prefer not to push automatically, you can instead add the remote URL to the local repo:

```powershell
cd "c:\Users\adinj\Desktop\ESPAtomizer-iOS-Repo"
git remote add origin https://github.com/your-username/ESPAtomizer-iOS.git
git push -u origin main
```

4. Open the exported project in Xcode:
- If the project uses an `.xcworkspace`, open that file; otherwise open the `.xcodeproj`.
- Set your signing & team in the Xcode project settings if required.

Notes & next steps
- The script avoids pushing to any remote automatically unless you provide a remote URL
  or explicitly use `gh repo create --push`.
- If you'd like, I can:
  - Run the export script here and prepare a `.zip` you can download (requires permission),
  - Or create the GitHub repo for you if you provide a GitHub token/authorization method.

Contact me with the export path you used or the GitHub repo details and I will help
finish the remote setup and README polish for the iOS repo.