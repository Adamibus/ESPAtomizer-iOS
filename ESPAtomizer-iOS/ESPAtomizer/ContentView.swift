//
//  ContentView.swift
//  ESPAtomizer-iOS
//
//  Monolithic single-file UI: ContentView + subviews
//  Created by ChatGPT and Adam Dinjian
//

import SwiftUI
import CoreBluetooth

// ---
// UI Architecture Note:
//
// The HOME page is a fixed (non-scrolling) VStack: everything on it fits the
// screen, so it must never move. Only the Settings sheet scrolls — it owns its
// own ScrollView. Tab views (AutoModeView, ManualModeView, ConfigView) are plain
// VStacks with no ScrollView of their own.
//
// When adding home-page content, keep the page within one screen height (trim
// spacing or chart height rather than re-introducing a ScrollView).
// ---

// ============================================================================
// 🎨 UI-CUSTOMIZE  —  START HERE TO MAKE THIS APP LOOK LESS "DEFAULT/AI-GENERATED"
// ============================================================================
// Every spot in this file worth restyling is tagged with the marker:  🎨 UI-CUSTOMIZE
// Search the project for "🎨 UI-CUSTOMIZE" (Cmd-Shift-F in Xcode) to jump between them.
// See README.md ("UI Customization Guide") for a plain-English walkthrough of each one.
//
// The three highest-impact changes:
//   1. BRAND NAME + TITLE ....... AtomizerViewModel.swift  →  `appTitle` / `appName`
//   2. ACCENT COLOR ............. replace the many hard-coded `.blue` / `.green` /
//                                 `.orange` / `.red` calls with a single shared color.
//                                 Define one below in `Theme` and use `Theme.accent`.
//   3. CARD LOOK ................ the `.background(Theme.card)` +
//                                 `.cornerRadius(...)` pattern repeated on every panel.
//
// Tip: SwiftUI's stock look comes from (a) system fonts like `.largeTitle`/`.title2`,
// (b) system colors like `.systemGray6`, and (c) sharp default corners. Change those
// three things (custom font, a brand palette, softer shadows/corners) and the app
// immediately stops looking like a template.
//
// One place for every color, size and spacing value in the app.
// Change a ROLE (accent/card/bg/ok…) to restyle globally; the raw palette below
// is what those roles are picked from.
enum Theme {

    // MARK: Palette
    static let charcoal     = Color(red: 0.16, green: 0.17, blue: 0.19)   // #2A2C31
    static let charcoalLift = Color(red: 0.20, green: 0.21, blue: 0.24)   // cards on charcoal
    static let charcoalDeep = Color(red: 0.09, green: 0.10, blue: 0.12)   // app background / inset wells
    // Electric azure — a CHART color only, never a status color. Cyan-shifted so
    // it reads neon against charcoal without a full-brightness cyan's buzz.
    static let azure        = Color(red: 0.38, green: 0.78, blue: 1.00)   // #61C7FF
    static let azureGlow    = Color(red: 0.58, green: 0.87, blue: 1.00)   // #94DEFF
    static let rose         = Color(red: 0.91, green: 0.44, blue: 0.51)   // #E87082 warm reddish-pink; soft but still saturated enough to read on charcoal
    static let gold         = Color(red: 0.89, green: 0.76, blue: 0.43)   // #E3C26E champagne
    static let cream        = Color(red: 0.96, green: 0.94, blue: 0.90)   // #F5F0E6
    static let creamMuted   = Color(red: 0.96, green: 0.94, blue: 0.90).opacity(0.55)
    static let plum         = Color(red: 0.66, green: 0.53, blue: 0.88)   // #A887E0 desaturated to match gold's softness
    static let emerald      = Color(red: 0.29, green: 0.80, blue: 0.55)   // #4ACC8C desaturated to match gold's softness

    /// The three choices for the temperature line, in swatch order — evenly
    /// spaced across the picker. Gold sits in the MIDDLE and is the default
    /// (AtomizerViewModel.chartColorIndex = 1); white and emerald flank it.
    /// Persisted by INDEX: reorder this array and you silently change what
    /// existing installs are showing — see the configVersion migration if you do.
    static let chartSeriesColors: [Color] = [cream, gold, emerald]

    /// Measured values — the live temperature line and its readout. Neutral gray
    /// on purpose: the swatch colors what you SET (target), so what the device
    /// REPORTS must not compete with it.
    static let measured = Color(red: 0.62, green: 0.63, blue: 0.66)

    /// The selected series color, clamped. Use this everywhere the choice is
    /// read — the chart lines, the output strip, and the sliders that drive
    /// them — so a control can't be left behind on the old accent.
    static func seriesColor(_ index: Int) -> Color {
        chartSeriesColors[min(max(0, index), chartSeriesColors.count - 1)]
    }

    /// The swatch color, made safe to sit UNDER the system Toggle's knob — which
    /// is always white and cannot be restyled. The cream swatch is very nearly
    /// white, so used raw it recreates exactly the invisible white-on-white
    /// switch this palette was changed to avoid. Anything brighter than the
    /// threshold is scaled down just far enough for the knob to read; every
    /// other swatch passes through untouched.
    static func controlFill(_ index: Int) -> Color {
        let c = seriesColor(index)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        UIColor(c).getRed(&r, green: &g, blue: &b, alpha: &a)
        let luminance = 0.299 * r + 0.587 * g + 0.114 * b
        let ceiling: CGFloat = 0.72
        guard luminance > ceiling else { return c }
        let k = ceiling / luminance
        return Color(red: Double(r * k), green: Double(g * k), blue: Double(b * k))
    }

    // MARK: Roles — change these to restyle the whole app
    static let accent   = gold           // small highlights + primary actions ONLY, never large text
    static let bg       = charcoalDeep   // screen background
    static let card     = charcoalLift   // panel background
    static let inset    = charcoalDeep   // wells inside a card: fields, segmented track
    static let text     = cream
    static let textDim  = creamMuted
    // Connected / power on / healthy. Deliberately NOT a hue: status reads as
    // bright-vs-dim cream against `textDim`, so nothing in the chrome competes
    // with the swatch color or looks arbitrarily blue.
    static let ok       = cream
    static let warn     = Color(red: 0.90, green: 0.55, blue: 0.20)
    static let danger   = Color(red: 0.85, green: 0.28, blue: 0.28)
    static let hairline = cream.opacity(0.12)

    // MARK: Shape
    static let corner: CGFloat = 14
    static let cornerSm: CGFloat = 9      // fields, segments, small chips

    // MARK: Spacing scale — use these instead of ad-hoc numbers
    static let sBlock: CGFloat = 20   // between major sections
    static let sCard:  CGFloat = 14   // inside a card
    static let sRow:   CGFloat = 8    // between related rows
    static let sTight: CGFloat = 4    // label above value
    static let pad:    CGFloat = 16   // card inner padding
    static let fieldW: CGFloat = 96   // every numeric entry field, same width
}

// MARK: - Type scale. Use these, never ad-hoc fonts, so the app reads as one voice.
extension Text {
    /// SECTION HEADER — quiet uppercase label, like iOS Settings group headers.
    func sectionHeader() -> some View {
        self.font(.caption).fontWeight(.semibold)
            .textCase(.uppercase).kerning(1.1)
            .foregroundColor(Theme.textDim)
    }
    /// STAT LABEL — small label above a value.
    func statLabel() -> some View {
        self.font(.caption2).fontWeight(.medium)
            .textCase(.uppercase).kerning(0.8)
            .foregroundColor(Theme.textDim)
    }
    /// STAT VALUE — number that updates live; monospaced digits so it doesn't jitter.
    func statValue(_ color: Color = Theme.text) -> some View {
        self.font(.system(.title3, design: .rounded).weight(.semibold))
            .monospacedDigit()
            .foregroundColor(color)
    }
}

// MARK: - Button styles. Two, used everywhere: one filled primary, one quiet ghost.
struct PrimaryButtonStyle: ButtonStyle {
    var tint: Color = Theme.accent
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Theme.charcoalDeep)
            .padding(.vertical, 10).padding(.horizontal, 16)
            .background(tint.opacity(configuration.isPressed ? 0.75 : 1))
            .cornerRadius(Theme.cornerSm)
    }
}
struct GhostButtonStyle: ButtonStyle {
    var tint: Color = Theme.text
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.subheadline.weight(.medium))
            .foregroundColor(configuration.isPressed ? tint.opacity(0.5) : tint)
            .padding(.vertical, 10).padding(.horizontal, 16)
            .background(
                RoundedRectangle(cornerRadius: Theme.cornerSm)
                    .stroke(Theme.hairline, lineWidth: 1)
            )
    }
}

// MARK: - The one numeric entry row used by every PID / setpoint editor.
// Label left, dark inset field right. Kills the white UIKit boxes.
fileprivate struct NumberField: View {
    let label: String
    @Binding var text: String
    // Field-box width. Default fits the PID fields; pass something tighter for
    // short values (e.g. the device preset) so the dark box isn't oversized.
    var width: CGFloat = Theme.fieldW
    var body: some View {
        HStack {
            Text(label)
                .font(.subheadline)
                .foregroundColor(Theme.textDim)
            Spacer()
            TextField("", text: $text)
                .keyboardType(.decimalPad)
                .multilineTextAlignment(.trailing)
                .font(.body.monospacedDigit())
                .foregroundColor(Theme.text)
                .padding(.vertical, 7).padding(.horizontal, 10)
                .frame(width: width)
                .background(Theme.inset)
                .cornerRadius(Theme.cornerSm)
        }
    }
}

// Every panel in this app is padding + background + corner + shadow.
// Defined once so the cards can't drift apart again.
extension View {
    func card() -> some View {
        self
            .padding(Theme.pad)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(Theme.card)
            .cornerRadius(Theme.corner)
            .overlay(
                RoundedRectangle(cornerRadius: Theme.corner)
                    .stroke(Theme.hairline, lineWidth: 1)
            )
            .shadow(color: .black.opacity(0.25), radius: 8, y: 3)
    }
}

// MARK: - Brand header. Just the wordmark: the orb now lives on the launch
// screen, so in-app this is a plain title sitting at the top of the page.
fileprivate struct BrandHeader: View {
    let title: String
    var body: some View {
        // Same font, kerning, and centering as before — only the orb and glow
        // are gone, and the word is lifted to a title position at the top.
        Text(title)
            .font(.custom("Futura-Medium", size: 28))
            .kerning(7)
            // OVERLAY, not appended to the string: an overlay draws outside the
            // Text's layout, so the wordmark stays exactly as centered as it was
            // measured. Putting "™" in the text would widen it and shove the word.
            .overlay(alignment: .topTrailing) {
                Text("™")
                    .font(.system(size: 13, weight: .medium))
                    .foregroundColor(Theme.text.opacity(0.65))
                    .offset(x: 11, y: 2)
            }
            // Corrects for the trailing kerning SwiftUI adds after the final R.
            .offset(x: 4.3)
            .foregroundColor(Theme.text)
            .shadow(color: .black.opacity(0.55), radius: 6)
            .frame(maxWidth: .infinity)
            .padding(.top, 6)
            .padding(.bottom, 2)
            .allowsHitTesting(false)
    }
}

// MARK: - Header bar. The centered wordmark, with a settings gear on the right.
// Connection status lives inside Settings now (and the chart says "Not
// connected"), so the header stays clean — just the wordmark and the gear.
fileprivate struct HeaderBar: View {
    let title: String
    let onSettings: () -> Void

    var body: some View {
        ZStack {
            BrandHeader(title: title)
            HStack {
                Spacer()
                Button(action: onSettings) {
                    Image(systemName: "gearshape")
                        .font(.system(size: 20, weight: .regular))
                        .foregroundColor(Theme.text)
                        .frame(width: 40, height: 40)
                        .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .accessibilityLabel("Settings")
            }
        }
    }
}

// MARK: - Top-level ContentView

struct ContentView: View {
    @StateObject private var viewModel: AtomizerViewModel

    init(viewModel: AtomizerViewModel = AtomizerViewModel()) {
        _viewModel = StateObject(wrappedValue: viewModel)
    }
    
    // Shared text fields for PID editing in the "primary" area (Auto)
    @State private var kpString = ""
    @State private var kiString = ""
    @State private var kdString = ""
    
    // UI state
    @State private var selectedMode = 0
    @State private var showSettings = false
    @State private var showingError = false
    @State private var errorText: String = ""

    var body: some View {
        NavigationView {
            // FIXED page — no ScrollView. The home page fits one screen by
            // design; only the Settings sheet scrolls.
            VStack(spacing: Theme.sCard) {
                    // Brand header — the ONE place the name appears — plus the
                    // settings gear on the right.
                    HeaderBar(title: viewModel.appTitle) {
                        showSettings = true
                    }
                    StatusPanelView(viewModel: viewModel)
                    
                    // Controls and Tabs
                    VStack(alignment: .leading, spacing: Theme.sCard) {
                        // Power — the most important control, first and unmissable.
                        Toggle(isOn: $viewModel.powerToggle) {
                            Text("Power")
                                .font(.body.weight(.semibold))
                                .foregroundColor(Theme.text)
                        }
                        // On-track follows the swatch. Via controlFill, not
                        // seriesColor: the stock Toggle's knob is always white, so
                        // the near-white cream swatch has to be darkened or the
                        // switch disappears when on.
                        .tint(Theme.controlFill(viewModel.chartColorIndex))
                        .onChange(of: viewModel.powerToggle) {
                            viewModel.togglePower()
                        }

                        // Mode navigation (segmented control) + content switcher
                        VStack(spacing: Theme.sCard) {
                            HStack(spacing: 3) {
                                // Indices are the DEVICE's mode numbers (0=Auto,
                                // 1=Manual). Config moved to the Settings sheet; PID
                                // tuning went with it, so Auto is just the target now.
                                ModeButton(title: "Auto", index: 0, selected: $selectedMode)
                                ModeButton(title: "Manual", index: 1, selected: $selectedMode)
                            }
                            .padding(3)
                            .background(Theme.inset)
                            .cornerRadius(Theme.cornerSm + 3)

                            // Content area: switch on selectedMode
                            Group {
                                switch selectedMode {
                                case 0:
                                    AutoModeView(viewModel: viewModel)
                                case 1:
                                    ManualModeView(viewModel: viewModel)
                                default:
                                    Text("Unknown Mode")
                                }
                            }
                            .animation(.spring(response: 0.3, dampingFraction: 0.8), value: selectedMode)
                        }
                    }
                    .card()

                    // Connection now lives in the Settings sheet (gear, top-right).

                    Spacer(minLength: 0)
            }
            .padding(Theme.pad)
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
            .background(Theme.bg)
            // One wordmark only — the in-page header above. The nav bar is hidden so
            // no duplicate mini-title floats at the top.
            .navigationBarHidden(true)
            .onAppear {
                // Seed the PID text fields from the restored/last-known values;
                // the .onChange handlers below keep them in sync from then on.
                kpString = String(format: "%.3f", viewModel.kp)
                kiString = String(format: "%.3f", viewModel.ki)
                kdString = String(format: "%.3f", viewModel.kd)
                selectedMode = viewModel.pidMode
            }
            .onChange(of: viewModel.kp) { _, newVal in
                kpString = String(format: "%.3f", newVal)
            }
            .onChange(of: viewModel.ki) { _, newVal in
                kiString = String(format: "%.3f", newVal)
            }
            .onChange(of: viewModel.kd) { _, newVal in
                kdString = String(format: "%.3f", newVal)
            }
            .onChange(of: selectedMode) { _, newVal in
                // Tell the device only when there IS one, and never for Config
                // (read-only). Browsing tabs offline stays free: the local state
                // below still loads, so the app is fully explorable disconnected.
                // On (re)connect the device's own mode wins — `.onChange(of:
                // viewModel.pidMode)` above pulls selectedMode back into line —
                // so a tab picked while offline is never pushed as a stale command.
                if viewModel.isConnected {
                    viewModel.setMode(mode: newVal)
                }
            }
            .onChange(of: viewModel.pidMode) { _, newVal in
                // Only follow the device onto tabs this UI still has (Auto/Manual).
                // Firmware can still report U1/U2 (2/3); the hero card names the
                // mode, but there is no tab to select for it anymore.
                if newVal == 0 || newVal == 1 {
                    selectedMode = newVal
                }
            }
            .onChange(of: viewModel.lastErrorMessage) { _, newVal in
                if let m = newVal {
                    errorText = m
                    showingError = true
                }
            }
            .sheet(isPresented: $showSettings) {
                SettingsView(viewModel: viewModel,
                             kpString: $kpString, kiString: $kiString, kdString: $kdString)
                    .presentationDragIndicator(.visible)
                    .presentationBackground(Theme.bg)
            }
            .alert("Error", isPresented: $showingError, actions: {
                Button("OK", role: .cancel) { showingError = false }
            }, message: { Text(errorText) })
        } // NavigationView
    } // body

}

// MARK: - StatusPanelView

fileprivate struct StatusPanelView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    
    var body: some View {
        VStack(spacing: Theme.sCard) {
            // Fix F5: thermocouple fault banner. tcConn == false means the firmware reported
            // the thermocouple as disconnected/faulted (UUID_TC_STATUS notifies "0").
            if viewModel.status.tcConn == false {
                Label("Thermocouple disconnected", systemImage: "exclamationmark.triangle.fill")
                    .font(.caption.weight(.semibold))
                    .foregroundColor(Theme.danger)
                    .padding(.vertical, 6)
                    .padding(.horizontal, Theme.sCard)
                    .frame(maxWidth: .infinity)
                    .background(Theme.danger.opacity(0.15))
                    .cornerRadius(Theme.cornerSm)
            }

            // Hero: the live temperature, thermostat-style, with its target beneath.
            VStack(spacing: Theme.sTight) {
                // Measured temperature, so it matches the gray temperature line in
                // the chart. Dims further when power is off.
                Text(temperatureText)
                    .font(.system(size: 56, weight: .semibold, design: .rounded))
                    .monospacedDigit()
                    .foregroundColor(
                        Theme.measured.opacity(viewModel.status.power ? 1.0 : 0.55)
                    )
                // The target takes the swatch color, matching its line and its
                // slider — the same value in three places, one color.
                Text("Target \(setpointText)")
                    .font(.subheadline)
                    .monospacedDigit()
                    .foregroundColor(Theme.seriesColor(viewModel.chartColorIndex))
            }
            .frame(maxWidth: .infinity)
            .padding(.vertical, Theme.sRow)

            Divider().overlay(Theme.hairline)

            // Secondary stats — one row, identical treatment, equal columns.
            HStack(spacing: 0) {
                // Output is a set/driven value, so it wears the swatch color like
                // the manual output slider that drives it.
                stat("Output", String(format: "%.0f%%", viewModel.outputPercentage),
                     Theme.seriesColor(viewModel.chartColorIndex))
                stat("Battery",
                     viewModel.status.batPct.map { "\($0)%" } ?? "--",
                     (viewModel.status.batPct ?? 100) < 20 ? Theme.danger : Theme.text)
                stat("Mode", modeText, Theme.text)
                // "On" wears the swatch, matching the toggle track that caused it;
                // "Off" stays quiet gray.
                stat("Power",
                     viewModel.status.power ? "On" : "Off",
                     viewModel.status.power ? Theme.seriesColor(viewModel.chartColorIndex) : Theme.textDim)
            }
        }
        .card()
    }

    // One stat cell — every column identical, so nothing can drift.
    private func stat(_ label: String, _ value: String, _ color: Color) -> some View {
        VStack(spacing: Theme.sTight) {
            Text(label).statLabel()
            Text(value).statValue(color)
        }
        .frame(maxWidth: .infinity)
    }

    private var setpointText: String {
        if viewModel.tempUnit == "C" {
            return String(format: "%.0f°C", viewModel.status.setpoint)
        } else {
            return String(format: "%.0f°F", viewModel.status.setpoint * 9.0/5.0 + 32.0)
        }
    }
    
    private var temperatureText: String {
        if let t = viewModel.status.temp {
            if viewModel.tempUnit == "C" {
                return String(format: "%.1f°C", t)
            } else {
                let f = t * 9.0/5.0 + 32.0
                return String(format: "%.1f°F", f)
            }
        } else {
            return "--"
        }
    }
    
    // U1/U2 stay HERE (display only): the app can't select them, but the
    // device dial can, and the firmware reports them over mode-read — the
    // hero card must name whatever mode the device is actually in.
    private var modeText: String {
        switch viewModel.pidMode {
        case 0: return "Auto"
        case 1: return "Manual"
        case 2: return "U1"
        case 3: return "U2"
        default: return "Unknown"
        }
    }
}


// MARK: - AutoModeView

// Small reusable button used for mode navigation
fileprivate struct ModeButton: View {
    let title: String
    let index: Int
    @Binding var selected: Int

    var body: some View {
        // One segment of the mode switcher. The bar's dark track is drawn by the
        // parent; the selected segment lifts to the card color — the standard iOS
        // segmented-control pattern, so state is legible at a glance.
        Button(action: { selected = index }) {
            Text(title)
                .font(.footnote.weight(selected == index ? .semibold : .regular))
                .foregroundColor(selected == index ? Theme.text : Theme.textDim)
                .padding(.vertical, 7)
                .frame(maxWidth: .infinity)
                .background(
                    RoundedRectangle(cornerRadius: Theme.cornerSm)
                        .fill(selected == index ? Theme.card : Color.clear)
                )
                // Transparent regions are NOT hit-testable: without this, an
                // unselected segment's tap target was only the word itself
                // (its Color.clear background swallows nothing), so taps landing
                // off the glyphs did nothing and switching modes felt like it
                // needed two presses. Make the whole segment tappable.
                .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
    }
}


// Chart + Target slider for the Auto tab. PID tuning used to live here too, but
// it moved to the Settings sheet — the main screen keeps only the primary control.
fileprivate struct AutoModeView: View {
    @ObservedObject var viewModel: AtomizerViewModel

    var body: some View {
        VStack(alignment: .leading, spacing: Theme.sBlock) {
            TemperatureChartView(viewModel: viewModel)

            // Setpoint
            VStack(alignment: .leading, spacing: Theme.sRow) {
                HStack {
                    // "Target", matching the hero card and the chart — one word
                    // for this value everywhere the user sees it. ("Setpoint"
                    // survives in code/protocol names only.)
                    Text("Target").sectionHeader()
                    Spacer()
                    Text(String(format: "%.1f°\(viewModel.tempUnit)", displayValue(viewModel.setpoint)))
                        .statValue()
                }
                Slider(value: Binding(
                    get: { displayValue(viewModel.setpoint) },
                    set: { newVal in
                        viewModel.setSetpoint(toCelsius(newVal))
                    }
                ), in: displayRange(), step: displayStep())
                // Follows the chart swatch: this slider IS the target line.
                .tint(Theme.seriesColor(viewModel.chartColorIndex))
                // Every move writes to the peripheral. With no connection that
                // just raises "cannot write" while the thumb slides anyway, so
                // the UI claims a setpoint the device never received.
                .disabled(!viewModel.isConnected)
            }
            .opacity(viewModel.isConnected ? 1 : 0.5)
        }
        .frame(maxWidth: .infinity)
    }

    // Unit conversion helpers (°C stored internally; display in the selected unit).
    private func displayValue(_ celsius: Double) -> Double { viewModel.tempUnit == "C" ? celsius : (celsius * 9.0/5.0 + 32.0) }
    private func toCelsius(_ display: Double) -> Double { viewModel.tempUnit == "C" ? display : ((display - 32.0) * 5.0/9.0) }
    // 165–315 °C: symmetric around the 240 °C default so it sits at the slider's
    // physical center. Ceiling stays under the firmware's 320 °C thermal-runaway
    // fault (THERMAL_RUNAWAY_TEMP_C, config.h) — a setpoint above that always trips
    // the safety cutoff. If firmware raises that limit to ≥345, this can widen to 140–340.
    private func displayRange() -> ClosedRange<Double> { viewModel.tempUnit == "C" ? 165...315 : (329...599) }
    private func displayStep() -> Double { viewModel.tempUnit == "C" ? 0.5 : 1.0 }
}

// PID tuning editor — the Kp/Ki/Kd fields with Save/Reset. Extracted so it can
// live in the Settings sheet. Tunes the one loop the app owns (Auto).
fileprivate struct PIDTuningView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @Binding var kpString: String
    @Binding var kiString: String
    @Binding var kdString: String

    var body: some View {
        VStack(alignment: .leading, spacing: Theme.sRow) {
            Text("PID Tuning").sectionHeader()
            NumberField(label: "Proportional (Kp)", text: $kpString)
                .onChange(of: kpString) { _, val in
                    if let d = Double(val) { viewModel.kp = d }
                }
            NumberField(label: "Integral (Ki)", text: $kiString)
                .onChange(of: kiString) { _, val in
                    if let d = Double(val) { viewModel.ki = d }
                }
            NumberField(label: "Derivative (Kd)", text: $kdString)
                .onChange(of: kdString) { _, val in
                    if let d = Double(val) { viewModel.kd = d }
                }

            HStack(spacing: Theme.sRow) {
                Button("Save") {
                    viewModel.savePID()
                }
                .buttonStyle(PrimaryButtonStyle())

                Button("Reset") {
                    viewModel.resetPID()
                }
                .buttonStyle(GhostButtonStyle())

                Spacer()
            }
            .padding(.top, Theme.sTight)
            .disabled(!viewModel.isConnected)   // Save/Reset both write to the device
            .opacity(viewModel.isConnected ? 1 : 0.5)
        }
        .frame(maxWidth: .infinity)
    }
}


// MARK: - ManualModeView

fileprivate struct ManualModeView: View {
    @ObservedObject var viewModel: AtomizerViewModel

    var body: some View {
        VStack(alignment: .leading, spacing: Theme.sBlock) {
            TemperatureChartView(viewModel: viewModel)

            VStack(alignment: .leading, spacing: Theme.sRow) {
                HStack {
                    Text("Output").sectionHeader()
                    Spacer()
                    // Matches the Output stat in the hero card and the slider
                    // right below it — all three are the same set value.
                    Text(String(format: "%.0f%%", (viewModel.manualOutput / Double(max(1, viewModel.status.pwmMax))) * 100.0))
                        .statValue(Theme.seriesColor(viewModel.chartColorIndex))
                }
                Slider(
                    value: Binding(
                        get: { viewModel.manualOutput },
                        set: { newVal in
                            viewModel.setManualOutput(newVal)
                        }
                    ),
                    in: 0...Double(viewModel.status.pwmMax),
                    step: 1
                )
                .tint(Theme.seriesColor(viewModel.chartColorIndex))
                // Enabled ONLY in Manual mode (1): in Auto the PID owns the
                // output. This was `== 1` — inverted — which disabled the slider
                // exactly when Manual mode made it usable.
                .disabled(viewModel.pidMode != 1 || !viewModel.isConnected)

                Text("Sends a fixed output level to the nail. Use with caution.")
                    .font(.caption)
                    .foregroundColor(Theme.textDim)
            }
        }
        .frame(maxWidth: .infinity)
    }
}

// MARK: - ConfigView

fileprivate struct ConfigView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @State private var defaultSetpointString = ""
    var body: some View {
        // Flat sections inside the parent card — no card-in-card chrome.
        // PID tuning is NOT here: each mode tab already owns its own editor.
        VStack(alignment: .leading, spacing: Theme.sBlock) {

            // Device
            VStack(alignment: .leading, spacing: Theme.sRow) {
                Text("Device").sectionHeader()
                infoRow("Name", viewModel.deviceName)
                // The BLE contract version the device reported (ble.h
                // BLE_PROTOCOL_VERSION). The firmware exposes no separate
                // firmware-version characteristic, so this is the honest
                // version signal available.
                infoRow("Protocol", viewModel.deviceProtocolVersion.map { "v\($0)" } ?? "--")
                // "Device preset": the standalone power-on target, saved to the
                // device and used with no phone present. (Auto's target only
                // matters while the app is open.) Edited in the selected display
                // unit; always stored and sent in °C.
                NumberField(label: "Device preset (°\(viewModel.tempUnit))",
                            text: $defaultSetpointString, width: 76)
                    .onChange(of: defaultSetpointString) { _, val in
                        // Same ceiling as the slider: firmware faults above 320 °C.
                        if let d = Double(val) {
                            let c = viewModel.tempUnit == "C" ? d : (d - 32.0) * 5.0 / 9.0
                            viewModel.defaultSetpoint = min(max(c, 165), 315)
                        }
                    }
                    // Re-render the field's number in the newly picked unit.
                    .onChange(of: viewModel.tempUnit) { _, _ in
                        seedPresetString()
                    }
                Button("Save to Device") {
                    viewModel.setDefaultSetpoint(viewModel.defaultSetpoint)
                }
                .buttonStyle(PrimaryButtonStyle())
            }

            Divider().overlay(Theme.hairline)

            // Display
            VStack(alignment: .leading, spacing: 0) {
                Text("Display").sectionHeader()
                    .padding(.bottom, Theme.sRow)
                // One switch, for the one thing worth choosing. The toggles that
                // used to live here were removed: "Notifications" flipped a bool
                // nothing read (no notification code exists), "Dual-axis output"
                // had no readers at all, "Smoothing" only affected the iOS 15
                // Canvas path this app can't reach, and grid/time-labels were
                // preferences over details the chart needs to be readable.
                ToggleRow(label: "Show chart", isOn: $viewModel.showTemperatureChart)
                    .onChange(of: viewModel.showTemperatureChart) { viewModel.saveConfigState() }
                // Chart time span is fixed at AtomizerViewModel.chartWindowSeconds.
                // The slider that used to live here let the window disagree with
                // the axis ticks, which are pinned at 0/-5/-10/-15.
            }

            Divider().overlay(Theme.hairline)

            // Units — display preference for every temperature the app shows
            // (Target readout/slider). Stored as "C"/"F" and persisted; the device
            // may also report its own unit, which wins on the next notification.
            VStack(alignment: .leading, spacing: Theme.sRow) {
                Text("Units").sectionHeader()
                Picker("Temperature", selection: Binding(
                    get: { viewModel.tempUnit },
                    set: { viewModel.setTempUnit($0) }
                )) {
                    Text("°C").tag("C")
                    Text("°F").tag("F")
                }
                .pickerStyle(.segmented)
            }

            // No Bluetooth section here — the Connection card in Settings owns
            // scanning, connecting and forgetting. One home per feature.
        }
        .frame(maxWidth: .infinity)
        .onAppear {
            seedPresetString()
        }
    }

    /// Seed the preset field from the model, converted to the display unit.
    /// (The model value is always °C.)
    private func seedPresetString() {
        let v = viewModel.tempUnit == "C"
            ? viewModel.defaultSetpoint
            : viewModel.defaultSetpoint * 9.0 / 5.0 + 32.0
        defaultSetpointString = String(format: "%.1f", v)
    }

    private func infoRow(_ label: String, _ value: String) -> some View {
        HStack {
            Text(label)
                .font(.subheadline)
                .foregroundColor(Theme.textDim)
            Spacer()
            Text(value)
                .font(.subheadline.weight(.medium))
                .foregroundColor(Theme.text)
        }
    }

}

// Reusable toggle row style
fileprivate struct ToggleRow: View {
    let label: String
    @Binding var isOn: Bool
    var body: some View {
        HStack {
            Text(label)
                .font(.subheadline)
                .foregroundColor(Theme.text)
            Spacer()
            Toggle("", isOn: $isOn)
                .labelsHidden()
                // Same warm gray the Power switch shows under the white swatch
                // (controlFill-darkened cream): the knob is always white, and a
                // raw cream track made the switch state unreadable.
                .tint(Theme.controlFill(0))
        }
        .padding(.vertical, 6)
    }
}

// MARK: - SettingsView
// Modal sheet for everything that isn't a primary live control: PID tuning,
// device config, display options, and connection. Built from the same charcoal
// cards as the main screen so it reads as the same app, not a stock iOS Form.
fileprivate struct SettingsView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @Binding var kpString: String
    @Binding var kiString: String
    @Binding var kdString: String
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: Theme.sBlock) {
                    // Order: device details first, then how you connect to it, then
                    // the deeper PID tuning knobs last.
                    // Device + Display + Units sections.
                    ConfigView(viewModel: viewModel)
                        .card()

                    // Connection (brings its own .card() and device-picker sheet).
                    ConnectionView(viewModel: viewModel)

                    // PID tuning (the Auto loop).
                    VStack(alignment: .leading, spacing: Theme.sCard) {
                        PIDTuningView(viewModel: viewModel, kpString: $kpString,
                                      kiString: $kiString, kdString: $kdString)
                    }
                    .card()

                    Spacer(minLength: 12)
                }
                .padding(Theme.pad)
            }
            .background(Theme.bg)
            .navigationTitle("Settings")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button("Done") { dismiss() }
                }
            }
            .toolbarBackground(Theme.bg, for: .navigationBar)
            .toolbarBackground(.visible, for: .navigationBar)
        }
        .preferredColorScheme(.dark)
        .tint(Theme.accent)
    }
}

fileprivate struct ConnectionView: View {
    @ObservedObject var viewModel: AtomizerViewModel

    // Forget confirmation is owned here — this view is self-contained now that it
    // lives inside the Settings sheet rather than being driven from ContentView.
    @State private var showForgetConfirmation = false
    // Device discovery lives in a bottom sheet, not inline: an unbounded ForEach
    // of BLE peripherals used to grow this card down the page as devices appeared.
    @State private var showDevicePicker = false

    var body: some View {
        VStack(alignment: .leading, spacing: Theme.sCard) {
            HStack(spacing: Theme.sRow) {
                Circle()
                    .fill(viewModel.isConnected ? Theme.ok : Theme.textDim.opacity(0.4))
                    .frame(width: 8, height: 8)
                Text(viewModel.isConnected ? "Connected" : "Not connected")
                    .font(.subheadline.weight(.medium))
                    .foregroundColor(viewModel.isConnected ? Theme.ok : Theme.textDim)
                Spacer()
                if viewModel.isConnected {
                    Text(viewModel.shouldAutoReconnect ? "Auto-reconnect on" : "Auto-reconnect off")
                        .statLabel()
                }
            }

            if viewModel.isConnected {
                HStack(spacing: Theme.sRow) {
                    Button("Disconnect") {
                        viewModel.disconnect()
                    }
                    .buttonStyle(GhostButtonStyle())
                    Button("Forget") {
                        showForgetConfirmation = true
                    }
                    .buttonStyle(GhostButtonStyle(tint: Theme.danger))
                    Spacer()
                }
            } else {
                Text("Make sure Bluetooth is on and the device is powered up.")
                    .font(.caption)
                    .foregroundColor(Theme.textDim)
                Button("Connect") {
                    viewModel.startScanning()
                    showDevicePicker = true
                }
                .buttonStyle(PrimaryButtonStyle())
            }
        }
        .card()
        // The picker owns scanning for as long as it's up. Dismissing it — by
        // swipe, Done, or a successful connection — always stops the scan.
        .sheet(isPresented: $showDevicePicker, onDismiss: {
            viewModel.cancelScan()
        }) {
            DevicePickerView(viewModel: viewModel, isPresented: $showDevicePicker)
                .presentationDetents([.medium, .large])
                .presentationDragIndicator(.visible)
                .presentationBackground(Theme.bg)
        }
        .confirmationDialog("Forget Device", isPresented: $showForgetConfirmation) {
            Button("Forget", role: .destructive) {
                viewModel.forgetDevice()
            }
            Button("Cancel", role: .cancel) { }
        } message: {
            Text("This will disconnect and disable auto-reconnection. Are you sure?")
        }
    }
}

// MARK: - DevicePickerView
// Standard iOS device-selection pattern: a bounded, scrollable list in a bottom
// sheet. However many peripherals appear, the list scrolls inside the sheet —
// the main page never moves. Tapping a row connects and auto-dismisses.
fileprivate struct DevicePickerView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @Binding var isPresented: Bool

    // The row we're mid-connect on, so it can show a spinner instead of a chevron.
    @State private var connectingID: UUID? = nil

    var body: some View {
        NavigationStack {
            Group {
                if viewModel.discoveredPeripherals.isEmpty {
                    // Empty state — centered, calm, still clearly scanning.
                    VStack(spacing: Theme.sCard) {
                        ProgressView()
                            .tint(Theme.textDim)
                        Text("Looking for devices…")
                            .font(.subheadline)
                            .foregroundColor(Theme.textDim)
                        Text("Make sure the device is powered on and nearby.")
                            .font(.caption)
                            .foregroundColor(Theme.textDim.opacity(0.7))
                            .multilineTextAlignment(.center)
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .padding()
                } else {
                    List {
                        ForEach(viewModel.discoveredPeripherals, id: \.peripheral.identifier) { item in
                            Button {
                                connectingID = item.peripheral.identifier
                                viewModel.connect(to: item.peripheral)
                            } label: {
                                DeviceRow(
                                    name: item.peripheral.name ?? "Unknown device",
                                    rssi: item.rssi.intValue,
                                    connecting: connectingID == item.peripheral.identifier
                                )
                            }
                            .listRowBackground(Theme.card)
                        }
                    }
                    .scrollContentBackground(.hidden)
                }
            }
            .background(Theme.bg)
            .navigationTitle("Select Device")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .topBarLeading) {
                    Button("Done") { isPresented = false }
                }
                ToolbarItem(placement: .topBarTrailing) {
                    // A quiet "still scanning" cue that lives in the chrome rather
                    // than pushing the list around.
                    ProgressView().tint(Theme.textDim)
                }
            }
        }
        // Connecting succeeded elsewhere in the model — close the sheet (its
        // onDismiss stops the scan). Failure leaves the sheet up to retry.
        .onChange(of: viewModel.isConnected) { _, connected in
            if connected { isPresented = false }
        }
    }

}

// One row of the device list. Plain values, no CoreBluetooth types — so it
// renders identically for a real peripheral or a preview/mock.
fileprivate struct DeviceRow: View {
    let name: String
    let rssi: Int
    let connecting: Bool

    var body: some View {
        HStack(spacing: Theme.sRow) {
            VStack(alignment: .leading, spacing: 2) {
                Text(name)
                    .font(.body)
                    .foregroundColor(Theme.text)
                Text(signalDescription(rssi))
                    .font(.caption)
                    .foregroundColor(Theme.textDim)
            }
            Spacer()
            if connecting {
                ProgressView().tint(Theme.textDim)
            } else {
                SignalBars(rssi: rssi)
            }
        }
        .contentShape(Rectangle())
    }

    private func signalDescription(_ rssi: Int) -> String {
        let quality: String
        switch rssi {
        case ..<(-90): quality = "Weak signal"
        case ..<(-70): quality = "Fair signal"
        default:       quality = "Strong signal"
        }
        return "\(quality) · \(rssi) dBm"
    }
}

// A compact 3-bar signal indicator derived from RSSI — the standard visual for
// wireless strength, quieter than a raw dBm number on every row.
fileprivate struct SignalBars: View {
    let rssi: Int

    private var level: Int {
        switch rssi {
        case ..<(-90): return 1
        case ..<(-70): return 2
        default:       return 3
        }
    }

    var body: some View {
        HStack(alignment: .bottom, spacing: 2) {
            ForEach(1...3, id: \.self) { i in
                RoundedRectangle(cornerRadius: 1)
                    .fill(i <= level ? Theme.ok : Theme.textDim.opacity(0.25))
                    .frame(width: 3, height: CGFloat(4 + i * 3))
            }
        }
        .accessibilityLabel("Signal strength \(level) of 3")
    }
}
