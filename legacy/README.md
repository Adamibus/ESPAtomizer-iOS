# ESPAtomizer

ESPAtomizer is a multi-part project for a compact atomizer controller built around an ESP32-based firmware stack, a companion iOS app, and PCB/hardware work.

## What’s in this repo

- `ESPAtomizer/` - Arduino firmware for the device
- `ESPAtomizer-iOS/` - iOS companion app
- `hardware/` - PCB design files, footprints, and board notes
- `docs/` - build notes, reviews, wiring guides, and reports
- `tools/` - helper scripts for checks and analysis

## Quick start

- Open `ESPAtomizer/ESPAtomizer.ino` to work on the firmware.
- Read `docs/README-ESP32C6.md` for the current wiring and board notes.
- Use `tools/README.md` for available project checks and utility scripts.

## Helpful checks

If you want to validate the firmware and config without hardware, the repo includes utility scripts under `tools/` such as the smoke check and config lint scripts.

## Safe Git workflow

If you are working from this repo, use the tracked branch on GitHub and keep pulls rebased:

```powershell
git branch --set-upstream-to=origin/main main
git config pull.rebase true
git config push.default upstream
```

Typical update flow:

```powershell
git add -A
git commit -m "Describe your change"
git pull --rebase
git push
```

## Notes

This repo contains active development files, backups, and generated artifacts. Keep that in mind when reviewing diffs or preparing releases.
