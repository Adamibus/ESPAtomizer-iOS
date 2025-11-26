//
//  AtomizerViewModel.swift
//  ESPAtomizer-iOS
//
//  Updated and cleaned version
//  Created by ChatGPT and Adam Dinjian
//

import Foundation
import SwiftUI
import Combine
import CoreBluetooth

// MARK: - AtomizerStatus

struct AtomizerStatus: Codable {
    var temp: Double?
    var setpoint: Double
    var kp: Double
    var ki: Double
    var kd: Double
    var manual: Bool
    var power: Bool
    var out: Double
    var pwmMax: Int
    var spmode: Bool
    var batV: Double?
    var batPct: Int?
    var tcConn: Bool?

    enum CodingKeys: String, CodingKey {
        case temp, setpoint, kp, ki, kd, manual, power, out, pwmMax, spmode, batV, batPct, tcConn
    }

    init(
        temp: Double? = nil,
        setpoint: Double = 200.0,
        kp: Double = 10.0,
        ki: Double = 0.5,
        kd: Double = 50.0,
        manual: Bool = false,
        power: Bool = false,
        out: Double = 0.0,
        pwmMax: Int = 1023,
        spmode: Bool = false,
        batV: Double? = nil,
        batPct: Int? = nil,
        tcConn: Bool? = nil
    ) {
        self.temp = temp
        self.setpoint = setpoint
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.manual = manual
        self.power = power
        self.out = out
        self.pwmMax = pwmMax
        self.spmode = spmode
        self.batV = batV
        self.batPct = batPct
        self.tcConn = tcConn
    }

    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        temp = try container.decodeIfPresent(Double.self, forKey: .temp)
        setpoint = try container.decode(Double.self, forKey: .setpoint)
        kp = try container.decode(Double.self, forKey: .kp)
        ki = try container.decode(Double.self, forKey: .ki)
        kd = try container.decode(Double.self, forKey: .kd)
        manual = try container.decode(Bool.self, forKey: .manual)
        power = try container.decode(Bool.self, forKey: .power)
        out = try container.decode(Double.self, forKey: .out)
        pwmMax = try container.decode(Int.self, forKey: .pwmMax)
        spmode = try container.decode(Bool.self, forKey: .spmode)
        batV = try container.decodeIfPresent(Double.self, forKey: .batV)
        batPct = try container.decodeIfPresent(Int.self, forKey: .batPct)
        tcConn = try container.decodeIfPresent(Bool.self, forKey: .tcConn)
    }
}


// MARK: - AtomizerViewModel

/// ViewModel handling BLE comms and published properties for the ESP Atomizer.
/// Public APIs are intentionally preserved so your UI code continues to work.
class AtomizerViewModel: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    @Published var appTitle: String = "Adamizer"
    @Published var appName: String = "Adamizer"

    // MARK: - Published UI state

    @Published var status = AtomizerStatus()
    @Published var isConnected = false
    @Published var shouldAutoReconnect = true
    @Published var powerToggle = false            // bound to UI toggle (kept in sync with status.power)
    @Published var setpoint: Double = 200.0      // UI current setpoint control (mirrors status.setpoint)
    @Published var kp: Double = 10.0
    @Published var ki: Double = 0.5
    @Published var kd: Double = 50.0
    @Published var manualMode = false
    @Published var pidMode = 0
    @Published var manualOutput = 0.0
    @Published var defaultSetpoint = 200.0
    @Published var tempUnit: String = "C" // 'C' or 'F', persisted
    // Per-mode persisted state
    @Published var setpoints: [Double] = [200.0,200.0,200.0,200.0,200.0]
    struct PID: Codable { var kp: Double; var ki: Double; var kd: Double }
    @Published var pidTunings: [PID] = Array(repeating: PID(kp:10.0, ki:0.5, kd:50.0), count: 5)

    // Small rolling history for charting (time-series)
    struct DataPoint: Identifiable, Codable {
        var id = UUID()
        var time: Date
        var temp: Double?
        var setpoint: Double
        var outputPct: Double
    }
    @Published var history: [DataPoint] = []
    // Configurable history limit (persisted)
    @Published var historyLimit: Int = 120
    private var lastHistoryAppend: Date? = nil
    private let historyAppendMS: TimeInterval = 0.250 // 250 ms between samples (matches HTML GRAPH_UPDATE_MS)

    // Chart display options (persisted)
    @Published var showChartGrid: Bool = true
    @Published var showChartTimeLabels: Bool = true
    @Published var smoothingFallback: Bool = true
    @Published var dualAxisOutput: Bool = false
    // Master toggle to show/hide the entire chart UI
    @Published var showTemperatureChart: Bool = true

    // Chart refresh rate in milliseconds (persisted)
    @Published var chartRefreshRateMS: Int = 250 // default 250ms

    // Scanning & discovery
    @Published var isScanning = false
    @Published var discoveredPeripherals: [(peripheral: CBPeripheral, rssi: NSNumber)] = []

    // MARK: - BLE UUIDs

    // Primary service UUID
    private let serviceUUID = CBUUID(string: "b09aa6b5-0f22-4d9c-9dbc-6e3c7d9b2f0a")

    // Characteristic UUIDs used by firmware (keep in sync with device)
    private let uuidEnable   = CBUUID(string: "3f1a0001-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidSetpoint = CBUUID(string: "3f1a0002-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidKp       = CBUUID(string: "3f1a0003-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidKi       = CBUUID(string: "3f1a0004-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidKd       = CBUUID(string: "3f1a0005-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidMode     = CBUUID(string: "3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidTemp     = CBUUID(string: "3f1a0007-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidOut      = CBUUID(string: "3f1a0008-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidBat      = CBUUID(string: "3f1a0009-2a8d-4a54-8f2f-b7cd2b4b8001")
    private let uuidModeRead = CBUUID(string: "3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8002")
    private let uuidDefaultSp = CBUUID(string: "3f1a000a-2a8d-4a54-8f2f-b7cd2b4b8001")

    // Map of discovered characteristics for easier writes/reads
    private var characteristics = [CBUUID: CBCharacteristic]()

    // CoreBluetooth objects
    private var centralManager: CBCentralManager!
    private var atomizerPeripheral: CBPeripheral?

    // Keys for state preservation and saved peripheral id
    private let centralRestoreKey = "com.adinj.atomizer.central.restore"
    private let savedPeripheralKey = "atomizerSavedPeripheralUUID"

    // Expose transient UI errors for write/read failures
    @Published var lastErrorMessage: String? = nil

    // Timing helpers
    private var lastScanStart: Date?
    private let minScanInterval: TimeInterval = 2.0     // don't restart scan within this interval

    // MARK: - Computed

    /// Safely compute output percentage (guard against pwmMax == 0)
    var outputPercentage: Double {
        guard status.pwmMax > 0 else { return 0.0 }
        return (status.out / Double(status.pwmMax)) * 100.0
    }

    // MARK: - Init

    /// Initialize the view model. Pass `startCentral=false` to avoid creating a CBCentralManager
    /// (useful for SwiftUI previews and unit tests where CoreBluetooth isn't available).
    init(startCentral: Bool = true) {
        super.init()
        // restore persisted state
        restoreConfigState()
        if startCentral {
            let opts: [String: Any] = [CBCentralManagerOptionRestoreIdentifierKey: centralRestoreKey]
            centralManager = CBCentralManager(delegate: self, queue: nil, options: opts)
        }
    }

    // MARK: - Persistence

    private let configKey = "atomizerConfigState"

    func saveConfigState() {
        let encoder = JSONEncoder()
        struct Saved: Codable {
            var setpoints: [Double]
            var pidTunings: [PID]
            var manualOutput: Double
            var defaultSetpoint: Double
            var tempUnit: String
            var historyLimit: Int
            var showChartGrid: Bool
            var showChartTimeLabels: Bool
            var smoothingFallback: Bool
            var dualAxisOutput: Bool
            var showTemperatureChart: Bool
            var chartRefreshRateMS: Int
        }
        let saved = Saved(
            setpoints: setpoints,
            pidTunings: pidTunings,
            manualOutput: manualOutput,
            defaultSetpoint: defaultSetpoint,
            tempUnit: tempUnit,
            historyLimit: historyLimit,
            showChartGrid: showChartGrid,
            showChartTimeLabels: showChartTimeLabels,
            smoothingFallback: smoothingFallback,
            dualAxisOutput: dualAxisOutput,
            showTemperatureChart: showTemperatureChart,
            chartRefreshRateMS: chartRefreshRateMS
        )
        if let data = try? encoder.encode(saved) {
            UserDefaults.standard.set(data, forKey: configKey)
        }
    }

    func restoreConfigState() {
        if let data = UserDefaults.standard.data(forKey: configKey) {
            let decoder = JSONDecoder()
            // Decode robustly to support older saved formats that may not include all keys
            struct Saved: Codable {
                var setpoints: [Double]?
                var pidTunings: [PID]?
                var manualOutput: Double?
                var defaultSetpoint: Double?
                var tempUnit: String?
                var historyLimit: Int?
                var showChartGrid: Bool?
                var showChartTimeLabels: Bool?
                var smoothingFallback: Bool?
                var dualAxisOutput: Bool?
                var showTemperatureChart: Bool?
                var chartRefreshRateMS: Int?
            }
            if let saved = try? decoder.decode(Saved.self, from: data) {
                if let sps = saved.setpoints, sps.count == 5 { self.setpoints = sps }
                if let pids = saved.pidTunings, pids.count == 5 { self.pidTunings = pids }
                if let mo = saved.manualOutput { self.manualOutput = mo }
                if let ds = saved.defaultSetpoint { self.defaultSetpoint = ds }
                // default to Celsius if not present
                self.tempUnit = saved.tempUnit ?? self.tempUnit
                if let hl = saved.historyLimit { self.historyLimit = hl }
                if let sg = saved.showChartGrid { self.showChartGrid = sg }
                if let st = saved.showChartTimeLabels { self.showChartTimeLabels = st }
                if let sf = saved.smoothingFallback { self.smoothingFallback = sf }
                if let da = saved.dualAxisOutput { self.dualAxisOutput = da }
                if let stc = saved.showTemperatureChart { self.showTemperatureChart = stc }
                if let cr = saved.chartRefreshRateMS { self.chartRefreshRateMS = cr }
            }
        }
    }


    func setTempUnit(_ unit: String) {
        guard unit == "C" || unit == "F" else { return }
        tempUnit = unit
        saveConfigState()
    }

    // MARK: - Scanning & Connection APIs (Public)

    /// Start scanning for peripherals (no service filter so we can match by name/advertisement)
    func startScanning() {
        // Only scan when Bluetooth is powered on
        guard centralManager.state == .poweredOn else {
            debugPrint("startScanning: Bluetooth not powered on (state=\(centralManager.state.rawValue))")
            return
        }

        // Prevent rapid re-starts
        if isScanning {
            debugPrint("startScanning: already scanning")
            return
        }
        if let last = lastScanStart, Date().timeIntervalSince(last) < minScanInterval {
            debugPrint("startScanning: last scan started too recently — skipping")
            return
        }

        discoveredPeripherals.removeAll()
        isScanning = true
        lastScanStart = Date()

        // scanning without service filter to catch devices that might not advertise service UUID
        centralManager.scanForPeripherals(withServices: nil, options: [CBCentralManagerScanOptionAllowDuplicatesKey: false])
        debugPrint("Started scanning for Atomizer peripherals...")
        // Stop scanning automatically after 10 seconds unless stopped earlier
        DispatchQueue.main.asyncAfter(deadline: .now() + 10.0) { [weak self] in
            guard let self = self else { return }
            if self.isScanning {
                self.stopScanning()
            }
        }
    }

    /// Stop any active scan
    func stopScanning() {
        guard isScanning else { return }
        centralManager.stopScan()
        isScanning = false
        debugPrint("Stopped scanning.")
    }

    /// Connect to a peripheral you discovered
    func connect(to peripheral: CBPeripheral) {
        // If we are connected to another peripheral, disconnect first
        if let current = atomizerPeripheral, current.identifier != peripheral.identifier {
            disconnect()
        }

        atomizerPeripheral = peripheral
        atomizerPeripheral?.delegate = self

        // Stop scanning before connecting
        stopScanning()
        discoveredPeripherals.removeAll()
        centralManager.connect(peripheral, options: nil)
        debugPrint("Connecting to peripheral \(peripheral.name ?? peripheral.identifier.uuidString)...")
    }

    /// Cancel discovery without connecting
    func cancelScan() {
        stopScanning()
        discoveredPeripherals.removeAll()
    }

    /// Disconnect current peripheral
    func disconnect() {
        if let p = atomizerPeripheral {
            centralManager.cancelPeripheralConnection(p)
        }
        atomizerPeripheral = nil
        isConnected = false
        characteristics.removeAll()
    }

    /// Forget the device: disables auto reconnect and disconnects now
    func forgetDevice() {
        shouldAutoReconnect = false
        // Clear persisted saved peripheral UUID so the app won't attempt to auto-restore
        UserDefaults.standard.removeObject(forKey: savedPeripheralKey)
        UserDefaults.standard.synchronize()
        disconnect()
    }

    // MARK: - CBCentralManagerDelegate

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn:
            debugPrint("CBCentralManager: poweredOn")
            // Optionally begin scanning automatically (if you want immediate discovery)
            // Only start if not already connected and not scanning
            // If we have a saved peripheral identifier, attempt to retrieve & reconnect it first
            if !isConnected {
                if attemptRestoreSavedPeripheral() {
                    // restored or connecting — skip automatic scan
                } else if !isScanning {
                    startScanning()
                }
            }
        case .poweredOff:
            debugPrint("CBCentralManager: poweredOff")
            disconnect()
        case .unauthorized:
            debugPrint("CBCentralManager: unauthorized")
        case .unsupported:
            debugPrint("CBCentralManager: unsupported")
        default:
            debugPrint("CBCentralManager: state \(central.state.rawValue)")
        }
    }

    // MARK: - State preservation / restoration

    func centralManager(_ central: CBCentralManager, willRestoreState dict: [String : Any]) {
        debugPrint("centralManager willRestoreState: keys=\(dict.keys)")
        // CoreBluetooth may provide peripherals that were connected or pending when the app was terminated.
        if let peripherals = dict[CBCentralManagerRestoredStatePeripheralsKey] as? [CBPeripheral], let p = peripherals.first {
            debugPrint("Restored peripheral from system state: \(p.identifier.uuidString)")
            atomizerPeripheral = p
            atomizerPeripheral?.delegate = self
            // Try to connect to the restored peripheral (system may already have restored connection state)
            if central.state == .poweredOn {
                central.connect(p, options: nil)
            }
        }
    }

    /// Attempt to retrieve a previously saved peripheral by UUID and connect to it.
    /// Returns true if a retrieve/connect was initiated (or the peripheral was already available).
    private func attemptRestoreSavedPeripheral() -> Bool {
        guard let saved = UserDefaults.standard.string(forKey: savedPeripheralKey), let uuid = UUID(uuidString: saved) else {
            return false
        }
        debugPrint("Attempting to restore saved peripheral: \(saved)")
        let peripherals = centralManager.retrievePeripherals(withIdentifiers: [uuid])
        if let p = peripherals.first {
            debugPrint("Retrieved peripheral from CoreBluetooth cache: \(p.identifier.uuidString)")
            atomizerPeripheral = p
            atomizerPeripheral?.delegate = self
            centralManager.connect(p, options: nil)
            return true
        }
        return false
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        debugPrint("Discovered: \(peripheral.name ?? "Unknown") rssi:\(RSSI)")
        // Keep unique list by identifier
        if !discoveredPeripherals.contains(where: { $0.peripheral.identifier == peripheral.identifier }) {
            discoveredPeripherals.append((peripheral, RSSI))
        }
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        debugPrint("Connected to \(peripheral.name ?? peripheral.identifier.uuidString)")
        isConnected = true
        shouldAutoReconnect = true
        atomizerPeripheral = peripheral
        characteristics.removeAll()

        // Persist the peripheral's identifier so we can attempt to restore across launches
        UserDefaults.standard.set(peripheral.identifier.uuidString, forKey: savedPeripheralKey)
        UserDefaults.standard.synchronize()

        // Discover only the expected service to speed up discovery
        peripheral.discoverServices([serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        debugPrint("Failed to connect: \(error?.localizedDescription ?? "unknown")")
        isConnected = false
        // no change to shouldAutoReconnect here; connection may be retried by higher layer
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        debugPrint("Disconnected from peripheral: \(error?.localizedDescription ?? "none")")
        isConnected = false
        atomizerPeripheral = nil
        characteristics.removeAll()
        // discoveredPeripherals intentionally preserved as empty state to encourage re-scan

        // Attempt reconnect if enabled, but avoid overlapping scans/connections
        if shouldAutoReconnect && !isScanning && !isConnected {
            DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) { [weak self] in
                guard let self = self else { return }
                if self.shouldAutoReconnect && !self.isConnected && !self.isScanning {
                    self.startScanning()
                }
            }
        }
    }

    // MARK: - CBPeripheralDelegate

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let e = error {
            debugPrint("Service discovery error: \(e.localizedDescription)")
            return
        }

        guard let services = peripheral.services else { return }

        for service in services where service.uuid == serviceUUID {
            // discover the set of characteristics we care about
            let characteristicUUIDs = [
                uuidEnable, uuidSetpoint, uuidKp, uuidKi, uuidKd,
                uuidMode, uuidTemp, uuidOut, uuidBat, uuidModeRead, uuidDefaultSp
            ]
            peripheral.discoverCharacteristics(characteristicUUIDs, for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        if let e = error {
            debugPrint("Characteristic discovery error: \(e.localizedDescription)")
            return
        }

        guard let chars = service.characteristics else { return }

        for characteristic in chars {
            characteristics[characteristic.uuid] = characteristic

            // Set notifications for characteristics that the device will push updates for
            if characteristic.uuid == uuidTemp || characteristic.uuid == uuidBat || characteristic.uuid == uuidModeRead || characteristic.uuid == uuidOut || characteristic.uuid == uuidEnable {
                peripheral.setNotifyValue(true, for: characteristic)
            }

            // Read initial value where appropriate
            peripheral.readValue(for: characteristic)
        }

        debugPrint("Discovered \(chars.count) characteristics")
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if let e = error {
            debugPrint("Characteristic read error for \(characteristic.uuid): \(e.localizedDescription)")
            return
        }

        guard let data = characteristic.value else {
            debugPrint("Characteristic \(characteristic.uuid) had no value")
            return
        }

        DispatchQueue.main.async { [weak self] in
            self?.updateStatusFromCharacteristic(characteristic.uuid, data: data)
        }
    }

    // MARK: - Characteristic value parsing

    /// Normalize a UTF8 string from Data by trimming whitespace/newlines
    private func normalizedString(from data: Data) -> String {
        // Use decoding that is resilient to weird bytes
        let s = String(decoding: data, as: UTF8.self)
        return s.trimmingCharacters(in: .whitespacesAndNewlines)
    }

    private func updateStatusFromCharacteristic(_ uuid: CBUUID, data: Data) {
        let s = normalizedString(from: data)

        switch uuid {
        case uuidTemp:
            if let v = Double(s) {
                status.temp = v
            } else {
                debugPrint("Temp parse failed for '\(s)'")
            }

        case uuidBat:
            // Battery might come as percent or voltage; try Int then Double
            if let pct = Int(s) {
                status.batPct = pct
            } else if let v = Double(s) {
                // If firmware sends a voltage, attempt to convert to percent if you have formula.
                // For now, just store voltage in batV and leave batPct nil
                status.batV = v
            } else {
                debugPrint("Battery parse failed for '\(s)'")
            }

        case uuidModeRead:
            if let v = Int(s) {
                pidMode = v
                status.manual = (v == 1)
                manualMode = status.manual
            } else {
                debugPrint("Mode parse failed for '\(s)'")
            }

        case uuidOut:
            // Out is PWM raw value from device
            if let v = Double(s) {
                status.out = v
            } else {
                debugPrint("Out parse failed for '\(s)'")
            }

        case uuidEnable:
            // Enable/disable (power) may be reported as '1'/'0' or 'ON'/'OFF'
            if let i = Int(s) {
                status.power = (i != 0)
            } else {
                let u = s.uppercased()
                if u == "ON" || u == "1" { status.power = true }
                else if u == "OFF" || u == "0" { status.power = false }
                else { debugPrint("Enable parse ambiguous for '\(s)'") }
            }

        case uuidSetpoint:
            if let v = Double(s) {
                status.setpoint = v
                setpoint = v
            }

        case uuidKp:
            if let v = Double(s) {
                status.kp = v
                kp = v
            }

        case uuidKi:
            if let v = Double(s) {
                status.ki = v
                ki = v
            }

        case uuidKd:
            if let v = Double(s) {
                status.kd = v
                kd = v
            }

        case uuidDefaultSp:
            if let v = Double(s) {
                defaultSetpoint = v
            }

        default:
            // Unknown characteristic — ignore or log
            debugPrint("Unhandled UUID \(uuid) payload '\(s)'")
        }

        // Keep UI boolean values in sync with status
        powerToggle = status.power
        // Append a data point for charting (throttled)
        appendHistory()
    }

    // Append a new DataPoint to the rolling history (throttled; kept on main thread)
    private func appendHistory() {
        let now = Date()
        if let last = lastHistoryAppend {
            if now.timeIntervalSince(last) < historyAppendMS { return }
        }
        lastHistoryAppend = now

        let dp = DataPoint(time: now, temp: status.temp, setpoint: status.setpoint, outputPct: outputPercentage)
        history.append(dp)
        if history.count > historyLimit {
            history.removeFirst(history.count - historyLimit)
        }
    }

    /// Update history limit (called from UI). Trim existing history if needed and persist.
    func setHistoryLimit(_ limit: Int) {
        historyLimit = max(10, min(1000, limit))
        if history.count > historyLimit {
            history.removeFirst(history.count - historyLimit)
        }
        saveConfigState()
    }

    // MARK: - Public write / control methods

    /// Toggle power (sends enable char)
    func togglePower() {
        let newPower = !status.power
        writeCharacteristic(uuidEnable, value: newPower ? "1" : "0")
        // Update UI immediately for responsiveness; device will report actual state via notifications if implemented
        status.power = newPower
        powerToggle = newPower
    }

    /// Set device setpoint (sends setpoint char)
    func setSetpoint(_ value: Double) {
        writeCharacteristic(uuidSetpoint, value: String(format: "%.1f", value))
        // Optimistically update UI copy
        status.setpoint = value
        setpoint = value
        // persist per-mode setpoint
        if pidMode >= 0 && pidMode < setpoints.count {
            setpoints[pidMode] = value
            saveConfigState()
            // reflect setpoint change in history (throttled)
            DispatchQueue.main.async { [weak self] in
                self?.appendHistory()
            }
        }
    }

    // Helper to get/set setpoint for a specific mode
    func getSetpointForMode(_ mode: Int) -> Double {
        guard mode >= 0 && mode < setpoints.count else { return defaultSetpoint }
        return setpoints[mode]
    }

    func setSetpointForMode(_ mode: Int, _ value: Double) {
        guard mode >= 0 && mode < setpoints.count else { return }
        writeCharacteristic(uuidSetpoint, value: String(format: "%.1f", value))
        setpoints[mode] = value
        if mode == pidMode {
            status.setpoint = value
            setpoint = value
        }
        saveConfigState()
        DispatchQueue.main.async { [weak self] in
            self?.appendHistory()
        }
    }

    func updatePIDForMode(_ mode: Int, kp: Double, ki: Double, kd: Double) {
        guard mode >= 0 && mode < pidTunings.count else { return }
        pidTunings[mode] = PID(kp: kp, ki: ki, kd: kd)
        // Persist the PID change for this mode
        saveConfigState()
        // If this is the currently active mode, apply to device as well
        if mode == pidMode {
            setPID(kp: kp, ki: ki, kd: kd)
        }
    }

    func resetPIDForMode(_ mode: Int) {
        guard mode >= 0 && mode < pidTunings.count else { return }
        let def = PID(kp: 10.0, ki: 0.5, kd: 50.0)
        pidTunings[mode] = def
        // Persist reset
        saveConfigState()
        // If current mode, apply to device
        if mode == pidMode {
            setPID(kp: def.kp, ki: def.ki, kd: def.kd)
        }
    }

    func resetAllPIDs() {
        let def = PID(kp: 10.0, ki: 0.5, kd: 50.0)
        for i in 0..<pidTunings.count { pidTunings[i] = def }
        saveConfigState()
        if isConnected {
            setPID(kp: def.kp, ki: def.ki, kd: def.kd)
        }
    }

    var manualOutputPercentage: Double {
        guard status.pwmMax > 0 else { return 0 }
        return (manualOutput / Double(status.pwmMax)) * 100.0
    }

    /// Set PID values (sends three writes)
    func setPID(kp: Double, ki: Double, kd: Double) {
        // Write using decimal formatting — ensure firmware expects this format
        writeCharacteristic(uuidKp, value: String(format: "%.3f", kp))
        writeCharacteristic(uuidKi, value: String(format: "%.3f", ki))
        writeCharacteristic(uuidKd, value: String(format: "%.3f", kd))

        // Update UI model optimistically
        status.kp = kp
        status.ki = ki
        status.kd = kd
        self.kp = kp
        self.ki = ki
        self.kd = kd
    }

    /// Set mode. Confirm the firmware expects string codes like "AUTO"/"MAN"/"U1"/"U2"
    func setMode(mode: Int) {
        let modeString: String
        switch mode {
        case 0: modeString = "AUTO"
        case 1: modeString = "MAN"
        case 2: modeString = "U1"
        case 3: modeString = "U2"
        default:
            debugPrint("setMode: unsupported mode \(mode)")
            return
        }
        writeCharacteristic(uuidMode, value: modeString)
        pidMode = mode
        status.manual = (mode == 1)
        manualMode = status.manual
    }

    /// Set manual output (sends raw value). Do not overwrite status.out — device will report real out.
    func setManualOutput(_ value: Double) {
        writeCharacteristic(uuidOut, value: String(format: "%.0f", value))
        manualOutput = value
        // Do NOT set status.out here. Let the device notify back the actual PWM output.
        // Persist manual output so the UI/mock state survives restarts
        saveConfigState()
        DispatchQueue.main.async { [weak self] in
            self?.appendHistory()
        }
    }

    /// Write default setpoint to device
    func setDefaultSetpoint(_ value: Double) {
        writeCharacteristic(uuidDefaultSp, value: String(format: "%.1f", value))
        defaultSetpoint = value
        // Persist default setpoint so it survives app restarts
        saveConfigState()
        DispatchQueue.main.async { [weak self] in
            self?.appendHistory()
        }
    }

    /// Generic write helper (public) — writes string as UTF-8 to a characteristic if available
    func writeCharacteristic(_ uuid: CBUUID, value: String) {
        guard let peripheral = atomizerPeripheral else {
            debugPrint("Cannot write \(uuid): no peripheral")
            lastErrorMessage = "Cannot write: no peripheral"
            scheduleClearError()
            return
        }
        guard let characteristic = characteristics[uuid] else {
            debugPrint("Cannot write \(uuid): characteristic not discovered")
            lastErrorMessage = "Characteristic not discovered"
            scheduleClearError()
            return
        }
        guard let data = value.data(using: .utf8) else {
            debugPrint("Cannot encode value '\(value)' for \(uuid)")
            lastErrorMessage = "Cannot encode value to UTF8"
            scheduleClearError()
            return
        }

        // Check that the characteristic supports write (or writeWithoutResponse)
        let props = characteristic.properties
        if !props.contains(.write) && !props.contains(.writeWithoutResponse) {
            debugPrint("Characteristic \(uuid) does not support write")
            lastErrorMessage = "Characteristic does not support write"
            scheduleClearError()
            return
        }

        // Use .withResponse when available so we get confirmation in didWriteValueFor
        let writeType: CBCharacteristicWriteType = props.contains(.write) ? .withResponse : .withoutResponse
        peripheral.writeValue(data, for: characteristic, type: writeType)
        debugPrint("Wrote to \(uuid): '\(value)'")
    }

    // CBPeripheralDelegate: write response handler
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        if let e = error {
            let msg = "Write failed for \(characteristic.uuid): \(e.localizedDescription)"
            debugPrint(msg)
            DispatchQueue.main.async { [weak self] in
                self?.lastErrorMessage = msg
                self?.scheduleClearError()
            }
        } else {
            // successful write — optionally clear previous write errors
            DispatchQueue.main.async { [weak self] in
                self?.lastErrorMessage = nil
            }
        }
    }

    private func scheduleClearError(after seconds: TimeInterval = 3.0) {
        DispatchQueue.main.asyncAfter(deadline: .now() + seconds) { [weak self] in
            self?.lastErrorMessage = nil
        }
    }

    // MARK: - Read helpers

    /// Read a characteristic by UUID if discovered
    func readCharacteristic(_ uuid: CBUUID) {
        guard let peripheral = atomizerPeripheral, let char = characteristics[uuid] else {
            debugPrint("readCharacteristic: missing char \(uuid)")
            return
        }
        peripheral.readValue(for: char)
    }

    /// Request read of all known characteristics we care about
    func requestAllReads() {
        let uuidsToRead = [uuidTemp, uuidBat, uuidModeRead, uuidOut, uuidSetpoint, uuidKp, uuidKi, uuidKd, uuidDefaultSp]
        for u in uuidsToRead {
            readCharacteristic(u)
        }
    }

    // MARK: - Deinit

    deinit {
        stopScanning()
        disconnect()
    }
}
