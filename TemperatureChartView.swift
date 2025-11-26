//
//  TemperatureChartView.swift
//  ESPAtomizer-iOS
//
//  Small SwiftUI chart wrapper using Apple's Charts to visualize temperature history.
//

import SwiftUI
#if canImport(Charts)
import Charts
#endif

struct TemperatureChartView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @State private var chartRefreshTrigger: Int = 0
    @State private var timer: Timer? = nil

    var body: some View {
        Group {
            if viewModel.showTemperatureChart {
#if canImport(Charts)
                if #available(iOS 16.0, *) {
                    ChartsImpl(viewModel: viewModel, chartRefreshTrigger: chartRefreshTrigger)
                } else {
                    CanvasFallback(viewModel: viewModel, chartRefreshTrigger: chartRefreshTrigger)
                }
#else
                CanvasFallback(viewModel: viewModel, chartRefreshTrigger: chartRefreshTrigger)
#endif
            }
        }
        .onAppear {
            startTimer()
        }
        .onDisappear {
            stopTimer()
        }
        .onChange(of: viewModel.chartRefreshRateMS) { _ in
            restartTimer()
        }
    }

    private func startTimer() {
        stopTimer()
        timer = Timer.scheduledTimer(withTimeInterval: Double(viewModel.chartRefreshRateMS) / 1000.0, repeats: true) { _ in
            chartRefreshTrigger += 1
        }
    }
    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }
    private func restartTimer() {
        stopTimer()
        startTimer()
    }
}

// MARK: - Charts implementation (iOS 16+)
#if canImport(Charts)
@available(iOS 16.0, *)
private struct ChartsImpl: View {
    @ObservedObject var viewModel: AtomizerViewModel
    var chartRefreshTrigger: Int // triggers chart redraw

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Temperature History (") + Text(viewModel.tempUnit) + Text(")")
                .font(.headline)

            Chart {
                ForEach(viewModel.history) { dp in
                    // convert to display units
                    let tempVal = dp.temp ?? 0.0
                    let dispTemp = (viewModel.tempUnit == "C") ? tempVal : (tempVal * 9.0/5.0 + 32.0)
                    let dispSp = (viewModel.tempUnit == "C") ? dp.setpoint : (dp.setpoint * 9.0/5.0 + 32.0)

                    LineMark(
                        x: .value("Time", dp.time),
                        y: .value("Temp", dispTemp)
                    )
                    .foregroundStyle(Color.red)
                    .interpolationMethod(.monotone)

                    LineMark(
                        x: .value("Time", dp.time),
                        y: .value("Setpoint", dispSp)
                    )
                    .foregroundStyle(Color.blue)
                    .interpolationMethod(.monotone)
                }
            }
            .chartXAxis {
                if viewModel.showChartTimeLabels {
                    AxisMarks(values: .automatic(desiredCount: 4))
                } else {
                    AxisMarks().hidden()
                }
            }
            .frame(height: 160)

            HStack(alignment: .center, spacing: 12) {
                VStack(alignment: .leading, spacing: 4) {
                    Text(String(format: "Output: %.0f%%", latestOutputPct))
                        .font(.subheadline)
                    ProgressView(value: latestOutputPct / 100.0)
                        .progressViewStyle(LinearProgressViewStyle(tint: Color.green))
                        .frame(maxWidth: 160)
                }

                Chart {
                    ForEach(viewModel.history) { dp in
                        AreaMark(
                            x: .value("Time", dp.time),
                            y: .value("OutPct", dp.outputPct)
                        )
                        .foregroundStyle(Gradient(colors: [Color.green.opacity(0.6), Color.green.opacity(0.15)]))
                        .interpolationMethod(.monotone)
                    }
                }
                .chartXAxis(.hidden)
                .chartYAxis(viewModel.dualAxisOutput ? .hidden : .hidden)
                .frame(height: 44)
            }
        }
        .padding(8)
        .background(Color(uiColor: .systemGray6))
        .cornerRadius(8)
    }

    private var latestOutputPct: Double {
        viewModel.history.last?.outputPct ?? viewModel.outputPercentage
    }
}
#endif

// MARK: - Canvas fallback for older OS or when Charts not available
private struct CanvasFallback: View {
    @ObservedObject var viewModel: AtomizerViewModel
    var chartRefreshTrigger: Int // triggers chart redraw

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Temperature History (") + Text(viewModel.tempUnit) + Text(")")
                .font(.headline)

            GeometryReader { geo in
                ZStack {
                    // background grid
                    Color.clear
                    Canvas { ctx, size in
                        let w = size.width
                        let h = size.height
                        ctx.stroke(Path { p in
                            p.move(to: CGPoint(x: 0, y: h * 0.0))
                            p.addLine(to: CGPoint(x: w, y: h * 0.0))
                        }, with: .color(.clear))
                        // draw series
                        let points = viewModel.history
                        guard points.count > 1 else { return }

                        // compute temp range
                        let temps = points.compactMap { $0.temp }
                        let allTemps = temps + points.map { $0.setpoint }
                        let minT = (allTemps.min() ?? 0) - 5
                        let maxT = (allTemps.max() ?? 1) + 5

                        func x(for idx: Int) -> CGFloat {
                            let pct = CGFloat(idx) / CGFloat(max(1, points.count - 1))
                            return pct * w
                        }

                        func yForTemp(_ t: Double) -> CGFloat {
                            // convert to display units
                            let d = (viewModel.tempUnit == "C") ? t : (t * 9.0/5.0 + 32.0)
                            let rMin = (viewModel.tempUnit == "C") ? minT : (minT * 9.0/5.0 + 32.0)
                            let rMax = (viewModel.tempUnit == "C") ? maxT : (maxT * 9.0/5.0 + 32.0)
                            let norm = (d - rMin) / (rMax - rMin)
                            return (1.0 - CGFloat(norm)) * h
                        }

                        // draw temp line (optionally smoothed)
                        var tempPath = Path()
                        var tempPoints: [CGPoint] = []
                        for (i, dp) in points.enumerated() {
                            if let t = dp.temp {
                                let pt = CGPoint(x: x(for: i), y: yForTemp(t))
                                tempPoints.append(pt)
                            }
                        }
                        if viewModel.smoothingFallback && tempPoints.count > 2 {
                            tempPath = Path.smoothedPath(points: tempPoints)
                        } else {
                            for (i, pt) in tempPoints.enumerated() {
                                if i == 0 { tempPath.move(to: pt) } else { tempPath.addLine(to: pt) }
                            }
                        }
                        ctx.stroke(tempPath, with: .color(.red), lineWidth: 2)

                        // draw setpoint line (no smoothing)
                        var spPath = Path()
                        for (i, dp) in points.enumerated() {
                            let pt = CGPoint(x: x(for: i), y: yForTemp(dp.setpoint))
                            if i == 0 { spPath.move(to: pt) } else { spPath.addLine(to: pt) }
                        }
                        ctx.stroke(spPath, with: .color(.blue), lineWidth: 1.5)
                    }
                }
            }
            .frame(height: 160)

            HStack(alignment: .center, spacing: 12) {
                VStack(alignment: .leading, spacing: 4) {
                    Text(String(format: "Output: %.0f%%", latestOutputPct))
                        .font(.subheadline)
                    ProgressView(value: latestOutputPct / 100.0)
                        .progressViewStyle(LinearProgressViewStyle(tint: Color.green))
                        .frame(maxWidth: 160)
                }

                // small sparkline drawn with Canvas
                GeometryReader { geo in
                    Canvas { ctx, size in
                        let w = size.width
                        let h = size.height
                        let points = viewModel.history
                        guard points.count > 1 else { return }
                        // output range 0..100
                        func x(_ idx: Int) -> CGFloat { CGFloat(idx) / CGFloat(max(1, points.count - 1)) * w }
                        func y(_ v: Double) -> CGFloat { (1.0 - CGFloat(v / 100.0)) * h }
                        var path = Path()
                        for (i, dp) in points.enumerated() {
                            let pt = CGPoint(x: x(i), y: y(dp.outputPct))
                            if i == 0 { path.move(to: pt) } else { path.addLine(to: pt) }
                        }
                        ctx.fill(path, with: .linearGradient(Gradient(colors: [Color.green.opacity(0.6), Color.green.opacity(0.1)]), startPoint: .top, endPoint: .bottom))
                    }
                }
                .frame(height: 44)
            }
        }
        .padding(8)
        .background(Color(uiColor: .systemGray6))
        .cornerRadius(8)
    }

    private var latestOutputPct: Double { viewModel.history.last?.outputPct ?? viewModel.outputPercentage }
}

// Path smoothing helper (Catmull-Rom -> Bezier approximation)
fileprivate extension Path {
    static func smoothedPath(points: [CGPoint]) -> Path {
        var path = Path()
        guard points.count > 1 else { return path }

        // Catmull-Rom to Bezier conversion
        func controlPoints(p0: CGPoint, p1: CGPoint, p2: CGPoint, p3: CGPoint) -> (CGPoint, CGPoint) {
            let smoothing: CGFloat = 0.2
            let d1 = hypot(p1.x - p0.x, p1.y - p0.y)
            let d2 = hypot(p2.x - p1.x, p2.y - p1.y)
            let d3 = hypot(p3.x - p2.x, p3.y - p2.y)

            var cp1 = CGPoint.zero
            var cp2 = CGPoint.zero

            if d1 + d2 != 0 {
                cp1.x = p2.x - (p3.x - p1.x) * smoothing
                cp1.y = p2.y - (p3.y - p1.y) * smoothing
            }
            if d2 + d3 != 0 {
                cp2.x = p1.x + (p2.x - p0.x) * smoothing
                cp2.y = p1.y + (p2.y - p0.y) * smoothing
            }
            return (cp1, cp2)
        }

        path.move(to: points[0])
        let n = points.count
        for i in 0..<(n - 1) {
            let p0 = i - 1 >= 0 ? points[i - 1] : points[i]
            let p1 = points[i]
            let p2 = points[i + 1]
            let p3 = i + 2 < n ? points[i + 2] : points[i + 1]

            let (cp1, cp2) = controlPoints(p0: p0, p1: p1, p2: p2, p3: p3)
            path.addCurve(to: p2, control1: cp1, control2: cp2)
        }
        return path
    }
}

struct TemperatureChartView_Previews: PreviewProvider {
    static var previews: some View {
        // Create a lightweight VM with sample history (do not start CoreBluetooth)
        let vm = AtomizerViewModel(startCentral: false)
        // seed sample
        for i in 0..<40 {
            let t = 180.0 + Double(i) * 0.5
            let sp = 200.0
            let out = Double(i % 20) * 5.0
            vm.history.append(AtomizerViewModel.DataPoint(time: Date().addingTimeInterval(Double(-40 + i)), temp: t, setpoint: sp, outputPct: out))
        }
        return TemperatureChartView(viewModel: vm)
            .previewLayout(.sizeThatFits)
            .padding()
    }
}
