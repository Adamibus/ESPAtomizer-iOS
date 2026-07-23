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
// The current look is the FACEPLATE: one continuous dark surface, like the
// front panel of a bench power supply — zones separated by hairline rules and
// etched monospaced labels (RuleLabel), depth reserved for recessed wells
// (`.well()`: the temperature readout, the chart bed). There are deliberately
// NO cards: the rounded-panel-on-dark-background pattern is what made the app
// read as template/AI-generated. If restyling, the three highest-impact spots:
//   1. BRAND NAME + TITLE ....... AtomizerViewModel.swift  →  `appTitle` / `appName`
//   2. ACCENT COLOR ............. `Theme.accent` below.
//   3. PANEL STRUCTURE .......... FaceplateRule / RuleLabel / `.well()` — the
//                                 three primitives every zone is built from.
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
    // FACEPLATE look: the screen is ONE continuous surface, like the front
    // panel of a bench power supply — no floating cards. Structure comes from
    // hairline rules + etched uppercase labels; depth comes from RECESSED
    // wells (readout window, chart bed, entry fields) and the occasional
    // RAISED key (selected mode segment, list rows).
    static let accent   = gold           // small highlights + primary actions ONLY, never large text
    static let bg       = charcoal       // the faceplate surface
    static let card     = charcoalLift   // raised key: selected segment, device-list rows
    static let inset    = charcoalDeep   // recessed wells: readout, chart bed, fields, tracks
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
    // Machined, not friendly: corners eased just enough to not glint. Soft
    // radii are the single biggest "template app" tell — keep these tight.
    static let corner: CGFloat = 3
    static let cornerSm: CGFloat = 2      // fields, segments, small chips

    // MARK: Spacing scale — use these instead of ad-hoc numbers
    static let sBlock: CGFloat = 20   // between major sections
    static let sCard:  CGFloat = 14   // inside a card
    static let sRow:   CGFloat = 8    // between related rows
    static let sTight: CGFloat = 4    // label above value
    static let pad:    CGFloat = 16   // card inner padding
    static let fieldW: CGFloat = 96   // every numeric entry field, same width
}

// MARK: - Type scale. Use these, never ad-hoc fonts, so the app reads as one voice.
// Labels and values are MONOSPACED throughout — silk-screened lettering on test
// gear, terminal readouts. This is most of the personality; don't dilute it
// with rounded or default-sans numbers.
extension Text {
    /// SECTION HEADER — etched uppercase label, like the silk-screen on a scope.
    func sectionHeader() -> some View {
        self.font(.system(.caption, design: .monospaced)).fontWeight(.semibold)
            .textCase(.uppercase).kerning(1.1)
            .foregroundColor(Theme.textDim)
    }
    /// STAT LABEL — small etched label above a value.
    func statLabel() -> some View {
        self.font(.system(.caption2, design: .monospaced)).fontWeight(.medium)
            .textCase(.uppercase).kerning(0.8)
            .foregroundColor(Theme.textDim)
    }
    /// STAT VALUE — live number. Monospaced face (not just digits) so it reads
    /// as an instrument readout and never jitters.
    func statValue(_ color: Color = Theme.text) -> some View {
        self.font(.system(.title3, design: .monospaced).weight(.semibold))
            .foregroundColor(color)
    }
}

// MARK: - Button styles. Two, used everywhere: one filled primary, one quiet ghost.
// Sharp-cornered, monospaced, uppercase — panel keys, not iOS pills.
struct PrimaryButtonStyle: ButtonStyle {
    var tint: Color = Theme.accent
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(.footnote, design: .monospaced).weight(.semibold))
            .textCase(.uppercase)
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
            .font(.system(.footnote, design: .monospaced).weight(.medium))
            .textCase(.uppercase)
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
                .font(.system(.subheadline, design: .monospaced))
                .foregroundColor(Theme.textDim)
            Spacer()
            TextField("", text: $text)
                .keyboardType(.decimalPad)
                .multilineTextAlignment(.trailing)
                .font(.system(.body, design: .monospaced))
                .foregroundColor(Theme.text)
                .padding(.vertical, 7).padding(.horizontal, 10)
                .frame(width: width)
                .background(Theme.inset)
                .cornerRadius(Theme.cornerSm)
        }
    }
}

// FACEPLATE primitives. There are no cards: content sits flat on the one
// continuous surface, and these three things do all the structural work.

/// A full-width engraved hairline — the rule between zones of the panel.
struct FaceplateRule: View {
    var body: some View {
        Rectangle().fill(Theme.hairline).frame(height: 1)
    }
}

/// Etched zone label with the rule running off to the right edge:
///   TARGET ─────────────────────
/// This replaces what used to be a card boundary.
struct RuleLabel: View {
    let label: String
    init(_ label: String) { self.label = label }
    var body: some View {
        HStack(spacing: Theme.sRow) {
            Text(label).sectionHeader()
                .fixedSize()
            FaceplateRule()
        }
    }
}

/// A RECESSED well — the readout window and the chart bed. The one place the
/// faceplate is allowed depth: a dark inset pane behind glass.
extension View {
    func well(pad: CGFloat = Theme.pad) -> some View {
        self
            .padding(pad)
            .frame(maxWidth: .infinity)
            .background(Theme.inset)
            .cornerRadius(Theme.corner)
            .overlay(
                RoundedRectangle(cornerRadius: Theme.corner)
                    .stroke(Theme.hairline, lineWidth: 1)
            )
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

                    // Rule between the readout cluster and the control zone —
                    // this line is what the card boundary used to be.
                    FaceplateRule()

                    // Controls and Tabs — flat on the faceplate, no panel chrome.
                    VStack(alignment: .leading, spacing: Theme.sCard) {
                        // Power — the most important control, first and unmissable.
                        Toggle(isOn: $viewModel.powerToggle) {
                            // Etched like every other faceplate label — the stock
                            // sans here read as a different app than the rest.
                            Text("Power")
                                .font(.system(.callout, design: .monospaced).weight(.semibold))
                                .textCase(.uppercase)
                                .kerning(1.2)
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
                            .cornerRadius(Theme.corner)

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

                    // Connection now lives in the Settings sheet (gear, top-right).

                    Spacer(minLength: 0)

                    // tmux-style status strip pinned to the bottom of the panel.
                    StatusLine(viewModel: viewModel)
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
                    // Near-square sheet corners: the stock ~40pt radius read as
                    // soft consumer UI against the machined faceplate.
                    .presentationCornerRadius(8)
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
                Label("TC FAULT — THERMOCOUPLE DISCONNECTED", systemImage: "exclamationmark.triangle.fill")
                    .font(.system(.caption2, design: .monospaced).weight(.semibold))
                    .foregroundColor(Theme.danger)
                    .padding(.vertical, 6)
                    .padding(.horizontal, Theme.sCard)
                    .frame(maxWidth: .infinity)
                    .background(Theme.danger.opacity(0.15))
                    .cornerRadius(Theme.cornerSm)
            }

            // Hero: the READOUT WINDOW — a recessed dark pane, like the LCD on a
            // bench supply, with the live temperature glowing behind the glass.
            // The tallest thing on the page on purpose: it's the one number the
            // whole device exists to show.
            VStack(spacing: Theme.sRow) {
                // Measured temperature, so it matches the gray temperature line in
                // the chart. Dims further when power is off; a faint phosphor
                // glow while powered sells the "lit display" read.
                Text(temperatureText)
                    .font(.system(size: 64, weight: .medium, design: .monospaced))
                    .minimumScaleFactor(0.8)   // "1250.4°F" would clip; digits shrink before wrapping
                    .lineLimit(1)
                    .foregroundColor(
                        Theme.measured.opacity(viewModel.status.power ? 1.0 : 0.55)
                    )
                    .shadow(color: viewModel.status.power
                            ? Theme.measured.opacity(0.35) : .clear,
                            radius: 9)
                // The target takes the swatch color, matching its line and its
                // slider — the same value in three places, one color.
                Text("TARGET \(setpointText)")
                    .font(.system(.body, design: .monospaced))
                    .kerning(1.2)
                    .foregroundColor(Theme.seriesColor(viewModel.chartColorIndex))
            }
            .padding(.vertical, Theme.pad)
            .well(pad: Theme.sRow)

            // Gauge cluster — one row of readouts flat on the faceplate, sitting
            // under the readout window like the small dials under a scope screen.
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

// MARK: - StatusLine
// tmux-style strip pinned to the bottom of the faceplate: link state on the
// left, protocol version on the right. Quiet, monospaced, always present —
// connection state stays glanceable even though its controls live in Settings.
fileprivate struct StatusLine: View {
    @ObservedObject var viewModel: AtomizerViewModel

    var body: some View {
        VStack(spacing: Theme.sRow) {
            FaceplateRule()
            HStack {
                Text(viewModel.isConnected
                     ? "● LINK · \(viewModel.deviceName.uppercased())"
                     : "○ NO LINK")
                    .foregroundColor(viewModel.isConnected ? Theme.ok : Theme.textDim)
                Spacer()
                if let v = viewModel.deviceProtocolVersion {
                    Text("PROTO v\(v)")
                        .foregroundColor(Theme.textDim)
                }
            }
            .font(.system(.caption2, design: .monospaced))
            .lineLimit(1)
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
                .font(.system(.footnote, design: .monospaced)
                    .weight(selected == index ? .semibold : .regular))
                .textCase(.uppercase)
                .kerning(0.8)
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

                // One-line hint, mirroring Manual's caution line EXACTLY in
                // structure: the two tabs must be the same height, or switching
                // between them compresses the readout well and the whole page
                // shifts. If you edit one caption, keep the other in step.
                Text("PID drives the coil to the target temperature.")
                    .font(.system(.caption, design: .monospaced))
                    .foregroundColor(Theme.textDim)
                    .lineLimit(1)
                    .minimumScaleFactor(0.8)
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
            RuleLabel("PID Tuning")
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
                    // Cream like the Target readout beside ITS slider — the two
                    // tabs' slider rows must match. (The swatch-colored Output
                    // stat lives in the hero cluster; down here it clashed with
                    // Target's neutral value one tab over.)
                    Text(String(format: "%.0f%%", (viewModel.manualOutput / Double(max(1, viewModel.status.pwmMax))) * 100.0))
                        .statValue()
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

                // One line, structurally identical to Auto's hint — see the
                // comment there: unequal tab heights shift the whole page.
                Text("Manual sends voltage directly to the coil. USE WITH CARE.")
                    .font(.system(.caption, design: .monospaced))
                    .foregroundColor(Theme.textDim)
                    .lineLimit(1)
                    .minimumScaleFactor(0.8)
            }
            // Same disconnected dimming as Auto's block: without this the two
            // tabs rendered the same controls in two different grays.
            .opacity(viewModel.isConnected ? 1 : 0.5)
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
                RuleLabel("Device")
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

            // Display — the RuleLabel carries its own rule; no divider needed.
            VStack(alignment: .leading, spacing: 0) {
                RuleLabel("Display")
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

            // Units — display preference for every temperature the app shows
            // (Target readout/slider). Stored as "C"/"F" and persisted; the device
            // may also report its own unit, which wins on the next notification.
            VStack(alignment: .leading, spacing: Theme.sRow) {
                RuleLabel("Units")
                // Same sharp two-key switcher as AUTO/MANUAL on the home page.
                // The stock segmented picker's capsule was the one rounded
                // consumer control left on this panel.
                HStack(spacing: 3) {
                    unitKey("°C", tag: "C")
                    unitKey("°F", tag: "F")
                }
                .padding(3)
                .background(Theme.inset)
                .cornerRadius(Theme.corner)
            }

            // No Bluetooth section here — the Connection card in Settings owns
            // scanning, connecting and forgetting. One home per feature.
        }
        .frame(maxWidth: .infinity)
        .onAppear {
            seedPresetString()
        }
    }

    /// One key of the unit switcher — the ModeButton pattern, tagged by the
    /// stored "C"/"F" string instead of a device mode index.
    private func unitKey(_ title: String, tag: String) -> some View {
        let selected = viewModel.tempUnit == tag
        return Button { viewModel.setTempUnit(tag) } label: {
            Text(title)
                .font(.system(.footnote, design: .monospaced)
                    .weight(selected ? .semibold : .regular))
                .foregroundColor(selected ? Theme.text : Theme.textDim)
                .padding(.vertical, 7)
                .frame(maxWidth: .infinity)
                .background(
                    RoundedRectangle(cornerRadius: Theme.cornerSm)
                        .fill(selected ? Theme.card : Color.clear)
                )
                .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
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
                .font(.system(.subheadline, design: .monospaced))
                .foregroundColor(Theme.textDim)
            Spacer()
            Text(value)
                .font(.system(.subheadline, design: .monospaced).weight(.medium))
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
                .font(.system(.subheadline, design: .monospaced))
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

// MARK: - SheetHeader
// Sheet chrome drawn on the faceplate itself — no UIKit nav bar, whose iOS 26
// glass treatment (circular buttons, sans titles) kept leaking stock consumer
// UI into the panel. Title left, rule through the middle, DONE key right.
fileprivate struct SheetHeader: View {
    let title: String
    var busy: Bool = false          // quiet scanning cue, lives in the chrome
    let onDone: () -> Void

    var body: some View {
        HStack(spacing: Theme.sRow) {
            Text(title)
                .font(.system(.subheadline, design: .monospaced).weight(.semibold))
                .kerning(1.5)
                .foregroundColor(Theme.text)
                .fixedSize()
            Spacer()
            if busy {
                ProgressView().tint(Theme.textDim)
            }
            Button(action: onDone) {
                Text("DONE")
                    .font(.system(.footnote, design: .monospaced).weight(.semibold))
                    .kerning(1.0)
                    .foregroundColor(Theme.accent)
                    .contentShape(Rectangle())
            }
            .buttonStyle(.plain)
        }
        .padding(.top, Theme.sRow)   // clears the sheet's drag indicator
    }
}

// MARK: - SettingsView
// Modal sheet for everything that isn't a primary live control: PID tuning,
// device config, display options, and connection. Same continuous faceplate as
// the main screen — flat zones under rule-labels — so it reads as the same
// piece of equipment, not a stock iOS Form.
fileprivate struct SettingsView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @Binding var kpString: String
    @Binding var kiString: String
    @Binding var kdString: String
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        // FIXED panel, like the home page — everything fits the sheet by
        // design, so nothing scrolls. When adding a section, trim spacing
        // rather than re-introducing a ScrollView.
        VStack(spacing: Theme.sBlock) {
            // Faceplate chrome, not a nav bar.
            SheetHeader(title: "SETTINGS") { dismiss() }

            // Order: device details first, then how you connect to it, then
            // the deeper PID tuning knobs last. Each zone announces itself
            // with its own RuleLabel — no panel chrome between them.
            // Device + Display + Units sections.
            ConfigView(viewModel: viewModel)

            // Connection (brings its own RuleLabel and device-picker sheet).
            ConnectionView(viewModel: viewModel)

            // PID tuning (the Auto loop).
            PIDTuningView(viewModel: viewModel, kpString: $kpString,
                          kiString: $kiString, kdString: $kdString)

            Spacer(minLength: 0)
        }
        .padding(Theme.pad)
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
        .background(Theme.bg)
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
            RuleLabel("Connection")
            HStack(spacing: Theme.sRow) {
                Circle()
                    .fill(viewModel.isConnected ? Theme.ok : Theme.textDim.opacity(0.4))
                    .frame(width: 8, height: 8)
                Text(viewModel.isConnected ? "Connected" : "Not connected")
                    .font(.system(.subheadline, design: .monospaced).weight(.medium))
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
                // Terse enough for one line — the full sentence wrapped, and a
                // faceplate prints hints, it doesn't paragraph them.
                Text("Turn on Bluetooth with the device powered and in range.")
                    .font(.system(.caption, design: .monospaced))
                    .foregroundColor(Theme.textDim)
                    .lineLimit(1)
                    .minimumScaleFactor(0.8)
                Button("Connect") {
                    viewModel.startScanning()
                    showDevicePicker = true
                }
                .buttonStyle(PrimaryButtonStyle())
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        // The picker owns scanning for as long as it's up. Dismissing it — by
        // swipe, Done, or a successful connection — always stops the scan.
        .sheet(isPresented: $showDevicePicker, onDismiss: {
            viewModel.cancelScan()
        }) {
            DevicePickerView(viewModel: viewModel, isPresented: $showDevicePicker)
                .presentationDetents([.medium, .large])
                .presentationDragIndicator(.visible)
                .presentationBackground(Theme.bg)
                .presentationCornerRadius(8)
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
        VStack(spacing: 0) {
            // Faceplate chrome: title, rule, live scan spinner, DONE key.
            SheetHeader(title: "SELECT DEVICE", busy: true) { isPresented = false }
                .padding(.horizontal, Theme.pad)
                .padding(.top, Theme.sRow)

            Group {
                if viewModel.discoveredPeripherals.isEmpty {
                    // Empty state — centered, calm, still clearly scanning.
                    VStack(spacing: Theme.sCard) {
                        ProgressView()
                            .tint(Theme.textDim)
                        Text("SCANNING…")
                            .font(.system(.subheadline, design: .monospaced))
                            .kerning(1.0)
                            .foregroundColor(Theme.textDim)
                        Text("Make sure the device is powered on and nearby.")
                            .font(.system(.caption, design: .monospaced))
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
        }
        .background(Theme.bg)
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
                    .font(.system(.body, design: .monospaced))
                    .foregroundColor(Theme.text)
                Text(signalDescription(rssi))
                    .font(.system(.caption, design: .monospaced))
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
