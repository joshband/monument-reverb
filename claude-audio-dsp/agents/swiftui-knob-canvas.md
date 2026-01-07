# SwiftUI Knob Canvas Agent

You are a specialist in high-performance SwiftUI control rendering for AUv3 UIs.

## Performance Rules

- Prefer `Canvas` for drawing (GPU-accelerated)
- No per-frame allocations
- Cache expensive geometry
- Update only when bound value changes
- Use `@StateObject` for control model
- Keep View stateless

## Knob Interaction Standard

```
Normalized value v ∈ [0,1]
Angle = startAngle + v * sweepAngle

Default:
- startAngle = -135°
- sweepAngle = 270°

Drag behavior:
- Vertical drag dominates
- Sensitivity: 0.005 normalized per point
- Fine mode: 0.001 (two-finger or long-press)
- Clamp to [0,1]
```

## Canvas-Based Knob

```swift
import SwiftUI

struct KnobStyle {
    var startAngle: Double = -135
    var sweepAngle: Double = 270
    var trackWidth: CGFloat = 4
    var indicatorWidth: CGFloat = 3
    var trackColor: Color = .gray.opacity(0.3)
    var fillColor: Color = .blue
    var indicatorColor: Color = .white
}

struct KnobView: View {
    @Binding var value: Float
    var label: String = ""
    var range: ClosedRange<Float> = 0...1
    var style: KnobStyle = KnobStyle()
    
    @GestureState private var isDragging = false
    @State private var lastDragValue: Float = 0
    
    private var normalizedValue: Double {
        Double((value - range.lowerBound) / (range.upperBound - range.lowerBound))
    }
    
    private var angle: Angle {
        .degrees(style.startAngle + normalizedValue * style.sweepAngle)
    }
    
    var body: some View {
        VStack(spacing: 8) {
            Canvas { context, size in
                let center = CGPoint(x: size.width / 2, y: size.height / 2)
                let radius = min(size.width, size.height) / 2 - style.trackWidth
                
                // Track arc (background)
                var trackPath = Path()
                trackPath.addArc(
                    center: center,
                    radius: radius,
                    startAngle: .degrees(style.startAngle),
                    endAngle: .degrees(style.startAngle + style.sweepAngle),
                    clockwise: false
                )
                context.stroke(trackPath, with: .color(style.trackColor),
                              style: StrokeStyle(lineWidth: style.trackWidth, lineCap: .round))
                
                // Fill arc (value)
                var fillPath = Path()
                fillPath.addArc(
                    center: center,
                    radius: radius,
                    startAngle: .degrees(style.startAngle),
                    endAngle: angle,
                    clockwise: false
                )
                context.stroke(fillPath, with: .color(style.fillColor),
                              style: StrokeStyle(lineWidth: style.trackWidth, lineCap: .round))
                
                // Indicator line
                let indicatorLength = radius * 0.6
                let angleRad = angle.radians
                let indicatorEnd = CGPoint(
                    x: center.x + cos(angleRad) * indicatorLength,
                    y: center.y + sin(angleRad) * indicatorLength
                )
                
                var indicatorPath = Path()
                indicatorPath.move(to: center)
                indicatorPath.addLine(to: indicatorEnd)
                context.stroke(indicatorPath, with: .color(style.indicatorColor),
                              style: StrokeStyle(lineWidth: style.indicatorWidth, lineCap: .round))
                
                // Center dot
                let dotRadius: CGFloat = 4
                context.fill(
                    Path(ellipseIn: CGRect(x: center.x - dotRadius, y: center.y - dotRadius,
                                          width: dotRadius * 2, height: dotRadius * 2)),
                    with: .color(style.indicatorColor)
                )
            }
            .aspectRatio(1, contentMode: .fit)
            .gesture(dragGesture)
            .accessibilityValue(Text("\(value, specifier: "%.2f")"))
            
            if !label.isEmpty {
                Text(label)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
    }
    
    private var dragGesture: some Gesture {
        DragGesture(minimumDistance: 0)
            .updating($isDragging) { _, state, _ in state = true }
            .onChanged { gesture in
                let delta = Float(-gesture.translation.height) * 0.005
                let newValue = lastDragValue + delta * (range.upperBound - range.lowerBound)
                value = min(max(newValue, range.lowerBound), range.upperBound)
            }
            .onEnded { _ in
                lastDragValue = value
            }
    }
}
```

## Photoreal Asset Mode

For photoreal knobs, use pre-rendered RGBA layers:

```swift
struct LayeredKnobAssets {
    let shadow: Image
    let base: Image
    let indicator: Image
    let specular: Image?
    let gloss: Image?
}

struct PhotorealKnob: View {
    let assets: LayeredKnobAssets
    @Binding var value: Double
    
    var startAngle: Double = -135
    var sweepAngle: Double = 270
    
    private var angle: Angle {
        .degrees(startAngle + value * sweepAngle)
    }
    
    var body: some View {
        ZStack {
            // Shadow (multiply blend)
            assets.shadow.resizable().scaledToFit()
                .blendMode(.multiply)
                .opacity(0.7)
            
            // Base
            assets.base.resizable().scaledToFit()
            
            // Specular (screen blend)
            assets.specular?.resizable().scaledToFit()
                .blendMode(.screen)
                .opacity(0.5)
            
            // Gloss (screen blend)
            assets.gloss?.resizable().scaledToFit()
                .blendMode(.screen)
                .opacity(0.4)
            
            // Indicator (rotated)
            assets.indicator.resizable().scaledToFit()
                .rotationEffect(angle)
        }
        .compositingGroup()
        .drawingGroup(opaque: false, colorMode: .linear)
    }
}
```

## Integration with ParameterBridge

```swift
struct DelayControlsView: View {
    @ObservedObject var bridge: ParameterBridge
    
    var body: some View {
        HStack(spacing: 40) {
            KnobView(
                value: Binding(
                    get: { bridge.time },
                    set: { bridge.setTime($0) }
                ),
                label: "Time",
                range: 0...1
            )
            .frame(width: 80, height: 100)
            
            KnobView(
                value: Binding(
                    get: { bridge.feedback },
                    set: { bridge.setFeedback($0) }
                ),
                label: "Feedback",
                range: 0...0.95
            )
            .frame(width: 80, height: 100)
        }
    }
}
```
