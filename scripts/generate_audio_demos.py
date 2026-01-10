#!/usr/bin/env python3
"""
Monument Reverb - Audio Demo Generator
Generates musical demonstrations for all presets with spectrograms and waveform analysis
"""

import numpy as np
import soundfile as sf
import matplotlib.pyplot as plt
from pathlib import Path
import json
from scipy import signal
from scipy.fft import fft, fftfreq
import argparse


class AudioDemoGenerator:
    """Generates audio demos and visualizations for Monument Reverb presets"""

    def __init__(self, sample_rate=48000):
        self.sample_rate = sample_rate
        self.output_dir = Path("test-results/audio-demos")
        self.output_dir.mkdir(exist_ok=True, parents=True)

    def generate_test_signals(self, duration=2.0):
        """Generate various test signals for reverb demonstration

        Returns dict of signal_name -> audio_data (mono)
        """
        num_samples = int(duration * self.sample_rate)
        t = np.linspace(0, duration, num_samples, endpoint=False)

        signals = {}

        # 1. Impulse (single click)
        signals['impulse'] = np.zeros(num_samples)
        signals['impulse'][0] = 1.0

        # 2. Drum hit (synthetic kick drum)
        freq_env = 200 * np.exp(-30 * t)
        amp_env = np.exp(-12 * t)
        signals['kick'] = np.sin(2 * np.pi * freq_env * t) * amp_env

        # 3. Snare hit (noise + tone)
        noise = np.random.randn(num_samples) * 0.3
        tone = np.sin(2 * np.pi * 220 * t)
        snare_env = np.exp(-18 * t)
        signals['snare'] = (noise + tone * 0.5) * snare_env

        # 4. Hi-hat (filtered noise burst)
        noise = np.random.randn(num_samples)
        hihat_env = np.exp(-35 * t)
        # High-pass filter
        sos = signal.butter(4, 4000, 'high', fs=self.sample_rate, output='sos')
        signals['hihat'] = signal.sosfilt(sos, noise) * hihat_env * 0.5

        # 5. Vocal "ah" (formant synthesis)
        f0 = 220  # A3
        harmonics = [1, 2, 3, 4, 5, 6, 7, 8]
        amps = [1.0, 0.5, 0.3, 0.2, 0.15, 0.1, 0.08, 0.05]
        vocal = np.zeros(num_samples)
        for h, amp in zip(harmonics, amps):
            vocal += amp * np.sin(2 * np.pi * f0 * h * t)
        vocal_env = np.ones_like(t)
        vocal_env[int(num_samples*0.8):] = np.linspace(1, 0, num_samples - int(num_samples*0.8))
        signals['vocal'] = vocal * vocal_env * 0.3

        # 6. Synth pad (detuned saws)
        pad = np.zeros(num_samples)
        freqs = [440, 441.5, 438.5]  # Slightly detuned
        for freq in freqs:
            # Sawtooth wave
            pad += signal.sawtooth(2 * np.pi * freq * t) / len(freqs)
        pad_env = np.minimum(t * 5, 1.0) * np.maximum(1.0 - (t - duration + 0.5) * 2, 0)
        signals['pad'] = pad * pad_env * 0.2

        # 7. Guitar pluck (Karplus-Strong)
        pluck = np.random.randn(num_samples) * 0.001
        pluck[:100] = np.random.randn(100) * 0.5
        # Simple comb filter
        delay_samples = int(self.sample_rate / 330)  # ~E note
        for i in range(delay_samples, num_samples):
            pluck[i] = pluck[i] + pluck[i - delay_samples] * 0.995
        signals['guitar'] = pluck * 0.8

        # Normalize all signals
        for name in signals:
            max_val = np.abs(signals[name]).max()
            if max_val > 0:
                signals[name] = signals[name] / max_val * 0.8

        return signals

    def convolve_with_ir(self, input_signal, ir):
        """Convolve input signal with impulse response

        Args:
            input_signal: Input signal (mono)
            ir: Impulse response (can be mono or stereo)

        Returns:
            Convolved signal (same format as IR)
        """
        if len(ir.shape) == 1:
            # Mono IR
            return signal.fftconvolve(input_signal, ir, mode='full')[:len(input_signal)]
        else:
            # Stereo IR - convolve each channel
            left = signal.fftconvolve(input_signal, ir[:, 0], mode='full')[:len(input_signal)]
            right = signal.fftconvolve(input_signal, ir[:, 1], mode='full')[:len(input_signal)]
            return np.stack([left, right], axis=-1)

    def generate_spectrogram(self, audio, title, output_path):
        """Generate and save spectrogram using STFT

        Args:
            audio: Audio signal (mono or stereo)
            title: Plot title
            output_path: Where to save the PNG
        """
        # If stereo, convert to mono for spectrogram
        if len(audio.shape) > 1:
            audio = audio.mean(axis=1)

        # STFT parameters (similar to JUCE FFT approach)
        nperseg = 4096  # FFT size
        noverlap = 3072  # 75% overlap

        f, t, Sxx = signal.spectrogram(
            audio,
            fs=self.sample_rate,
            nperseg=nperseg,
            noverlap=noverlap,
            scaling='spectrum'
        )

        # Convert to dB
        Sxx_db = 10 * np.log10(Sxx + 1e-10)

        # Create figure
        plt.figure(figsize=(12, 6))
        plt.pcolormesh(t, f, Sxx_db, shading='gouraud', cmap='inferno',
                       vmin=Sxx_db.max()-80, vmax=Sxx_db.max())
        plt.ylabel('Frequency (Hz)')
        plt.xlabel('Time (s)')
        plt.title(title)
        plt.colorbar(label='Magnitude (dB)')
        plt.ylim([20, 20000])
        plt.yscale('log')
        plt.tight_layout()
        plt.savefig(output_path, dpi=150)
        plt.close()

    def generate_decay_plot(self, audio, title, output_path, rt60_target=None):
        """Generate decay envelope plot

        Args:
            audio: Audio signal (mono or stereo)
            title: Plot title
            output_path: Where to save the PNG
            rt60_target: Optional RT60 value to overlay
        """
        # If stereo, convert to mono
        if len(audio.shape) > 1:
            audio = audio.mean(axis=1)

        # Calculate envelope using Hilbert transform
        analytic = signal.hilbert(audio)
        envelope = np.abs(analytic)

        # Smooth envelope
        window_size = int(self.sample_rate * 0.01)  # 10ms
        envelope_smooth = signal.convolve(envelope, np.ones(window_size)/window_size, mode='same')

        # Convert to dB
        envelope_db = 20 * np.log10(envelope_smooth + 1e-10)

        # Time axis
        t = np.arange(len(audio)) / self.sample_rate

        # Create figure
        plt.figure(figsize=(12, 6))
        plt.plot(t, envelope_db, linewidth=0.8, alpha=0.7, label='Decay envelope')

        # Add RT60 indicator if provided
        if rt60_target:
            plt.axhline(-60, color='red', linestyle='--', alpha=0.5, label='-60dB')
            plt.axvline(rt60_target, color='green', linestyle='--', alpha=0.5,
                       label=f'RT60 = {rt60_target:.2f}s')

        plt.xlabel('Time (s)')
        plt.ylabel('Amplitude (dB)')
        plt.title(title)
        plt.ylim([-80, 0])
        plt.xlim([0, t[-1]])
        plt.grid(alpha=0.3)
        plt.legend()
        plt.tight_layout()
        plt.savefig(output_path, dpi=150)
        plt.close()

    def process_preset(self, preset_idx, preset_name, test_signals):
        """Process all test signals through a preset and generate visualizations

        Args:
            preset_idx: Preset index (0-7)
            preset_name: Preset display name
            test_signals: Dict of signal_name -> audio_data

        Returns:
            Dict with paths to generated files
        """
        preset_dir = Path(f"test-results/preset-baseline/preset_{preset_idx:02d}")

        # Load impulse response
        ir_path = preset_dir / "wet.wav"
        if not ir_path.exists():
            print(f"Warning: No IR found for preset {preset_idx}, skipping")
            return None

        ir, sr = sf.read(ir_path)
        if sr != self.sample_rate:
            print(f"Warning: Sample rate mismatch for preset {preset_idx}")

        # Load RT60 metrics
        rt60 = None
        rt60_path = preset_dir / "rt60_metrics.json"
        if rt60_path.exists():
            with open(rt60_path) as f:
                metrics = json.load(f)
                rt60 = metrics.get('rt60_seconds', None)

        # Create output directory for this preset
        output_dir = self.output_dir / f"preset_{preset_idx:02d}_{preset_name.lower().replace(' ', '_')}"
        output_dir.mkdir(exist_ok=True)

        results = {
            'preset_idx': preset_idx,
            'preset_name': preset_name,
            'rt60': rt60,
            'demos': {}
        }

        # Process each test signal
        for signal_name, signal_data in test_signals.items():
            print(f"  Processing {signal_name}...")

            # Convolve with IR
            wet = self.convolve_with_ir(signal_data, ir)

            # Save audio
            audio_path = output_dir / f"{signal_name}_wet.wav"
            sf.write(audio_path, wet, self.sample_rate, subtype='PCM_24')

            # Generate spectrogram
            spec_path = output_dir / f"{signal_name}_spectrogram.png"
            self.generate_spectrogram(
                wet,
                f"{preset_name} - {signal_name.title()} - Spectrogram",
                spec_path
            )

            # Generate decay plot
            decay_path = output_dir / f"{signal_name}_decay.png"
            self.generate_decay_plot(
                wet,
                f"{preset_name} - {signal_name.title()} - Decay",
                decay_path,
                rt60_target=rt60
            )

            results['demos'][signal_name] = {
                'audio': str(audio_path.relative_to('test-results')),
                'spectrogram': str(spec_path.relative_to('test-results')),
                'decay': str(decay_path.relative_to('test-results'))
            }

        return results

    def generate_html_report(self, all_results):
        """Generate interactive HTML comparison report

        Args:
            all_results: List of result dicts from process_preset
        """
        html_path = self.output_dir / "index.html"

        html = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Monument Reverb - Audio Demos</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            margin: 0;
            padding: 20px;
            background: #1a1a1a;
            color: #e0e0e0;
        }
        .header {
            text-align: center;
            padding: 40px 0;
            border-bottom: 2px solid #333;
        }
        h1 {
            font-size: 2.5em;
            margin: 0;
            color: #fff;
        }
        .subtitle {
            color: #999;
            margin-top: 10px;
        }
        .preset {
            margin: 40px 0;
            padding: 30px;
            background: #252525;
            border-radius: 10px;
            border: 1px solid #333;
        }
        .preset-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            border-bottom: 1px solid #333;
            padding-bottom: 15px;
        }
        .preset-name {
            font-size: 1.8em;
            font-weight: bold;
            color: #4a9eff;
        }
        .rt60-badge {
            background: #2a4a2a;
            color: #8fce00;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: bold;
        }
        .demo-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(500px, 1fr));
            gap: 20px;
        }
        .demo-card {
            background: #2a2a2a;
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #3a3a3a;
        }
        .demo-title {
            font-size: 1.2em;
            margin-bottom: 15px;
            color: #ffa500;
            text-transform: capitalize;
        }
        audio {
            width: 100%;
            margin: 10px 0;
        }
        img {
            width: 100%;
            border-radius: 4px;
            margin: 10px 0;
        }
        .tabs {
            display: flex;
            gap: 10px;
            margin-bottom: 10px;
        }
        .tab {
            padding: 8px 16px;
            background: #333;
            border: none;
            color: #e0e0e0;
            cursor: pointer;
            border-radius: 4px;
            transition: background 0.2s;
        }
        .tab:hover {
            background: #444;
        }
        .tab.active {
            background: #4a9eff;
            color: white;
        }
        .tab-content {
            display: none;
        }
        .tab-content.active {
            display: block;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>🏛️ Monument Reverb - Audio Demos</h1>
        <p class="subtitle">Interactive preset demonstrations with spectrograms and decay analysis</p>
    </div>
"""

        # Add each preset
        for result in all_results:
            if not result:
                continue

            preset_name = result['preset_name']
            rt60 = result['rt60']
            rt60_text = f"RT60: {rt60:.2f}s" if rt60 else "RT60: N/A"

            html += f"""
    <div class="preset">
        <div class="preset-header">
            <div class="preset-name">{preset_name}</div>
            <div class="rt60-badge">{rt60_text}</div>
        </div>
        <div class="demo-grid">
"""

            # Add each demo signal
            for signal_name, paths in result['demos'].items():
                html += f"""
            <div class="demo-card">
                <div class="demo-title">{signal_name}</div>
                <audio controls>
                    <source src="../{paths['audio']}" type="audio/wav">
                </audio>
                <div class="tabs">
                    <button class="tab active" onclick="showTab('{preset_name}_{signal_name}', 'spec')">Spectrogram</button>
                    <button class="tab" onclick="showTab('{preset_name}_{signal_name}', 'decay')">Decay</button>
                </div>
                <div id="{preset_name}_{signal_name}_spec" class="tab-content active">
                    <img src="../{paths['spectrogram']}" alt="Spectrogram">
                </div>
                <div id="{preset_name}_{signal_name}_decay" class="tab-content">
                    <img src="../{paths['decay']}" alt="Decay">
                </div>
            </div>
"""

            html += """
        </div>
    </div>
"""

        html += """
    <script>
        function showTab(demoId, tabType) {
            // Hide all tabs for this demo
            document.querySelectorAll(`[id^="${demoId}"]`).forEach(el => {
                el.classList.remove('active');
            });

            // Show selected tab
            document.getElementById(`${demoId}_${tabType}`).classList.add('active');

            // Update button states
            const buttons = event.currentTarget.parentElement.querySelectorAll('.tab');
            buttons.forEach(btn => btn.classList.remove('active'));
            event.currentTarget.classList.add('active');
        }
    </script>
</body>
</html>
"""

        with open(html_path, 'w') as f:
            f.write(html)

        print(f"\n✅ HTML report generated: {html_path}")
        print(f"   Open in browser: file://{html_path.absolute()}")


def main():
    parser = argparse.ArgumentParser(description='Generate audio demos for Monument Reverb presets')
    parser.add_argument('--presets', type=str, default='0-7',
                       help='Preset range (e.g., "0-7" or "3,5,7")')
    parser.add_argument('--signals', type=str, default='all',
                       help='Comma-separated signal types or "all"')
    args = parser.parse_args()

    # Preset names (matches SequencePresets.cpp)
    PRESET_NAMES = [
        "Cathedral Dawn",       # 0
        "Infinite Chamber",     # 1
        "Twilight Hall",        # 2
        "Infinite Abyss",       # 3
        "Quantum Tunneling",    # 4
        "Time Dissolution",     # 5
        "Crystalline Void",     # 6
        "Hyperdimensional Fold" # 7
    ]

    # Parse preset range
    if '-' in args.presets:
        start, end = map(int, args.presets.split('-'))
        preset_indices = range(start, end + 1)
    else:
        preset_indices = [int(x.strip()) for x in args.presets.split(',')]

    print("🏛️ Monument Reverb - Audio Demo Generator")
    print("=" * 60)

    generator = AudioDemoGenerator()

    print("\n📊 Generating test signals...")
    test_signals = generator.generate_test_signals()

    # Filter signals if specified
    if args.signals != 'all':
        signal_names = [s.strip() for s in args.signals.split(',')]
        test_signals = {k: v for k, v in test_signals.items() if k in signal_names}

    print(f"   Generated {len(test_signals)} test signals: {', '.join(test_signals.keys())}")

    all_results = []

    print(f"\n🎵 Processing {len(preset_indices)} presets...")
    for idx in preset_indices:
        if idx >= len(PRESET_NAMES):
            print(f"Warning: Preset {idx} out of range, skipping")
            continue

        preset_name = PRESET_NAMES[idx]
        print(f"\nPreset {idx}: {preset_name}")
        result = generator.process_preset(idx, preset_name, test_signals)
        if result:
            all_results.append(result)

    print("\n📝 Generating HTML report...")
    generator.generate_html_report(all_results)

    print("\n✅ Complete!")
    print(f"   Generated {len(all_results)} preset demos")
    print(f"   Total files: {len(all_results) * len(test_signals) * 3} (audio + spectrograms + decay plots)")


if __name__ == '__main__':
    main()
