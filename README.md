# HAM - Hardware Accumulator Mode Sequencer

## 🎹 Overview

HAM is a modern MIDI sequencer inspired by the Intellijel Metropolix, built with JUCE and C++20. It features sample-accurate timing, unlimited tracks, and sophisticated gate/ratchet patterns with both mono and polyphonic voice modes.

## ✨ Key Features

- **Unlimited Tracks** with individual plugin hosting (VST3/AU)
- **8-Stage Sequencing** with 1-8 pulses per stage
- **Advanced Ratcheting** up to 8 subdivisions per pulse
- **Mono/Poly Voice Modes** with intelligent voice management
- **Accumulator Engine** for evolving pitch sequences
- **Async Pattern Engine** for live scene switching and pattern memory
- **Multi-Channel MIDI Routing** with flexible channel assignment
- **Pattern Morphing** with quantized recall and interpolation
- **Sample-Accurate Timing** with <0.1ms MIDI jitter
- **Automated CI/CD Testing** with screenshot and log validation
- **Domain-Driven Architecture** for maintainability and testing

## 🚀 Quick Start

### Prerequisites

- macOS (Apple Silicon or Intel)
- CMake >= 3.22
- Xcode Command Line Tools
- JUCE 8.0.8 (automatically downloaded)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/HAM.git
cd HAM

# Build the application
./build.sh

# The app will be copied to your Desktop
# Run it from: ~/Desktop/HAM.app
```

### Development Build

```bash
# Debug build with symbols
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8

# Run with debug output
./HAM.app/Contents/MacOS/HAM --debug
```

## 🏗️ Architecture

HAM follows Domain-Driven Design principles with clean separation between:

- **Domain Layer**: Pure business logic, no UI dependencies
- **Application Layer**: Use cases and command patterns
- **Infrastructure Layer**: Technical implementations (JUCE, MIDI, Audio)
- **Presentation Layer**: Swappable UI components

See [Docs/HAM_ARCHITECTURE.md](Docs/HAM_ARCHITECTURE.md) for detailed architecture documentation.

## 🎛️ Core Concepts

### Sequencer Hierarchy
```
Track → Stage → Pulse → Ratchet
  ↓       ↓       ↓        ↓
Plugin  8 steps  1-8    1-8 subdivisions
```

### Voice Modes
- **Mono**: New notes cut previous notes immediately
- **Poly**: Up to 16 overlapping voices per track with oldest-note priority

### Gate Types
- **MULTIPLE**: Individual gate per ratchet
- **HOLD**: Single sustained gate across pulse
- **SINGLE**: Gate on first ratchet only
- **REST**: No gate output

## 📁 Project Structure

```
HAM/
├── Source/
│   ├── Domain/         # Business logic
│   ├── Application/    # Use cases
│   ├── Infrastructure/ # Technical layer
│   └── Presentation/   # UI components
├── Resources/          # Assets, scales, presets
├── Docs/              # Documentation
├── Tests/             # Unit and integration tests
└── build.sh           # One-click build script
```

## 🔧 Configuration

### Audio Settings
Edit `~/.ham/config.json`:
```json
{
  "audio": {
    "sampleRate": 48000,
    "bufferSize": 128,
    "device": "Default"
  },
  "midi": {
    "clockPPQN": 24,
    "inputDevice": "All",
    "outputDevice": "Virtual HAM Port",
    "channelMode": "dynamic",
    "maxChannels": 16
  },
  "patterns": {
    "morphingEnabled": true,
    "quantizedRecall": true,
    "sceneMemorySlots": 64
  }
}
```

## 🐛 Debugging

### MIDI Monitor
Enable the built-in MIDI monitor for debugging:
```bash
cmake .. -DDEBUG_MIDI_MONITOR=ON
```
The monitor shows all MIDI events on channel 16 with timing analysis.

### Performance Profiling
```bash
# Run with performance counters
./HAM.app/Contents/MacOS/HAM --profile

# Use macOS Instruments
instruments -t "Time Profiler" HAM.app
```

## 📚 Documentation

- [Architecture Guide](Docs/HAM_ARCHITECTURE.md) - System design and patterns
- [MIDI Routing](Docs/MIDI_ROUTING.md) - Multi-channel signal flow and voice management
- [Performance Guide](Docs/PERFORMANCE.md) - Optimization and real-time constraints
- [UI Design](Docs/UI_DESIGN.md) - Component hierarchy and styling
- [Test Protocol](TEST_PROTOCOL.md) - Automated testing and CI/CD pipeline

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'feat: add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by [Intellijel Metropolix](https://intellijel.com/shop/eurorack/metropolix/)
- Built with [JUCE Framework](https://juce.com/)
- Developed using [Claude Code](https://claude.ai/code)

## 📧 Contact

Philip Krieger - [your.email@example.com](mailto:your.email@example.com)

---

**Current Version**: 0.1.0-alpha  
**Status**: Active Development  
**Last Updated**: December 2024