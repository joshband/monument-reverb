# AUv3 UI & Parameter Binding

You are an expert in AUv3 parameter binding and UI architecture.

## Core Rules

1. **UI NEVER touches DSP objects directly**
2. **All control flows through AUParameterTree**
3. **Parameter identifiers are stable and versioned**
4. **Use AUParameterObserverToken for UI updates**

## Parameter Tree Setup

```objc
- (AUParameterTree *)createParameterTree {
    AUParameter *gain = [AUParameterTree createParameterWithIdentifier:@"gain"
        name:@"Gain"
        address:0
        min:-60.0 max:12.0 unit:kAudioUnitParameterUnit_Decibels
        unitName:nil
        flags:kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable
        valueStrings:nil
        dependentParameters:nil];
    
    gain.value = 0.0;  // Default
    
    AUParameterTree *tree = [AUParameterTree createTreeWithChildren:@[gain]];
    
    // Implementor block for DSP access
    __weak typeof(self) weakSelf = self;
    tree.implementorValueProvider = ^(AUParameter *param) {
        return [weakSelf valueForParameterAddress:param.address];
    };
    tree.implementorValueObserver = ^(AUParameter *param, AUValue value) {
        [weakSelf setParameterAddress:param.address value:value];
    };
    
    return tree;
}
```

## UI Observer Pattern

```swift
class ParameterBridge: ObservableObject {
    @Published var gain: Float = 0.0
    
    private var parameterTree: AUParameterTree?
    private var observerToken: AUParameterObserverToken?
    
    func connect(to audioUnit: AUAudioUnit) {
        parameterTree = audioUnit.parameterTree
        
        // Register observer
        observerToken = parameterTree?.token(byAddingParameterObserver: { [weak self] address, value in
            // Called on arbitrary thread!
            DispatchQueue.main.async {
                self?.handleParameterChange(address: address, value: value)
            }
        })
        
        // Initial sync
        syncFromParameterTree()
    }
    
    func disconnect() {
        if let token = observerToken {
            parameterTree?.removeParameterObserver(token)
        }
        observerToken = nil
        parameterTree = nil
    }
    
    private func handleParameterChange(address: AUParameterAddress, value: AUValue) {
        switch address {
        case 0: gain = value
        default: break
        }
    }
    
    func setGain(_ value: Float) {
        parameterTree?.parameter(withAddress: 0)?.setValue(value, originator: observerToken)
    }
}
```

## SwiftUI Binding

```swift
struct GainKnob: View {
    @ObservedObject var bridge: ParameterBridge
    
    var body: some View {
        VStack {
            Slider(value: Binding(
                get: { bridge.gain },
                set: { bridge.setGain($0) }
            ), in: -60...12)
            
            Text("\(bridge.gain, specifier: "%.1f") dB")
        }
    }
}
```

## View Controller Setup

```swift
class AUv3ViewController: AUViewController {
    var audioUnit: AUAudioUnit? {
        didSet {
            if let au = audioUnit {
                bridge.connect(to: au)
            }
        }
    }
    
    private let bridge = ParameterBridge()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let hostingController = UIHostingController(rootView: 
            ContentView(bridge: bridge)
        )
        addChild(hostingController)
        view.addSubview(hostingController.view)
        hostingController.view.frame = view.bounds
        hostingController.view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        hostingController.didMove(toParent: self)
    }
    
    deinit {
        bridge.disconnect()
    }
}
```

## Anti-Patterns

```swift
// WRONG: Storing AUAudioUnit in SwiftUI View
struct BadView: View {
    let audioUnit: AUAudioUnit  // NO!
}

// WRONG: Using @State for parameter values
struct BadView: View {
    @State var gain: Float  // NO! Use @ObservedObject bridge
}

// WRONG: Polling parameters
Timer.scheduledTimer(...) {
    gain = parameterTree.value  // NO! Use observer
}

// WRONG: Direct DSP access
func setGain(_ value: Float) {
    audioUnit.dspProcessor.gain = value  // NO!
}
```

## Rules Summary

- UI is optional; AUv3 must function headless
- UI lifecycle is independent of audio lifecycle
- UI must tolerate parameter changes before it exists
- Use normalized [0,1] internally
- Never assume synchronous updates
