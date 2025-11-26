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
// The app uses a SINGLE global ScrollView in ContentView. All tab views (AutoModeView, ManualModeView, PresetView, ConfigView)
// use VStack layouts and DO NOT contain their own ScrollView. This ensures the entire page scrolls as one, with no nested scroll areas.
// This matches iOS/macOS best practices and prevents double scrollbars, clipped content, or scroll conflicts.
//
// When adding new tabs or expanding tab content, always place content inside a VStack and rely on the global ScrollView in ContentView.
// Do NOT add additional ScrollViews inside tab views unless you have a very specific reason (e.g., a large List inside a card).
// ---

import SwiftUI
import CoreBluetooth

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
    
    // Additional script text for U1/U2
    
    
    // UI state
    @State private var selectedMode = 0
    @State private var showForgetConfirmation = false
    @State private var showingError = false
    @State private var errorText: String = ""
    
    // Re-declare any CBUUIDs we need to write from the UI (ViewModel keeps them private).
    // These must match the UUIDs used in your AtomizerViewModel.
    private let uuidMode = CBUUID(string: "3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidSaveScript = CBUUID(string: "3f1a00ff-0000-0000-0000-000000000001") // example custom - adjust if different
    // NOTE: If your firmware expects script uploading under a specific characteristic,
    // replace `uuidSaveScript` with the correct UUID. If you don't have one, script save will be local only.
    
    // Prefer flexible layouts; avoid hard-coded large fixed heights so smaller devices can scroll.
    // The inner mode views are already ScrollViews so the main column should be scrollable.
    
    var body: some View {
        NavigationView {
            ScrollView { // SINGLE global scroll view
                VStack(spacing: 18) {
                    Text(viewModel.appTitle)
                        .font(.largeTitle)
                        .foregroundColor(.blue)
                    StatusPanelView(viewModel: viewModel)
                    
                    // Controls and Tabs
                    VStack(alignment: .leading, spacing: 12) {
                        HStack {
                            Text("Controls")
                                .font(.headline)
                    // show setpoint with unit conversion if needed
                    if viewModel.tempUnit == "C" {
                        Text(String(format: "%.1f°C", viewModel.status.setpoint))
                            .font(.title2)
                    } else {
                        let f = viewModel.status.setpoint * 9.0/5.0 + 32.0
                        Text(String(format: "%.1f°F", f))
                            .font(.title2)
                    }
                            if viewModel.pidMode == 1 {
                                Text(String(format: "%.0f%%", viewModel.manualOutputPercentage))
                                    .font(.title2)
                            } else {
                                if viewModel.tempUnit == "C" {
                                    Text(String(format: "%.1f°C", viewModel.status.setpoint))
                                        .font(.title2)
                                } else {
                                    let f = viewModel.status.setpoint * 9.0/5.0 + 32.0
                                    Text(String(format: "%.1f°F", f))
                                        .font(.title2)
                                }
                            }
                            Text("Mode: \(modeName(for: viewModel.pidMode))")
                                .font(.subheadline)
                                .foregroundColor(.secondary)
                        }
                        
                        // Power Toggle
                        Toggle("Power", isOn: $viewModel.powerToggle)
                            .onChange(of: viewModel.powerToggle) { _ in
                                viewModel.togglePower()
                            }
                            .padding(.vertical, 6)
                        
                        // Mode navigation (button-bar) + content switcher
                        VStack(spacing: 8) {
                            HStack(spacing: 8) {
                                ModeButton(title: "Auto", index: 0, selected: $selectedMode)
                                ModeButton(title: "Manual", index: 1, selected: $selectedMode)
                                ModeButton(title: "U1", index: 2, selected: $selectedMode)
                                ModeButton(title: "U2", index: 3, selected: $selectedMode)
                                ModeButton(title: "Config", index: 4, selected: $selectedMode)
                            }

                            // Content area: switch on selectedMode
                            Group {
                                switch selectedMode {
                                case 0:
                                    AutoModeView(viewModel: viewModel, kpString: $kpString, kiString: $kiString, kdString: $kdString)
                                case 1:
                                    ManualModeView(viewModel: viewModel)
                                case 2:
                                    PresetView(title: "U1 Preset", viewModel: viewModel, kpString: $kpString, kiString: $kiString, kdString: $kdString, modeIndex: 2)
                                case 3:
                                    PresetView(title: "U2 Preset", viewModel: viewModel, kpString: $kpString, kiString: $kiString, kdString: $kdString, modeIndex: 3)
                                case 4:
                                    ConfigView(viewModel: viewModel)
                                default:
                                    Text("Unknown Mode")
                                }
                            }
                            .animation(.default, value: selectedMode)
                        }
                        
                        Text("Setpoint Mode: \(viewModel.status.spmode ? "POT" : "FIXED")")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                    .padding()
                .background(Color(uiColor: .systemGray6))
                .cornerRadius(12)
                    
                    // Connection controls
                    ConnectionView(
                        viewModel: viewModel,
                        showForgetConfirmation: $showForgetConfirmation
                    )
                    
                    Spacer(minLength: 40)
                }
                .padding()
            } // END single ScrollView
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .principal) {
                    HStack(spacing: 8) {
                        Image("app_icon")
                            .resizable()
                            .frame(width: 30, height: 30)
                            .clipShape(RoundedRectangle(cornerRadius: 6))
                            .shadow(radius: 1)
                        
                        Text(viewModel.appName)
                            .font(.headline)
                    }
                }
            }
            .onAppear {
                // initialize fields
                kpString = String(format: "%.3f", viewModel.kp)
                kiString = String(format: "%.3f", viewModel.ki)
                kdString = String(format: "%.3f", viewModel.kd)
                selectedMode = viewModel.pidMode
                // load per-mode setpoint and PID into UI
                let sp = viewModel.getSetpointForMode(selectedMode)
                viewModel.setpoint = sp
                if selectedMode >= 0 && selectedMode < viewModel.pidTunings.count {
                    let pid = viewModel.pidTunings[selectedMode]
                    viewModel.kp = pid.kp
                    viewModel.ki = pid.ki
                    viewModel.kd = pid.kd
                    kpString = String(format: "%.3f", pid.kp)
                    kiString = String(format: "%.3f", pid.ki)
                    kdString = String(format: "%.3f", pid.kd)
                }
            }
            .onChange(of: viewModel.kp) { newVal in
                kpString = String(format: "%.3f", newVal)
            }
            .onChange(of: viewModel.ki) { newVal in
                kiString = String(format: "%.3f", newVal)
            }
            .onChange(of: viewModel.kd) { newVal in
                kdString = String(format: "%.3f", newVal)
            }
            .onChange(of: selectedMode) { newVal in
                // When user switches modes via the button bar, tell the device (except config which is read-only)
                if newVal != 4 {
                    viewModel.setMode(mode: newVal)
                }
                // Load the per-mode setpoint and PID values into the UI
                let sp = viewModel.getSetpointForMode(newVal)
                viewModel.setpoint = sp
                if newVal >= 0 && newVal < viewModel.pidTunings.count {
                    let pid = viewModel.pidTunings[newVal]
                    viewModel.kp = pid.kp
                    viewModel.ki = pid.ki
                    viewModel.kd = pid.kd
                    kpString = String(format: "%.3f", pid.kp)
                    kiString = String(format: "%.3f", pid.ki)
                    kdString = String(format: "%.3f", pid.kd)
                }
            }
            .onChange(of: viewModel.pidMode) { newVal in
                selectedMode = newVal
            }
            .onChange(of: viewModel.lastErrorMessage) { newVal in
                if let m = newVal {
                    errorText = m
                    showingError = true
                }
            }
            .confirmationDialog("Forget Device", isPresented: $showForgetConfirmation) {
                Button("Forget", role: .destructive) {
                    viewModel.forgetDevice()
                }
                Button("Cancel", role: .cancel) { }
            } message: {
                Text("This will disconnect and disable auto-reconnection. Are you sure?")
            }
            .alert("Error", isPresented: $showingError, actions: {
                Button("OK", role: .cancel) { showingError = false }
            }, message: { Text(errorText) })
        } // NavigationView
    } // body

    // Local computed temperature text for the large header
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
    
    // Helper: pretty mode name
    private func modeName(for mode: Int) -> String {
        switch mode {
        case 0: return "Auto"
        case 1: return "Manual"
        case 2: return "U1"
        case 3: return "U2"
        case 4: return "Config"
        default: return "Unknown"
        }
    }
    
    // Attempt to save a script to a profile on-device. If your firmware does not support scripts,
    // this simply prints to console and could be extended to call a proper characteristic.
    private func saveScriptToProfile(profileName: String, script: String) {
        // Construct a simple "SAVE:U1:<script>" payload or similar. Adapt to firmware.
        let payload = "SAVE:\(profileName):\(script)"
        // Use the public writeCharacteristic on the view model.
        // We need a characteristic UUID to write to — use an assumed one here (uuidSaveScript).
        if let data = payload.data(using: .utf8) {
            // We'll call the view model's writeCharacteristic(public) method.
            // Since writeCharacteristic expects a CBUUID, use uuidSaveScript declared above.
            viewModel.writeCharacteristic(uuidSaveScript, value: payload)
            print("Wrote script to profile \(profileName) with payload: \(payload)")
        } else {
            print("Failed to encode script payload.")
        }
    }
}

// MARK: - StatusPanelView

fileprivate struct StatusPanelView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Status")
                .font(.headline)
            
            HStack {
                VStack {
                    Text("Temperature")
                        .font(.caption)
                    Text(temperatureText)
                        .font(.largeTitle)
                        .foregroundColor(.blue)
                }
                
                Spacer()
                
                VStack {
                    Text("Setpoint")
                        .font(.caption)
                    // Use selected unit for setpoint display
                    if viewModel.tempUnit == "C" {
                        Text(String(format: "%.1f°C", viewModel.status.setpoint))
                            .font(.title2)
                    } else {
                        let f = viewModel.status.setpoint * 9.0/5.0 + 32.0
                        Text(String(format: "%.1f°F", f))
                            .font(.title2)
                    }
                }
                
                Spacer()
                
                VStack {
                    Text("Output")
                        .font(.caption)
                    Text(String(format: "%.0f%%", viewModel.outputPercentage))
                        .font(.title2)
                        .foregroundColor(.green)
                }
            }
            
            HStack {
                VStack {
                    Text("Battery")
                        .font(.caption)
                    if let pct = viewModel.status.batPct {
                        Text("\(pct)%")
                            .font(.title3)
                            .foregroundColor(pct < 20 ? .red : .green)
                    } else {
                        Text("--")
                            .font(.title3)
                    }
                }
                
                Spacer()
                
                VStack {
                    Text("Mode")
                        .font(.caption)
                    Text(modeText)
                        .font(.title3)
                        .foregroundColor(viewModel.pidMode == 1 ? .orange : .blue)
                }
                
                Spacer()
                
                VStack {
                    Text("Power")
                        .font(.caption)
                    Text(viewModel.status.power ? "ON" : "OFF")
                        .font(.title3)
                        .foregroundColor(viewModel.status.power ? .green : .red)
                }
            }
        }
        .padding()
        .background(Color(uiColor: .systemGray6))
        .cornerRadius(10)
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
        Button(action: { selected = index }) {
            Text(title)
                .font(.subheadline)
                .padding(.vertical, 8)
                .padding(.horizontal, 12)
                .background(selected == index ? Color.accentColor : Color.clear)
                .foregroundColor(selected == index ? .white : .primary)
                .cornerRadius(8)
        }
        .buttonStyle(.plain)
    }
}


fileprivate struct AutoModeView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @Binding var kpString: String
    @Binding var kiString: String
    @Binding var kdString: String
    
    var body: some View {
        VStack(alignment: .leading, spacing: 18) {
            Text("Auto Mode")
                .font(.title2)
            VStack(alignment: .leading, spacing: 12) {
                // Inline temperature chart
                TemperatureChartView(viewModel: viewModel)
                
                // Setpoint control (displayed in selected unit)
                VStack(alignment: .leading, spacing: 6) {
                    // Helper conversions
                    func displayValue(_ celsius: Double) -> Double { viewModel.tempUnit == "C" ? celsius : (celsius * 9.0/5.0 + 32.0) }
                    func toCelsius(_ display: Double) -> Double { viewModel.tempUnit == "C" ? display : ((display - 32.0) * 5.0/9.0) }
                    func displayRange() -> ClosedRange<Double> { viewModel.tempUnit == "C" ? 30...315 : (86...599) }
                    func displayStep() -> Double { viewModel.tempUnit == "C" ? 0.5 : 1.0 }

                    let disp = displayValue(viewModel.setpoint)
                    Text("Setpoint: \(String(format: "%.1f\(viewModel.tempUnit)", disp))")
                    Slider(value: Binding(
                        get: { displayValue(viewModel.setpoint) },
                        set: { newVal in
                            let c = toCelsius(newVal)
                            // Update UI copy and persist to Auto slot
                            viewModel.setpoint = c
                            viewModel.setSetpointForMode(0, c)
                        }
                    ), in: displayRange(), step: displayStep())
                }
                
                // PID Tunings
                VStack(alignment: .leading, spacing: 8) {
                    Text("PID Tunings")
                        .font(.headline)
                    
                    HStack {
                        Text("Kp:")
                        TextField("Kp", text: $kpString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 120)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kpString) { val in
                                if let d = Double(val) {
                                    viewModel.kp = d
                            }
                        }
                        Spacer()
                    }
                    
                    HStack {
                        Text("Ki:")
                        TextField("Ki", text: $kiString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 120)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kiString) { val in
                                if let d = Double(val) {
                                    viewModel.ki = d
                            }
                        }
                        Spacer()
                    }
                    
                    HStack {
                        Text("Kd:")
                        TextField("Kd", text: $kdString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 120)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kdString) { val in
                                if let d = Double(val) {
                                    viewModel.kd = d
                            }
                        }
                        Spacer()
                    }
                    
                    HStack(spacing: 12) {
                        Button("Update PID") {
                            // Update PID for Auto mode (0) and persist
                            viewModel.updatePIDForMode(0, kp: viewModel.kp, ki: viewModel.ki, kd: viewModel.kd)
                        }
                        .buttonStyle(.borderedProminent)

                        Button("Load from Device") {
                            // Try to read the current PID values by reading the characteristics (ViewModel will read on connect)
                            // If you need direct read triggers, add methods in viewModel to read specific characteristics.
                        }
                        .buttonStyle(.bordered)

                        Button("Reset to Default PID") {
                            viewModel.resetPIDForMode(0)
                        }
                        .buttonStyle(.bordered)

                        Spacer()
                    }
                }
                .padding(.top, 6)
            }
            .padding()
            .background(Color(.systemBackground))
            .cornerRadius(14)
            .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
        }
        .frame(maxWidth: .infinity)
        .padding()
    }
}


// MARK: - ManualModeView

fileprivate struct ManualModeView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    
    var body: some View {
        VStack(alignment: .leading, spacing: 18) {
            Text("Manual Mode")
                .font(.title2)
            VStack(alignment: .leading, spacing: 12) {
                // Inline temperature chart
                TemperatureChartView(viewModel: viewModel)
                
                VStack(alignment: .leading) {
                    Text("Manual Output: \(String(format: "%.0f%%", (viewModel.manualOutput / Double(max(1, viewModel.status.pwmMax))) * 100.0))")
                    Slider(
                        value: Binding(
                            get: { viewModel.manualOutput },
                            set: { newVal in
                                viewModel.setManualOutput(newVal)
                            }
                        ),
                        in: 0...Double(viewModel.status.pwmMax),
                        step: 1
                    ) {
                        Text("Manual Output")
                    }
                    .disabled(viewModel.pidMode == 1) // disable if in PID mode
                }
                
                Text("Note: Manual mode sends a fixed output value to the heater. Use with caution.")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            .padding()
            .background(Color(.systemBackground))
            .cornerRadius(14)
            .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
        }
        .frame(maxWidth: .infinity)
        .padding()
    }
}

fileprivate struct PresetView: View {
    let title: String
    @ObservedObject var viewModel: AtomizerViewModel
    @Binding var kpString: String
    @Binding var kiString: String
    @Binding var kdString: String
    let modeIndex: Int
    var body: some View {
        VStack(alignment: .leading, spacing: 18) {
            Text(title)
                .font(.title2)
            VStack(alignment: .leading, spacing: 12) {
                // Setpoint control
                VStack(alignment: .leading) {
                    // Helper conversions
                    func displayValue(_ celsius: Double) -> Double { viewModel.tempUnit == "C" ? celsius : (celsius * 9.0/5.0 + 32.0) }
                    func toCelsius(_ display: Double) -> Double { viewModel.tempUnit == "C" ? display : ((display - 32.0) * 5.0/9.0) }
                    func displayRange() -> ClosedRange<Double> { viewModel.tempUnit == "C" ? 30...315 : (86...599) }
                    func displayStep() -> Double { viewModel.tempUnit == "C" ? 0.5 : 1.0 }

                    let disp = displayValue(viewModel.setpoint)
                    Text("Setpoint: \(String(format: "%.1f\(viewModel.tempUnit)", disp))")
                    Slider(value: Binding(
                        get: { displayValue(viewModel.setpoint) },
                        set: { newVal in
                            let c = toCelsius(newVal)
                            // Update UI copy and persist to Auto slot
                            viewModel.setpoint = c
                            viewModel.setSetpointForMode(modeIndex, c)
                        }
                    ), in: displayRange(), step: displayStep())
                }
                
                // PID tunings
                VStack(alignment: .leading, spacing: 8) {
                    Text("PID Tunings")
                        .font(.headline)
                    
                    HStack {
                        Text("Kp:")
                        TextField("Kp", text: $kpString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 120)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kpString) { val in
                                if let d = Double(val) {
                                    viewModel.kp = d
                            }
                        }
                        Spacer()
                    }
                    
                    HStack {
                        Text("Ki:")
                        TextField("Ki", text: $kiString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 120)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kiString) { val in
                                if let d = Double(val) {
                                    viewModel.ki = d
                            }
                        }
                        Spacer()
                    }
                    
                    HStack {
                        Text("Kd:")
                        TextField("Kd", text: $kdString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 120)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kdString) { val in
                                if let d = Double(val) {
                                    viewModel.kd = d
                            }
                        }
                        Spacer()
                    }
                    
                    // Save/Reset actions
                    HStack(spacing: 12) {
                        Button("Update PID") {
                            // Update PID for Auto mode (0) and persist
                            viewModel.updatePIDForMode(modeIndex, kp: viewModel.kp, ki: viewModel.ki, kd: viewModel.kd)
                        }
                        .buttonStyle(.borderedProminent)

                        Button("Load from Device") {
                            // Try to read the current PID values by reading the characteristics (ViewModel will read on connect)
                            // If you need direct read triggers, add methods in viewModel to read specific characteristics.
                        }
                        .buttonStyle(.bordered)

                        Button("Reset to Default PID") {
                            viewModel.resetPIDForMode(modeIndex)
                        }
                        .buttonStyle(.bordered)

                        Spacer()
                    }
                }
                .padding(.top, 6)
            }
            .padding()
            .background(Color(.systemBackground))
            .cornerRadius(14)
            .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
        }
        .frame(maxWidth: .infinity)
        .padding()
    }
}

// MARK: - ConfigView

fileprivate struct ConfigView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @State private var defaultSetpointString = ""
    @State private var showDevicePicker = false
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 18) {
                // Device Info card
                VStack(alignment: .leading, spacing: 10) {
                    Text("Device Info")
                        .font(.headline)
                    
                    // Basic device information
                    HStack {
                        Text("Name:")
                        Spacer()
                        Text(viewModel.deviceName)
                            .fontWeight(.semibold)
                            .foregroundColor(.blue)
                    }
                    
                    HStack {
                        Text("Firmware:")
                        Spacer()
                        Text(viewModel.firmwareVersion)
                            .fontWeight(.semibold)
                            .foregroundColor(.blue)
                    }
                }
                .padding()
                .background(Color(.systemBackground))
                .cornerRadius(14)
                .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
                
                // PID Tuning card
                VStack(alignment: .leading, spacing: 10) {
                    Text("PID Tuning")
                        .font(.headline)
                    
                    // Kp, Ki, Kd sliders with text fields
                    HStack {
                        Text("Kp:")
                        Spacer()
                        TextField("Kp", text: $kpString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 80)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kpString) { val in
                                if let d = Double(val) {
                                    viewModel.kp = d
                            }
                            }
                    }
                    
                    HStack {
                        Text("Ki:")
                        Spacer()
                        TextField("Ki", text: $kiString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 80)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kiString) { val in
                                if let d = Double(val) {
                                    viewModel.ki = d
                            }
                            }
                    }
                    
                    HStack {
                        Text("Kd:")
                        Spacer()
                        TextField("Kd", text: $kdString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 80)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: kdString) { val in
                                if let d = Double(val) {
                                    viewModel.kd = d
                            }
                            }
                    }
                    
                    // Update button
                    Button("Update PID") {
                        viewModel.updatePIDForMode(viewModel.pidMode, kp: viewModel.kp, ki: viewModel.ki, kd: viewModel.kd)
                    }
                    .buttonStyle(.borderedProminent)
                }
                .padding()
                .background(Color(.systemBackground))
                .cornerRadius(14)
                .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
                
                // Setpoint configuration card
                VStack(alignment: .leading, spacing: 10) {
                    Text("Setpoint Configuration")
                        .font(.headline)
                    
                    // Default setpoint field
                    HStack {
                        Text("Default Setpoint:")
                        Spacer()
                        TextField("Setpoint", text: $defaultSetpointString)
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .keyboardType(.decimalPad)
                            .frame(width: 100)
                            .multilineTextAlignment(.trailing)
                            .onChange(of: defaultSetpointString) { val in
                                if let d = Double(val) {
                                    viewModel.defaultSetpoint = d
                            }
                            }
                    }
                    
                    // Save button
                    Button("Save Default Setpoint") {
                        viewModel.saveConfigState()
                    }
                    .buttonStyle(.borderedProminent)
                }
                .padding()
                .background(Color(.systemBackground))
                .cornerRadius(14)
                .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
                
                // App Title & Name Card
                VStack(alignment: .leading, spacing: 10) {
                    Text("App Branding")
                        .font(.headline)
                    TextField("App Title", text: $viewModel.appTitle)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .padding(.vertical, 4)
                    TextField("App Name", text: $viewModel.appName)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .padding(.vertical, 4)
                    Button("Save Branding") {
                        viewModel.saveConfigState()
                    }
                    .buttonStyle(.borderedProminent)
                }
                .padding()
                .background(Color(.systemBackground))
                .cornerRadius(14)
                .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
                
                // Notification Card
                VStack(alignment: .leading, spacing: 10) {
                    Text("Notifications")
                        .font(.headline)
                    Toggle("Enable App Notifications", isOn: $viewModel.notificationsEnabled)
                        .toggleStyle(SwitchToggleStyle())
                        .onChange(of: viewModel.notificationsEnabled) { _ in viewModel.saveConfigState() }
                }
                .padding()
                .background(Color(.systemBackground))
                .cornerRadius(14)
                .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
                
                // Display Options Card - each toggle in its own row
                VStack(alignment: .leading, spacing: 0) {
                    Text("Display Options")
                        .font(.headline)
                        .padding(.bottom, 8)
                    Divider()
                    ToggleRow(label: "Show Chart Grid", isOn: $viewModel.showChartGrid)
                    Divider()
                    ToggleRow(label: "Show Time Labels", isOn: $viewModel.showChartTimeLabels)
                    Divider()
                    ToggleRow(label: "Smoothing (Canvas fallback)", isOn: $viewModel.smoothingFallback)
                    Divider()
                    ToggleRow(label: "Dual-axis Output", isOn: $viewModel.dualAxisOutput)
                    Divider()
                    HStack {
                        Text("History Window")
                        Spacer()
                        Text("\(viewModel.historyLimit)")
                            .foregroundColor(.secondary)
                    }
                    Slider(value: Binding(
                        get: { Double(viewModel.historyLimit) },
                        set: { newVal in viewModel.setHistoryLimit(Int(newVal)) }
                    ), in: 20...1000, step: 10)
                    .padding(.vertical, 8)
                }
                .padding()
                .background(Color(.systemBackground))
                .cornerRadius(14)
                .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
                
                // BLE Device Picker Card
                VStack(alignment: .leading, spacing: 10) {
                    Text("Bluetooth Devices")
                        .font(.headline)
                    Button("Scan for Devices") {
                        viewModel.startScanning()
                        showDevicePicker = true
                    }
                    .buttonStyle(.borderedProminent)
                    if showDevicePicker {
                        ForEach(viewModel.discoveredPeripherals, id: \ .peripheral.identifier) { item in
                            HStack {
                                Text(item.peripheral.name ?? "Unknown")
                                Spacer()
                                Button(viewModel.isConnected && viewModel.atomizerPeripheral?.identifier == item.peripheral.identifier ? "Disconnect" : "Connect") {
                                    if viewModel.isConnected && viewModel.atomizerPeripheral?.identifier == item.peripheral.identifier {
                                        viewModel.disconnect()
                                    } else {
                                        viewModel.connect(to: item.peripheral)
                                    }
                                }
                                .buttonStyle(.bordered)
                            }
                            Divider()
                        }
                    }
                }
                .padding()
                .background(Color(.systemBackground))
                .cornerRadius(14)
                .shadow(color: Color.black.opacity(0.07), radius: 4, x: 0, y: 2)
            }
            .frame(maxWidth: .infinity)
            .padding()
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
                .font(.body)
            Spacer()
            Toggle("", isOn: $isOn)
                .labelsHidden()
                .toggleStyle(SwitchToggleStyle())
        }
        .padding(.vertical, 8)
    }
}

fileprivate struct ConnectionView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @Binding var showForgetConfirmation: Bool
    
    var body: some View {
        VStack {
            VStack(alignment: .leading, spacing: 14) {
                HStack(spacing: 12) {
                    Circle()
                        .fill(viewModel.isConnected ? Color.green : Color.black)
                        .frame(width: 16, height: 16)
                        .overlay(Circle().stroke(Color.gray.opacity(0.2), lineWidth: 1))
                        .shadow(color: viewModel.isConnected ? Color.green.opacity(0.3) : Color.black.opacity(0.2), radius: 4, x: 0, y: 1)
                    Text("Device Connection")
                        .font(.headline)
                    Spacer()
                    Group {
                        if viewModel.isConnected {
                            Text("Connected")
                                .foregroundColor(.green)
                                .font(.subheadline)
                        } else if viewModel.isScanning {
                            Text("Scanning...")
                                .foregroundColor(.orange)
                                .font(.subheadline)
                        } else {
                            Text("Disconnected")
                                .foregroundColor(.black)
                                .font(.subheadline)
                        }
                    }
                }
                Divider()
                if viewModel.isConnected {
                    HStack(spacing: 12) {
                        Button("Disconnect") {
                            viewModel.disconnect()
                        }
                        .buttonStyle(.bordered)
                        Button("Forget Device") {
                            showForgetConfirmation = true
                        }
                        .buttonStyle(.bordered)
                        .tint(.red)
                        Spacer()
                    }
                    HStack {
                        VStack(alignment: .leading, spacing: 2) {
                            Text("Auto Reconnect:")
                                .font(.caption)
                            Text(viewModel.shouldAutoReconnect ? "Enabled" : "Disabled")
                                .font(.caption2)
                                .foregroundColor(.secondary)
                        }
                        Spacer()
                        VStack(alignment: .leading, spacing: 2) {
                            Text("RSSI:")
                                .font(.caption)
                            Text("—")
                                .font(.caption2)
                                .foregroundColor(.secondary)
                        }
                    }
                } else if viewModel.isScanning {
                    VStack(spacing: 8) {
                        ProgressView()
                        Text("Scanning for Atomizer devices...")
                            .font(.caption)
                            .foregroundColor(.secondary)
                        if viewModel.discoveredPeripherals.isEmpty {
                            Text("No devices found yet.")
                                .font(.caption2)
                                .foregroundColor(.secondary)
                        } else {
                            List {
                                ForEach(viewModel.discoveredPeripherals, id: \ .peripheral.identifier) { item in
                                    HStack {
                                        VStack(alignment: .leading) {
                                            Text(item.peripheral.name ?? "Unknown Device")
                                            Text(item.peripheral.identifier.uuidString)
                                                .font(.caption2)
                                                .foregroundColor(.secondary)
                                        }
                                        Spacer()
                                        Text("\(item.rssi.intValue)")
                                            .font(.caption2)
                                            .foregroundColor(.secondary)
                                        Button("Connect") {
                                            viewModel.connect(to: item.peripheral)
                                        }
                                        .buttonStyle(.borderedProminent)
                                        .controlSize(.small)
                                    }
                                    .padding(6)
                                    .background(RoundedRectangle(cornerRadius: 10).fill(Color(.systemGray6)))
                                }
                            }
                            .frame(maxHeight: 180)
                            .listStyle(PlainListStyle())
                        }
                        HStack(spacing: 12) {
                            Button("Cancel Scan") {
                                viewModel.cancelScan()
                            }
                            .buttonStyle(.bordered)
                            Button("Stop") {
                                viewModel.stopScanning()
                            }
                            .buttonStyle(.bordered)
                            Spacer()
                        }
                    }
                } else {
                    VStack(spacing: 8) {
                        Text("Make sure Bluetooth is enabled and the Atomizer is powered on.")
                            .font(.caption)
                            .foregroundColor(.secondary)
                            .multilineTextAlignment(.center)
                        HStack(spacing: 12) {
                            Button("Connect") {
                                viewModel.startScanning()
                            }
                            .buttonStyle(.borderedProminent)
                            Button("Refresh Discovery") {
                                viewModel.startScanning()
                            }
                            .buttonStyle(.bordered)
                            Spacer()
                        }
                    }
                }
            }
            .padding()
            .background(Color(.systemBackground))
            .cornerRadius(16)
            .shadow(color: Color.black.opacity(0.09), radius: 6, x: 0, y: 3)
        }
        .padding(.horizontal)
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
