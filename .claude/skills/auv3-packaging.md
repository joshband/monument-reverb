# AUv3 Packaging & Deployment

You understand AUv3 packaging, entitlements, and App Store deployment.

## Architecture

```
MyPlugin.app (Host App - required for App Store)
└── PlugIns/
    └── MyPluginAU.appex (AUv3 Extension)
        ├── MyPluginAU (binary)
        ├── Info.plist
        └── MyPluginAU.entitlements
```

## Info.plist (Extension)

```xml
<key>NSExtension</key>
<dict>
    <key>NSExtensionAttributes</key>
    <dict>
        <key>AudioComponentBundle</key>
        <string>com.mycompany.myplugin.au</string>
        <key>AudioComponents</key>
        <array>
            <dict>
                <key>description</key>
                <string>My Plugin</string>
                <key>manufacturer</key>
                <string>MyC</string>  <!-- 4 char code -->
                <key>name</key>
                <string>MyCompany: My Plugin</string>
                <key>subtype</key>
                <string>MyPl</string>  <!-- 4 char code -->
                <key>type</key>
                <string>aufx</string>  <!-- aufx=effect, aumu=instrument, aumf=midi -->
                <key>version</key>
                <integer>1</integer>
                <key>sandboxSafe</key>
                <true/>
                <key>tags</key>
                <array>
                    <string>Effects</string>
                </array>
            </dict>
        </array>
    </dict>
    <key>NSExtensionPointIdentifier</key>
    <string>com.apple.AudioUnit-UI</string>
    <key>NSExtensionPrincipalClass</key>
    <string>MyPluginAU.AUv3ViewController</string>
</dict>
```

## Entitlements

### Extension Entitlements
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "...">
<plist version="1.0">
<dict>
    <key>com.apple.security.app-sandbox</key>
    <true/>
    <key>com.apple.security.application-groups</key>
    <array>
        <string>group.com.mycompany.myplugin</string>
    </array>
</dict>
</plist>
```

### Host App Entitlements (same group)
```xml
<key>com.apple.security.application-groups</key>
<array>
    <string>group.com.mycompany.myplugin</string>
</array>
```

## AudioComponentDescription

```objc
AudioComponentDescription desc = {
    .componentType = kAudioUnitType_Effect,        // 'aufx'
    .componentSubType = 'MyPl',                    // Your 4-char code
    .componentManufacturer = 'MyC ',               // Your 4-char code
    .componentFlags = 0,
    .componentFlagsMask = 0
};
```

| Type | Code | Use |
|------|------|-----|
| Effect | `aufx` | Audio effects |
| Instrument | `aumu` | Synths, samplers |
| MIDI Processor | `aumf` | MIDI effects |
| Generator | `augn` | Signal generators |

## App Group Storage

```swift
// Shared container for presets/settings
let containerURL = FileManager.default.containerURL(
    forSecurityApplicationGroupIdentifier: "group.com.mycompany.myplugin"
)

// Preset storage
let presetsURL = containerURL?.appendingPathComponent("Presets")
```

## App Store Review Pitfalls

1. **Startup time** - Must be fast, no heavy loading
2. **Memory** - Stay under limits, test on old devices
3. **Crash-free** - Test all host apps (GarageBand, AUM, Cubasis)
4. **Standalone function** - Host app must do something
5. **No private APIs** - Especially in DSP code
6. **Proper icons** - All sizes required

## Build Settings

```
// Extension target
INFOPLIST_KEY_NSExtensionPointIdentifier = com.apple.AudioUnit-UI
PRODUCT_BUNDLE_IDENTIFIER = com.mycompany.myplugin.au
CODE_SIGN_ENTITLEMENTS = MyPluginAU/MyPluginAU.entitlements
SKIP_INSTALL = YES
```

## Strong Defaults

- Avoid filesystem access from AUv3 (sandbox)
- Prefer in-memory or App Group storage
- No dynamic code loading
- No JIT compilation
- Presets: JSON in App Group, not Documents
