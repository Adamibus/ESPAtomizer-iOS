# ESPAtomizer iOS

Quick setup notes for opening and building the iOS app in Xcode.

Prerequisites
- Xcode 15+ (or the latest stable Xcode on your Mac)
- An Apple Developer account (for signing and running on devices)

Open the project
1. Clone the repo or pull the latest changes.
2. Open `ESPAtomizer-iOS.xcodeproj` in Xcode.

Signing & team
- Select the project in the Project navigator, then the target `ESPAtomizer_iOSApp`.
- In *Signing & Capabilities* set your *Team* (select your Apple Developer team).
- The project uses `CODE_SIGN_STYLE = Automatic`. If you prefer manual signing, change it in Xcode.
- Replace the placeholder `DEVELOPMENT_TEAM` in `project.pbxproj` with your Team ID if you want it pre-configured.

Bundle identifier
- The `Info.plist` currently uses `com.adamibus.ESPAtomizerIOS`. Change `CFBundleIdentifier` to your preferred reverse-DNS id (for example `com.yourcompany.ESPAtomizer`).

App icon
- An `Assets.xcassets` catalog with an `AppIcon` set was added and contains `AppIconTinted.png`.
- For best results add appropriately sized icons for each slot (iPhone/iPad @1x/@2x/@3x).

Run on device
- Connect an iPhone, select it as the build target, and press Run.
- If Xcode prompts about a provisioning profile or trust, follow the prompts to register the device.

Info & permissions
- `Info.plist` includes Bluetooth usage strings: `NSBluetoothAlwaysUsageDescription` and `NSBluetoothPeripheralUsageDescription`.

Notes
- The project file has common defaults (iOS 16.0 deployment target, Swift 5, automatic signing placeholders). Adjust as needed in Xcode.
- If you see warnings about `SceneDelegate`, they were removed to use the SwiftUI `@main` lifecycle.

Contact
- If you run into issues opening or building the project, open an issue in the repository or contact Adamibus.

Troubleshooting

- Signing / Provisioning errors:
	- Symptom: Xcode shows "No provisioning profile" or "Signing for \"ESPAtomizer_iOSApp\" requires a development team."  
	- Fix: Open the project, select the target, go to *Signing & Capabilities* and pick your *Team*. If you prefer automatic signing keep *Automatically manage signing* checked. If you have a Team ID, you can replace `DEVELOPMENT_TEAM` in `project.pbxproj` with your 10-char Team ID.

- App icon missing / placeholders:
	- Symptom: App icon is blank or shows a default placeholder on device/simulator.  
	- Fix: Open `Assets.xcassets` -> `AppIcon` in Xcode and ensure images are present for the required slots. You can add scaled versions of `AppIconTinted.png` for @1x/@2x/@3x.

- SceneDelegate / lifecycle warnings:
	- Symptom: Xcode warns about `SceneDelegate` or mismatched lifecycle.
	- Fix: This project uses SwiftUI `@main` app entry (`ESPAtomizer_iOSApp.swift`). Remove `UIApplicationSceneManifest`/SceneDelegate references from `Info.plist` (already removed in this repo). If you switch to a UIKit lifecycle, add a `SceneDelegate` implementation.

- Build failures / stale state:
	- Symptom: Random build errors after changing project settings or files.
	- Fix: Use *Product > Clean Build Folder* in Xcode, then rebuild. If problems persist, delete DerivedData: `rm -rf ~/Library/Developer/Xcode/DerivedData/*` and re-open the project.

- Missing or outdated settings after pulling changes:
	- Symptom: After pulling, Xcode shows unexpected settings or files missing.
	- Fix: Pull latest changes, then close and re-open Xcode. Re-run *File > Packages > Reset Package Caches* if using Swift packages. For signing changes, re-select the Team in *Signing & Capabilities*.

- Permission prompts for Bluetooth:
	- Symptom: App appears not to access Bluetooth or gets denied permission.
	- Fix: Check device *Settings > Privacy & Security > Bluetooth* to ensure the app is allowed. The app shows prompts due to `NSBluetoothAlwaysUsageDescription` in `Info.plist`—grant permission when asked.

If a problem isn't covered here, create an issue in the repo with the Xcode build log and screenshots and I'll assist.
