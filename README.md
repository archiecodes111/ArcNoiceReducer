# ARC NOISE REDUCER - Standalone Application & VST3 Plugin

> A professional mastering-grade noise reduction application and audio plugin built with JUCE 8 and modern C++20.
> Features precision Swiss-grade studio mastering aesthetics, high-contrast typography, and tactile physical hardware ergonomics.
> Implements STFT-based magnitude spectral subtraction with noise profile estimation, dual-mode visual mastering display, interactive waveform profiling, and 3-way auditioning.

---

## Overview

ARC NOISE REDUCER is designed to attenuate steady background noise (microphone hiss, ventilation, fan hum, electrical noise) from speech and music audio recordings using a learned noise profile and magnitude spectral subtraction.

The user interface features a high-end mastering hardware console layout:
- Warm brushed-aluminum faceplate with dark master utility bar and ARC DSP badge.
- ARC acoustic filtration arc monogram logo and bold cyan/slate brand typography.
- Recessed master display flanked by dual-column vertical mastering meters with ceiling marker bands.
- Tabbed master display: instant switching between `[ FREQUENCY ]` spectral curve analysis and `[ WAVEFORM ]` noise-profiling timeline.
- Precision matte black cylindrical knobs with amber indicator LEDs and drop-shadows.
- High-contrast, guaranteed-legible buttons with bold crisp white typography and distinct active states.

---

## Features & Capabilities

- Recessed Mastering Center Display:
  - Dual-Column Vertical Peak/RMS Meters: Flanking left (IN) and right (OUT) channels with red ceiling marker bands.
  - Frequency Spectrum View: 20 Hz - 20 kHz logarithmic graph showing raw input, noise profile, and clean denoised spectrum.
  - Waveform Profiling View: Full audio timeline with interactive draggable noise-selection handles and playhead scrubbing.
  - Tab Switcher: One-click toggling between `[ FREQUENCY ]` and `[ WAVEFORM ]`.
- One-Click Noise Profiling:
  - Learn Noise From Selection: Computes the average magnitude spectrum over the selected frames.
  - Load Noise File: Alternatively load a dedicated noise-only recording.
- Precision Mastering Hardware Knobs:
  - Subtraction (alpha): Rotary dial adjusting attenuation strength from 0.00 to 5.00 (default 1.50).
  - FFT Resolution: Switchable between 1024, 2048 (default), and 4096 points.
  - Output Trim: Output makeup gain adjustment (-12 dB to +12 dB).
  - Internal Spectral Floor (beta): Locked to the calibrated studio sweet spot (0.025 / -32 dB) to completely eliminate musical noise chirping artifacts with zero audible loss in signal transparency.
- 3-Way Audition Modes:
  - Denoised Output: Audition cleaned audio.
  - BYPASS: Instant comparison with the raw original audio via the green bypass rocker.
  - Residual: Audition the extracted noise (x[n] - y[n]) to verify that speech/music transients are preserved.
- Universal File I/O:
  - Loads WAV, MP3, AIFF, FLAC, and OGG files via juce::AudioFormatManager.
  - Exports cleaned audio to studio-quality 24-bit WAV files.

---

## Signal Processing Pipeline & Class Mapping

```
                                  +-------------------+
                                  | AudioFileManager  |
                                  +---------+---------+
                                            |
                                  +---------v---------+
                                  |  FrameProcessor   | (Analysis Windowing w_a, Hop H)
                                  +---------+---------+
                                            |
                                  +---------v---------+
                                  |   juce::dsp::FFT  |
                                  +----+---------+----+
                                       |         |
                  (Noise Segment)      |         | (Signal Frames)
              +------------------------+         +----------------------+
              |                                                         |
     +--------v--------+                                       +--------v--------+
     |  NoiseProfile   |  D_v(k) = (1/M) sum |V(m,k)|           |SpectralSubtractor|  G(m,k) = max(Dx - a*Dv, 0) / (Dx + eps)
     +--------+--------+                                       +--------+--------+
              |                                                         |
              +---------------------------------------------------------+
                                            |
                                  +---------v---------+
                                  | AudioReconstructor| (IFFT, Synthesis Window w_s, Overlap-Add & Normalise)
                                  +---------+---------+
                                            |
                                  +---------v---------+
                                  |   DenoiseEngine   | (3-Way Audition & WAV Export)
                                  +-------------------+
```

---

## How to Build & Run

### Prerequisites
- Windows 10/11
- Visual Studio 2022 / 2026 Build Tools (MSVC C++20)
- CMake 3.23+

### Building
Open PowerShell in the project directory:

```powershell
# 1. Configure CMake
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -B build -G "Visual Studio 18 2026"

# 2. Build Standalone Application
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target NoiseCleaner_Standalone

# 3. Build VST3 Plugin
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target NoiseCleaner_VST3
```

### Binaries Output
- Standalone App: build/NoiseCleaner_artefacts/Release/Standalone/Arc Noise Reducer.exe
- VST3 Plugin: build/NoiseCleaner_artefacts/Release/VST3/Arc Noise Reducer.vst3

---

## Documentation

A full academic technical report detailing the equations, derivation, windowing theory, parameters, and demonstration analysis is available in:
- [TECHNICAL_REPORT.md](TECHNICAL_REPORT.md)
