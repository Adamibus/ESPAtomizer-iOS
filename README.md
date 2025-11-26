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
