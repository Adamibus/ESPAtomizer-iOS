// Renders the ADAMIZER wordmark (Futura-Medium, kern 7pt) as transparent PNGs
// at 1x/2x/3x for the launch screen imageset. Cream #F5F0E6 to match Theme.text.
import AppKit

let word = "ADAMIZER"
let cream = NSColor(srgbRed: 0.96, green: 0.94, blue: 0.90, alpha: 1.0)

func render(scale: CGFloat, out: String) {
    let size: CGFloat = 28 * scale
    let kern: CGFloat = 7 * scale
    guard let font = NSFont(name: "Futura-Medium", size: size) else {
        fputs("MISSING FONT Futura-Medium\n", stderr); exit(1)
    }
    let attrs: [NSAttributedString.Key: Any] = [
        .font: font, .kern: kern, .foregroundColor: cream
    ]
    let str = NSAttributedString(string: word, attributes: attrs)
    let bounds = str.size()
    let w = ceil(bounds.width), h = ceil(bounds.height)
    let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: Int(w), pixelsHigh: Int(h),
                               bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                               colorSpaceName: .calibratedRGB, bytesPerRow: 0, bitsPerPixel: 0)!
    let ctx = NSGraphicsContext(bitmapImageRep: rep)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = ctx
    str.draw(at: .zero)
    NSGraphicsContext.restoreGraphicsState()
    try! rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: out))
    print("scale \(scale): \(w) x \(h) px  ->  \(w/scale) x \(h/scale) pt")
}

let dir = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "."
render(scale: 1, out: "\(dir)/wordmark.png")
render(scale: 2, out: "\(dir)/wordmark@2x.png")
render(scale: 3, out: "\(dir)/wordmark@3x.png")
