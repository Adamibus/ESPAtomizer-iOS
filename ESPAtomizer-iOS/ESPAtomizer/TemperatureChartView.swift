//
//  TemperatureChartView.swift
//  ESPAtomizer-iOS
//
//  Small SwiftUI chart wrapper using Apple's Charts to visualize temperature history.
//

import SwiftUI
import Charts

struct TemperatureChartView: View {
    @ObservedObject var viewModel: AtomizerViewModel
    @State private var chartRefreshTrigger: Int = 0
    @State private var timer: Timer? = nil

    /// How often the chart is nudged to redraw. A constant, not a setting: it
    /// was a persisted property with no UI that never moved off this value, and
    /// it is a rendering detail rather than something to configure. This paces
    /// redraws only — sample arrival is driven by BLE notifications.
    private static let refreshInterval: TimeInterval = 0.25

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            if viewModel.showTemperatureChart {
                ChartsImpl(viewModel: viewModel, chartRefreshTrigger: chartRefreshTrigger)
            }
            // OUTSIDE the if: the swatch colors far more than the chart line
            // (power toggle, target stat, sliders), so hiding the chart must
            // never take the picker with it.
            ChartColorPicker(viewModel: viewModel)
        }
        .onAppear { startTimer() }
        .onDisappear { stopTimer() }
    }

    private func startTimer() {
        stopTimer()
        timer = Timer.scheduledTimer(withTimeInterval: Self.refreshInterval, repeats: true) { _ in
            // The timer is scheduled from .onAppear, so it fires on the main run
            // loop — but the compiler can't prove that through a Sendable closure.
            // Stating it explicitly is what Swift 6's strict concurrency will
            // require; without it this becomes an error rather than a warning.
            MainActor.assumeIsolated { chartRefreshTrigger += 1 }
        }
    }
    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }
}

// MARK: - Line segmentation
// A run of samples with no missing time between them. The chart draws one line
// per segment so a BLE dropout leaves a visible break, instead of a straight
// line calmly bridging thirty seconds the device never reported.
// Identified by TIMESTAMP, not position. Index-based ids shift every time the
// rolling buffer drops its oldest sample, so segment 0 kept describing different
// data each frame and Charts re-animated the whole line ~4×/second — the line
// appeared to twitch and re-draw constantly. Timestamps are unique and stable.
private struct ChartPoint: Identifiable {
    let time: Date
    let value: Double
    var id: Date { time }
}
private struct ChartSegment: Identifiable {
    let id: Date          // first point's time
    let points: [ChartPoint]
}

/// The samples that fall inside the visible window, plus the single sample just
/// before it.
///
/// The history buffer holds far more time than the chart shows (120 samples vs a
/// 15 s window), so most of it sits off the left edge. Trimming here means
/// out-of-window data never reaches the chart at all, rather than relying on it
/// to clip correctly — Charts does not reliably clip when the scale domain is
/// narrower than the data.
///
/// The one extra sample before the cutoff is deliberate: without it the line
/// would start at the first in-window point and visibly float away from the left
/// edge. With it, the segment crosses the boundary and gets clipped there.
private func windowed(
    _ history: [AtomizerViewModel.DataPoint],
    anchor: Date,
    window: TimeInterval
) -> [AtomizerViewModel.DataPoint] {
    let cutoff = anchor.addingTimeInterval(-window)
    guard let firstInside = history.firstIndex(where: { $0.time >= cutoff }) else { return [] }
    return Array(history[max(0, firstInside - 1)...])
}

/// Split history into unbroken runs. `value` returns nil for a sample that should
/// end the current run (e.g. a missing temperature reading).
private func segments(
    _ history: [AtomizerViewModel.DataPoint],
    gapThreshold: TimeInterval,
    value: (AtomizerViewModel.DataPoint) -> Double?
) -> [ChartSegment] {
    var out: [ChartSegment] = []
    var current: [ChartPoint] = []
    var lastTime: Date? = nil

    func flush() {
        guard let first = current.first else { return }
        out.append(ChartSegment(id: first.time, points: current))
        current = []
    }

    for dp in history {
        guard let v = value(dp) else { flush(); lastTime = nil; continue }
        if let prev = lastTime, dp.time.timeIntervalSince(prev) > gapThreshold { flush() }
        current.append(ChartPoint(time: dp.time, value: v))
        lastTime = dp.time
    }
    flush()
    return out
}

// MARK: - Chart color picker
// Three bare swatches under the plot. No labels and no header — a row of colored
// dots under a colored chart explains itself, and a "CHART COLOR" caption here
// would out-shout the axis labels it sits beside.
private struct ChartColorPicker: View {
    @ObservedObject var viewModel: AtomizerViewModel

    // Parallel to Theme.chartSeriesColors — VoiceOver only; nothing is drawn.
    private let names = ["White", "Gold", "Emerald"]

    var body: some View {
        HStack(spacing: 0) {
            ForEach(Array(Theme.chartSeriesColors.enumerated()), id: \.offset) { index, color in
                let isSelected = index == viewModel.chartColorIndex
                Button {
                    viewModel.chartColorIndex = index
                    viewModel.saveConfigState()
                } label: {
                    Circle()
                        .fill(color)
                        .frame(width: 14, height: 14)
                        // Full strength always. Dimming unselected swatches to 55%
                        // muddied them against the charcoal — the orange in
                        // particular read as brown. The glow and ring below carry
                        // the selection state on their own.
                        .overlay(
                            Circle()
                                .stroke(Theme.cream.opacity(isSelected ? 0.9 : 0), lineWidth: 1.5)
                                .padding(-4)
                        )
                        .shadow(color: color.opacity(isSelected ? 0.9 : 0), radius: 7)
                        .shadow(color: color.opacity(isSelected ? 0.5 : 0), radius: 14)
                        // Constant footprint: the ring and glow live outside the
                        // 14pt dot, so selecting never nudges the row's spacing.
                        .frame(width: 34, height: 30)
                        .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .accessibilityLabel(names[index])
                .accessibilityAddTraits(isSelected ? [.isSelected] : [])

                if index < Theme.chartSeriesColors.count - 1 { Spacer(minLength: 0) }
            }
        }
        .frame(maxWidth: .infinity)
        .padding(.horizontal, 4)
        .padding(.top, 2)
        .animation(.easeOut(duration: 0.18), value: viewModel.chartColorIndex)
    }
}

// MARK: - Chart
private struct ChartsImpl: View {
    @ObservedObject var viewModel: AtomizerViewModel
    var chartRefreshTrigger: Int // triggers chart redraw

    // Style-scale keys. Names, not indices, so the scale stays readable.
    private static let kindTemp = "Temperature"
    private static let kindTarget = "Target"

    // The live line's color, chosen by the swatch row below the plot.
    private var seriesColor: Color {
        Theme.seriesColor(viewModel.chartColorIndex)
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Live temperature is the hero, in whichever brand color is selected.
            // The target rides underneath in muted cream — deliberately NOT a
            // palette color, so it can never collide with the user's choice.
            // Two independent groupings, and they are NOT the same thing:
            //   series:            which points connect into one stroke (per segment)
            //   foregroundStyle(by:) which color bucket a mark belongs to
            // Color comes from the scale below rather than a per-mark
            // .foregroundStyle(Color). Setting the color directly on each mark
            // did not survive Charts' own style resolution once marks were split
            // across many series — the picker changed the value and the line kept
            // its old color. The scale is the supported way to drive series color
            // from a variable, and it re-renders when seriesColor changes.
            Chart {
                ForEach(setpointSegments) { seg in
                    ForEach(seg.points) { p in
                        LineMark(
                            x: .value("Time", p.time),
                            y: .value("Setpoint", p.value),
                            series: .value("Line", "target-\(seg.id.timeIntervalSince1970)")
                        )
                        .foregroundStyle(by: .value("Kind", Self.kindTarget))
                        .lineStyle(StrokeStyle(lineWidth: 2, lineCap: .round, lineJoin: .round))
                        .interpolationMethod(.linear)
                    }
                }
                // Temperature declared last so it draws over the target line.
                ForEach(tempSegments) { seg in
                    ForEach(seg.points) { p in
                        LineMark(
                            x: .value("Time", p.time),
                            y: .value("Temp", p.value),
                            series: .value("Line", "temp-\(seg.id.timeIntervalSince1970)")
                        )
                        .foregroundStyle(by: .value("Kind", Self.kindTemp))
                        .lineStyle(StrokeStyle(lineWidth: 3.5, lineCap: .round, lineJoin: .round))
                        .interpolationMethod(.linear)
                    }
                }
            }
            // The TARGET carries the swatch color, matching the setpoint slider
            // that sets it — one color for "what you asked for", everywhere it
            // appears. The measured temperature is neutral gray: it is the
            // reading, not a setting, and nothing the user picks should imply
            // otherwise.
            .chartForegroundStyleScale(
                domain: [Self.kindTemp, Self.kindTarget],
                range: [Theme.measured, seriesColor]
            )
            // foregroundStyle(by:) adds a legend by default; the colors are the
            // point here, and a legend would re-introduce the labels we removed.
            .chartLegend(.hidden)
            .chartXScale(domain: xDomain)
            // Charts does NOT reliably clip marks to the plot area when the scale
            // domain is narrower than the data — strokes near a boundary spill
            // past the axes. Explicit clipping is the fix; trimming the data to
            // the window is not enough on its own, because the leading segment
            // point legitimately sits outside the domain.
            .chartPlotStyle { plotArea in
                plotArea.clipped()
            }
            .chartXAxis {
                // Marks at FIXED offsets from the right edge, not .automatic().
                // Automatic Date marks pick their own stride from the data
                // extent, so every new sample re-picked the tick positions and
                // the labels twitched between values like "-7s" and "-22s"
                // several times a second. Pinning them to 0/-5/-10/-15 means the
                // labels are constant text and the line slides underneath them,
                // which is what a live chart should look like.
                AxisMarks(values: xAxisTicks) { value in
                    AxisGridLine().foregroundStyle(Theme.hairline)
                    // Collision resolution DISABLED: the Y axis sits on the
                    // trailing edge, so the "0" label at the right edge was
                    // judged to collide with it and silently dropped — the
                    // axis read -15/-10/-5 with nothing at the live end.
                    AxisValueLabel(collisionResolution: .disabled) {
                        if let d = value.as(Date.self) {
                            Text(relativeLabel(for: d))
                                .font(.caption2)
                                .foregroundColor(Theme.textDim)
                        }
                    }
                }
            }
            .chartYAxis {
                // At most 4 temperature labels, small type, on the trailing edge.
                AxisMarks(values: .automatic(desiredCount: 4)) {
                    AxisGridLine().foregroundStyle(Theme.hairline)
                    AxisValueLabel()
                        .foregroundStyle(Theme.textDim)
                        .font(.caption2)
                }
            }
            .chartYScale(domain: yDomain)
            .frame(height: 160)
            // Temperature only exists while the device is notifying. Without this,
            // a disconnected app draws just the cream setpoint line and looks like
            // a working chart whose color picker is broken — which is exactly how
            // it read. Say plainly that there is no temperature to draw.
            .overlay {
                if tempSegments.isEmpty {
                    Text(viewModel.isConnected ? "Waiting for temperature…" : "Not connected")
                        .font(.caption)
                        .foregroundColor(Theme.textDim)
                }
            }

            // Output strip under the main plot — rose, its own identity. NOT tied
            // to Theme.ok: status is now bright-vs-dim cream, which would wash the
            // strip out against the gray temperature line.
            // Not tied to the swatch: it shows duty cycle, a different quantity
            // from temperature. Fixed 0–100% scale: the strip's height IS the
            // duty cycle, honestly.
            Chart {
                ForEach(viewModel.history) { dp in
                    AreaMark(
                        x: .value("Time", dp.time),
                        y: .value("OutPct", dp.outputPct)
                    )
                    .foregroundStyle(
                        .linearGradient(
                            Gradient(colors: [Theme.rose.opacity(0.45), Theme.rose.opacity(0.05)]),
                            startPoint: .top, endPoint: .bottom
                        )
                    )
                    .interpolationMethod(.linear)
                }
            }
            .chartXAxis(.hidden)
            .chartYAxis(.hidden)
            // Same X window as the plot above. The strip's axis is hidden, so if
            // this drifts it fails silently: the strip would still look plausible
            // while showing a different span of time than the line it sits under.
            .chartXScale(domain: xDomain)
            .chartYScale(domain: 0...100)
            .frame(height: 32)
        }
    }

    // MARK: X axis (rolling window)

    // The right edge of the window: the newest sample, or now when there is no
    // data yet. Anchoring to the sample rather than Date() keeps the line's head
    // pinned to the edge — anchoring to wall-clock made the line detach and drift
    // leftward whenever samples arrived even slightly late.
    private var xAnchor: Date {
        viewModel.history.last?.time ?? Date()
    }

    // Fixed-width window, so a pixel is always worth the same amount of time.
    // Previously the domain came from the data, so the window grew with the
    // history and the whole plot crawled and stretched.
    private var xDomain: ClosedRange<Date> {
        // Half-second of trailing pad. A tick sitting exactly ON the domain's
        // upper bound is dropped by Charts, which is why the "0" label never
        // rendered while -5/-10/-15 all did. The pad moves it just inside.
        xAnchor.addingTimeInterval(-viewModel.chartWindowSeconds)...xAnchor.addingTimeInterval(0.5)
    }

    // 0 / -5 / -10 / -15, right to left.
    private var xAxisTicks: [Date] {
        stride(from: 0.0, through: viewModel.chartWindowSeconds, by: 5.0)
            .map { xAnchor.addingTimeInterval(-$0) }
    }

    // Seconds behind the newest sample: "0", "-5", "-10", "-15". Bare numbers —
    // on a 15-second window a unit suffix is noise.
    private func relativeLabel(for date: Date) -> String {
        let ago = Int(xAnchor.timeIntervalSince(date).rounded())
        return ago == 0 ? "0" : "-\(ago)"
    }

    // Unit-converted, gap-split series feeding the plot — trimmed to the window
    // so nothing is handed to Charts that belongs off the left edge.
    private var visibleHistory: [AtomizerViewModel.DataPoint] {
        windowed(viewModel.history, anchor: xAnchor, window: viewModel.chartWindowSeconds)
    }
    private var tempSegments: [ChartSegment] {
        segments(visibleHistory, gapThreshold: viewModel.historyGapThreshold) { dp in
            guard let t = dp.temp else { return nil }   // missing reading breaks the line
            return display(t)
        }
    }
    private var setpointSegments: [ChartSegment] {
        segments(visibleHistory, gapThreshold: viewModel.historyGapThreshold) { display($0.setpoint) }
    }
    private func display(_ celsius: Double) -> Double {
        viewModel.tempUnit == "C" ? celsius : (celsius * 9.0/5.0 + 32.0)
    }

    // FIXED full-scale axis — deliberately not data-driven.
    //
    // This used to snap to the data in 25° steps, which meant the axis silently
    // rescaled as the coil heated and cooled: the same 10° wobble looked like a
    // gentle ripple during a hot run and a violent spike while cold, and the
    // gridlines slid around underneath the line. A fixed scale costs some
    // vertical detail and buys a chart you can actually read at a glance —
    // height on screen always means the same temperature.
    //
    // 0–350 °C spans ambient through the firmware's 320 °C thermal-runaway
    // cutoff (THERMAL_RUNAWAY_TEMP_C, config.h), so a runaway stays in frame
    // instead of clipping off the top. Raise the ceiling if that limit rises.
    private var yDomain: ClosedRange<Double> {
        viewModel.tempUnit == "C" ? 0...350 : 32...660
    }
}

struct TemperatureChartView_Previews: PreviewProvider {
    static var previews: some View {
        // Create a lightweight VM with sample history (do not start CoreBluetooth)
        let vm = AtomizerViewModel(startCentral: false)
        // seed sample
        // 0.25s spacing to match the real sample cadence. At 1s apart every point
        // sits past the gap threshold, so the preview rendered as 40 disconnected
        // dots and looked broken for no reason.
        for i in 0..<40 {
            let t = 180.0 + Double(i) * 0.5
            let sp = 200.0
            let out = Double(i % 20) * 5.0
            vm.history.append(AtomizerViewModel.DataPoint(time: Date().addingTimeInterval(Double(i - 40) * 0.25), temp: t, setpoint: sp, outputPct: out))
        }
        return TemperatureChartView(viewModel: vm)
            .previewLayout(.sizeThatFits)
            .padding()
    }
}
