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

Simulator fixes

- App not appearing in Simulator or crashes immediately:
  - Quit the Simulator app, then in Terminal run `xcrun simctl shutdown all` and relaunch Simulator from Xcode.
  - In Xcode choose *Device* → *Erase All Content and Settings...* for the target simulator to clear stale state.
  - Delete the app from the simulator home screen and reinstall (Run from Xcode).

- Simulator shows wrong device or orientation:
  - In Xcode select the desired simulator from the device menu (near the Run button). Use *Window > Scale* in Simulator to adjust size.

- Simulator hangs or Xcode build fails for simulator only:
  - Clean the build folder: *Product > Clean Build Folder*.
  - Reset DerivedData: close Xcode, then run `rm -rf ~/Library/Developer/Xcode/DerivedData/*` and re-open the project.

Provisioning profiles (step-by-step)

1. Create an App ID in Apple Developer:
	- Sign in to the Apple Developer portal, go to *Certificates, Identifiers & Profiles* → *Identifiers* → `+` to add an App ID.
	- Enter a descriptive name and the bundle identifier (e.g. `com.yourcompany.ESPAtomizer`).

2. Create a signing certificate (if you don't have one):
	- In *Certificates* select `+` and follow the instructions to create an Apple Development certificate (you'll need to use Keychain Access to generate a CSR).

3. Create a provisioning profile:
	- In *Profiles* click `+`, choose *iOS App Development*, select the App ID you created, choose your development certificate, select the test device(s) (or skip for automatic device inclusion), and download the profile.

4. Install certificate & profile:
	- Double-click the downloaded certificate and provisioning profile to add them to Keychain and Xcode respectively. Xcode should then list the profile under *Preferences > Accounts > Manage Certificates*.

5. In Xcode set signing:
	- Open the project, select the target and *Signing & Capabilities*.
	- Choose your Team. If the provisioning profile and certificate are correctly installed, Xcode will automatically select a profile or allow you to pick one.

6. Troubleshooting provisioning:
	- If Xcode reports a missing provisioning profile, ensure the App ID and bundle identifier match exactly between `Info.plist` and the profile.
	- If a device is not listed, add its UDID in the Developer portal under *Devices* and re-create the profile including that device.
	- For simpler local development, keep *Automatically manage signing* enabled so Xcode can create profiles for you.

