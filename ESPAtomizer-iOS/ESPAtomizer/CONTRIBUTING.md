# Contributing

Short guide for contributors who want to run or modify the iOS app locally.

Getting started
1. Fork the repo and clone your fork.
2. Create a feature branch for your changes: `git checkout -b feature/your-feature`.

Local setup
- Open `ESPAtomizer-iOS.xcodeproj` in Xcode.
- Select a Team in *Signing & Capabilities*. If you don't have an Apple Developer account, you can still run on a personal test device using a free account (Xcode will guide you).

Running the app
- Choose a simulator or connect a device and press Run.
- The app requests Bluetooth permissions at runtime; grant them when prompted.

Testing & PRs
- Keep changes small and focused. Include short, descriptive commit messages.
- Open a Pull Request against `main` and include a brief description of your changes and testing steps.

Maintainers
- Adamibus
