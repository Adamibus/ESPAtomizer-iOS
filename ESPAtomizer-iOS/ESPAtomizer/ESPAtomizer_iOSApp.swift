import SwiftUI

@main
struct ESPAtomizer_iOSApp: App {
    // The launch is a flat charcoal LaunchScreen.storyboard (see there for why),
    // so the OS app-open zoom scales an invisible flat color — no shake, no blurry
    // scaled logo. This in-app splash then plays over that same charcoal:
    //   1. orb + wordmark FADE IN (the storyboard had none, so nothing pops)
    //   2. hold
    //   3. logo dissolves into the charcoal cover
    //   4. the cover dissolves away, revealing the UI
    // Steps 3–4 are staged so the splash wordmark and the control screen's top
    // wordmark are never on screen together. Opacity is driven explicitly with
    // .animation(value:) because withAnimation in a .task after an await didn't
    // animate at all.
    @State private var logoOpacity: Double = 0.0   // fades IN from the flat launch
    @State private var coverOpacity: Double = 1.0

    var body: some Scene {
        WindowGroup {
            // ContentView owns the AtomizerViewModel via its own @StateObject.
            // The charcoal/cream palette is a dark design: lock dark mode so system
            // chrome (keyboard, dialogs, sliders) matches, and tint system controls
            // with the brand accent.
            ZStack {
                ContentView()
                // Opaque charcoal — matches the launch screen bg exactly, so the
                // hand-off is a flat-to-flat no-op. Hides the UI while the logo
                // fades in and out, then dissolves last to reveal it.
                Theme.bg
                    .ignoresSafeArea()
                    .opacity(coverOpacity)
                    .animation(.easeInOut(duration: 0.5), value: coverOpacity)
                    .allowsHitTesting(false)
                // Orb + wordmark, on top of the cover.
                SplashView()
                    .opacity(logoOpacity)
                    .animation(.easeInOut(duration: 0.45), value: logoOpacity)
                    .allowsHitTesting(false)
            }
            .preferredColorScheme(.dark)
            .tint(Theme.accent)
            .task {
                // Fade the logo in over the flat charcoal, hold, dissolve it into
                // the cover, then reveal the UI. The last sleep is just under the
                // logo fade so the cover clears as the logo finishes — no empty beat.
                logoOpacity = 1
                try? await Task.sleep(for: .seconds(1.5))
                logoOpacity = 0
                try? await Task.sleep(for: .seconds(0.4))
                coverOpacity = 0
            }
        }
    }
}

/// Pixel-exact mirror of LaunchScreen.storyboard so the OS hand-off from the
/// native launch screen to this view is seamless — any mismatch shows up as a
/// ghosted/blurred double logo during iOS's cross-dissolve. Positions use the
/// SAME math as the storyboard's constraints (full-screen center, NOT safe area,
/// because that's what the storyboard's centerY pins to):
///   orb   220×220, center = (W/2, H/2 − 70)
///   word  natural 202×36, top = orb.bottom + 8, centerX = W/2 + 4.3
/// Change one, change both — and re-check with a launch screenshot diff.
private struct SplashView: View {
    private let orbSize: CGFloat = 220
    private let orbCenterOffsetY: CGFloat = -70   // storyboard: centerY − 70
    private let wordGap: CGFloat = 8              // storyboard: word.top = orb.bottom + 8
    private let wordHeight: CGFloat = 36          // Wordmark.png natural height (109px @3x)
    private let wordCenterX: CGFloat = 4.3        // storyboard: centerX + 4.3

    var body: some View {
        GeometryReader { geo in
            let w = geo.size.width, h = geo.size.height
            let orbCenter = CGPoint(x: w / 2, y: h / 2 + orbCenterOffsetY)
            let wordCenterY = orbCenter.y + orbSize / 2 + wordGap + wordHeight / 2

            ZStack {
                Image("Logo")
                    .resizable()
                    .scaledToFit()
                    .frame(width: orbSize, height: orbSize)
                    .position(orbCenter)
                Image("Wordmark")            // natural size — matches storyboard 203×36
                    .overlay(alignment: .topTrailing) {
                        Text("™")
                            .font(.system(size: 13, weight: .medium))
                            .foregroundColor(Theme.text.opacity(0.65))
                            // Storyboard: leading = word.trailing − 2, top = +2.
                            .offset(x: 12, y: 2)
                    }
                    .position(x: w / 2 + wordCenterX, y: wordCenterY)
            }
        }
        // No background: the charcoal cover layer behind this provides it, so this
        // layer can fade to transparent without punching a hole to the UI below.
    }
}
