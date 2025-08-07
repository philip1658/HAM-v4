# CLAUDE.md – HAM Sequencer Development Guide

## 🎯 Projekt-Übersicht

**HAM** (Hardware Accumulator Mode) ist ein MIDI-Sequencer inspiriert von Intellijel Metropolix.
- **Projektpfad**: `/Users/philipkrieger/Desktop/HAM/`
- **Entwickler**: Philip Krieger
- **Platform**: macOS (Apple Silicon M3 Max, 128GB RAM)
- **Framework**: JUCE 8.0.8 mit C++20
- **Build-System**: CMake (>= 3.22)

---

## 👤 Entwickler-Profil

- **Erfahrung**: C++ Einsteiger, fokussiert auf JUCE & Audio/MIDI
- **Arbeitsweise**: Terminal-basiert, Claude CLI als Hauptwerkzeug
- **Editor**: CMake + Terminal
- **Präferenz**: Technische Tiefe > Small Talk, direkte Antworten

### Claude-Verhalten
- ✅ Erkläre implementierte Änderungen in einfacher Sprache
- ✅ Lies bei jedem start /Users/philipkrieger/Desktop/HAM/ROADMAP.md und halte das Dokument aktuell 
- ✅ Wenn du ein neues File zbsp .cpp erstellst, dann lösche das zu ersetzende File aus dem Projekt und aus dem Code und aus cmake files. Keine ausklammerungen. Keine vorübergehende Lösungen. Es muss alles sehr übersichtlich bleiben. Keine Quickfixes. Geh den Ursachen auf den Grund auch wenn es länger geht. Überspringe keine Fehler wenn du sie siehst. 
- ✅ Zeige konkreten Code (kein Pseudocode)
- ✅ Frage präzise nach bei Unklarheiten
- ❌ Keine unnötigen Einleitungen ("Gerne!", "Klar!")
- ❌ Keine Metaphern oder Chat-Floskeln

---

## 🏗️ Projekt-Architektur (Domain-Driven Design)

```
/Users/philipkrieger/Desktop/HAM/
├── CMakeLists.txt
├── build.sh                        # One-click Desktop Build
├── UI_DESIGN.md                    # Komplette UI Design Dokumentation
├── Source/
│   ├── Domain/                     # Business Logic (UI-unabhängig)
│   │   ├── Models/
│   │   │   ├── Track.h             # Track mit Mono/Poly Voice Mode
│   │   │   ├── Stage.h             # 8 Stages pro Track
│   │   │   ├── Pattern.h           # 128 Pattern Slots
│   │   │   ├── Scale.h             # Skalen-Definitionen
│   │   │   ├── Snapshot.h          # Pattern Snapshots für Morphing
│   │   │   └── Project.h           # Gesamtes Projekt
│   │   ├── Engines/
│   │   │   ├── VoiceManager.h      # KRITISCH: Poly/Mono Handling
│   │   │   ├── MasterClock.h       # 24 PPQN, Sample-accurate
│   │   │   ├── SequencerEngine.h   # Lightweight Coordinator (refactored!)
│   │   │   ├── AccumulatorEngine.h # Pitch-Transposition (inkl. PENDULUM)
│   │   │   ├── GateEngine.h        # Gate-Typen & Ratchets
│   │   │   ├── PitchEngine.h       # Pitch-Quantisierung
│   │   │   └── AsyncPatternEngine.h # Scene-Switching & Pattern Memory
│   │   ├── Processors/             # NEU: Extrahiert aus SequencerEngine
│   │   │   ├── TrackProcessor.h    # Track-Logik, Stage-Advancement, Directions
│   │   │   ├── MidiEventGenerator.h # MIDI-Event Erstellung, Ratchets
│   │   │   └── PatternScheduler.h  # Pattern-Übergänge, Chaining, Morphing
│   │   └── Services/
│   │       ├── MidiRouter.h        # Multi-Channel MIDI Routing
│   │       ├── PluginHost.h        # VST3/AU Hosting
│   │       ├── PresetManager.h     # JSON + Binary Save/Load
│   │       └── ChannelManager.h    # Dynamic MIDI Channel Assignment
│   │
│   ├── Application/                # Use Cases
│   │   ├── Commands/               # Undo/Redo Pattern
│   │   └── Services/               # Business Rules
│   │
│   ├── Infrastructure/             # Technische Implementation
│   │   ├── Audio/
│   │   │   └── AudioEngine.cpp     # JUCE AudioProcessor
│   │   ├── Persistence/
│   │   │   ├── JsonSerializer.cpp  # Pattern & Settings
│   │   │   └── BinarySerializer.cpp # Plugin States
│   │   └── Debug/
│   │       └── MidiMonitor.cpp     # Removable Debug Tool
│   │
│   └── Presentation/               # UI Layer (austauschbar)
│       ├── Components/
│       │   ├── HAMComponentLibrary.h # Base Resizable Component Library
│       │   ├── HAMComponentLibraryExtended.h # Extended Components (30+)
│       │   ├── TrackSidebar/       # Track Controls
│       │   ├── StageGrid/          # 8 Stage Cards
│       │   ├── HAMEditor/          # Erweiterter Stage Editor
│       │   └── Transport/          # Play/Stop/BPM
│       └── ViewModels/             # UI State Management
```

---

## 🎹 Core Features & Terminologie

### Sequencer-Hierarchie
1. **Track** → Unbegrenzte Anzahl, jeder mit eigenem Instrument
2. **Stage** → 8 Stages pro Track (Steps im Pattern)
3. **Pulse** → 1-8 Pulses pro Stage (Zeiteinheiten)
4. **Ratchet** → 1-8 Ratchets pro Pulse (Subdivisions)

### Voice Modes (KRITISCH - von Anfang an implementieren!)
- **Mono Mode**: Neue Note beendet vorherige sofort
- **Poly Mode**: Noten können überlappen (max 16 Voices pro Track)
- **Voice Stealing**: Oldest-Note-Priority bei Polyphonie-Limit

### HAM Editor (ehemals "HAM Button")
Erweiterter Stage-Editor für:
- Gate-Modulation pro Stage
- Ratchet-Patterns und Probability
- Pitchwheel-Automation
- CC-Mappings zu Plugins/Hardware
- Per-Stage Modulation Routing

### Gate-Typen
- **MULTIPLE**: Ein Gate pro Ratchet
- **HOLD**: Ein durchgehender Gate über gesamten Pulse
- **SINGLE**: Nur beim ersten Ratchet
- **REST**: Kein Gate (Pause)

### Accumulator Engine  
- Transponiert Pitch in Skalenstufen
- Modi: Per Stage / Per Pulse / Per Ratchet / **PENDULUM** (NEU!)
- Reset: Manual / Stage Loop / Pattern Change / Value Limit
- Wrap Mode für zyklische Akkumulation

---

## 🔧 Technische Spezifikationen

### Performance Requirements
- **CPU**: < 5% auf M3 Max mit geladenem Plugin
- **Latency**: < 0.1ms MIDI Jitter
- **Sample Rate**: 48kHz Standard, 44.1-192kHz Support
- **Buffer Size**: 128 Samples (Testing), 64-512 (Production)

### Real-Time Audio Constraints
```cpp
// IMMER im Audio Thread:
- Keine new/delete oder malloc
- Keine std::mutex oder Locks
- Keine String-Operationen
- Keine File I/O
- Verwende std::atomic für Parameter
- Pre-allocated Buffers
- Lock-free Message Queue für UI↔Engine
```

### JUCE Best Practices
```cpp
// Verwende JUCE-eigene Klassen:
juce::MidiMessage       // statt custom MIDI
juce::AudioBuffer       // für Audio-Processing
juce::Range            // für Wertebereiche
jassert()              // statt assert()
DBG()                  // statt std::cout
juce::ValueTree        // für State Management
```

---

## 🛠️ Build & Development Workflow

### Build Commands
```bash
# Einmaliges Setup
cd /Users/philipkrieger/Desktop/HAM
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Daily Build (immer auf Desktop)
./build.sh  # Baut und kopiert HAM.app auf Desktop

# Debug Build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8
```

### MIDI Monitor (Debug Feature)
```cpp
#ifdef DEBUG_MIDI_MONITOR
    // Channel 16 für Debug (Plugins nutzen nur 1-15)
    // Zeigt: Expected (UI) vs Actual (Engine)
    // Highlight: Timing-Differenzen > 0.1ms
#endif
// Einfach entfernbar via CMake Flag
```

### Testing Workflow
1. Build mit `./build.sh`
2. App liegt auf Desktop: `/Users/philipkrieger/Desktop/HAM.app`
3. MIDI Monitor.app parallel für External MIDI
4. Instruments.app für Performance Profiling

---

## 💾 Datenformate

### Projekt-Struktur
```
HAM_Project.ham/                   # Bundle Format
├── project.json                   # Metadaten, Routing, UI-State
├── patterns/
│   ├── pattern_001.json          # Lesbar für Debugging
│   └── pattern_002.json
├── plugin_states/                 # Binary für VST/AU
│   ├── track_1.bin
│   └── track_2.bin
└── scales/                        # Custom Scales
    └── user_scales.json
```

### Pattern Format (JSON)
```json
{
  "version": "1.0.0",
  "bpm": 120,
  "tracks": [{
    "id": 1,
    "voiceMode": "poly",
    "division": 4,
    "stages": [{
      "pitch": 60,
      "gate": 0.8,
      "velocity": 100,
      "pulseCount": 4,
      "ratchets": [1,1,2,1]
    }]
  }]
}
```

---

## 📝 Code Standards

### Naming Conventions
```cpp
// Klassen: PascalCase
class SequencerEngine {};

// Member-Variablen: m_ prefix
float m_currentBpm;

// Static: s_ prefix  
static int s_instanceCount;

// Funktionen: camelCase
void processNextStage();

// Konstanten: UPPER_SNAKE_CASE
const int MAX_TRACKS = 128;
```

### Kommentar-Style
```cpp
// Kurze Erklärung in verständlicher Sprache
// WAS macht dieser Code (nicht WIE)

/** @brief Berechnet nächste Stage basierend auf Clock
 *  @details Berücksichtigt Swing, Division und Skip-Conditions
 *  @param clockPosition Aktuelle Position in PPQN
 *  @return Stage-Index oder -1 wenn Skip
 */
int calculateNextStage(int clockPosition);
```

---

## 🚫 Anti-Patterns & Vermeidungen

### Was Claude NICHT tun soll
- ❌ std::cout im Release (→ verwende DBG())
- ❌ Relative #include Pfade (→ absolute vom Source-Root)
- ❌ Magic Numbers (→ benannte Konstanten)
- ❌ try/catch im Audio Thread (→ Pre-Validation)
- ❌ Dynamische Allocation im Realtime-Pfad
- ❌ Blockierende Operationen in processBlock()

---

## 📊 Aktueller Projekt-Status

### ✅ Bereits implementiert
- [x] Architektur-Design (Domain-Driven)
- [x] Core Feature-Set & Performance-Ziele
- [x] CMakeLists.txt Setup mit JUCE 8.0.4
- [x] Domain Models (Track, Stage, Pattern, Scale)
- [x] MasterClock mit 24 PPQN
- [x] **SequencerEngine refactored** - delegiert an 3 Prozessoren:
  - TrackProcessor: Track-States, Stage-Advancement, Directions
  - MidiEventGenerator: MIDI-Events, Ratchets, Velocity
  - PatternScheduler: Pattern-Übergänge, Chaining, Morphing
- [x] AccumulatorEngine mit **PENDULUM Mode** (Ping-Pong Akkumulation)
- [x] PitchEngine mit Scale-Quantisierung
- [x] GateEngine für Gate-Typen
- [x] VoiceManager für Mono/Poly (64 voices)
- [x] **100% Test Coverage** für alle Engines
- [x] UI Design System dokumentiert
- [x] Component Library (30+ Komponenten)

### 🚧 Nächste Schritte
- [ ] Lock-free Message Queue für UI↔Engine
- [ ] Basic UI mit Stage Cards implementieren
- [ ] AsyncPatternEngine Integration

### 📅 Spätere Features
- [ ] Plugin Hosting (VST3/AU)
- [ ] HAM Editor UI
- [ ] Extended Snapshot System mit Morphing
- [ ] Automated Testing Pipeline (CI/CD)
- [ ] Pattern Chaining mit Scene Manager
- [ ] MPE Support

---

## 🔍 Debug & Profiling

### Performance Measuring
```cpp
// In kritischen Sections
juce::ScopedHighResolutionTimer timer("processBlock");

// Oder mit PerformanceCounter
juce::PerformanceCounter counter("MIDI Events", 1000);
counter.start();
// ... code ...
counter.stop();
```

### Common Issues & Solutions
- **MIDI Jitter** → Check Thread Priority & Buffer Alignment
- **CPU Spikes** → Profile mit Instruments.app Time Profiler
- **Crashes** → `lldb HAM.app` dann `bt all` für Stack Trace
- **Memory Leaks** → Instruments.app Leaks Tool

---

## 🎯 Quick Reference

### Häufige Terminal Commands
```bash
# Build & Run
./build.sh && /Users/philipkrieger/Desktop/HAM.app/Contents/MacOS/HAM

# Git Workflow
git add -A && git commit -m "feat: implement VoiceManager"
git push origin main

# Clean Build
rm -rf build && mkdir build && cd build
cmake .. && make -j8

# Find MIDI Issues
grep -r "MidiMessage" Source/ --include="*.cpp"
```

### JUCE Module Dependencies
```cmake
juce_add_module(juce_audio_basics)
juce_add_module(juce_audio_devices)
juce_add_module(juce_audio_processors)
juce_add_module(juce_core)
juce_add_module(juce_data_structures)
juce_add_module(juce_dsp)
juce_add_module(juce_events)
juce_add_module(juce_graphics)
juce_add_module(juce_gui_basics)
juce_add_module(juce_gui_extra)
```

---

## 🎨 UI Component Library & Designer Tool

HAM verwendet eine **resizable Component Library** mit exakter Pulse-Design-Ästhetik:

### Komponenten-Übersicht
- **Pulse Component Library**: `Source/UI/PulseComponentLibrary.h/cpp` - Exakte Pulse-Komponenten
- **UI Designer Tool**: `/Tools/UIDesigner/` - Separates Entwicklungstool
- **Desktop App**: `HAM UI Designer.app` - Zum Testen und Designen
- **Namespace**: `HAM::PulseComponentLibrary`
- **Base Class**: `PulseComponent` - Alle UI-Komponenten mit Animationen

### Verfügbare Komponenten (30+ exakte Pulse-Kopien)

#### Basis-Komponenten
- **PulseVerticalSlider** - 22px Breite, Line-Indicator (kein Thumb)
- **PulseHorizontalSlider** - Mit optionalem Thumb
- **PulseButton** - 4 Styles: Solid, Outline, Ghost, Gradient
- **PulseToggle** - iOS-Style animierter Switch
- **PulseDropdown** - 3-Layer Shadow mit Gradient
- **PulsePanel** - 5 Styles: Flat, Raised, Recessed, Glass, TrackControl

#### Spezial-Komponenten
- **StageCard** - 140x420px mit 2x2 Slider-Grid (PITCH/PULSE/VEL/GATE)
- **ScaleSlotSelector** - 8 Scale Slots mit Hover-Effekten
- **GatePatternEditor** - 8-Step Pattern Editor mit Drag
- **PitchTrajectoryVisualizer** - Spring-animierte Pitch-Visualisierung
- **TrackControlPanel** - Track-Steuerung mit Gradient-Background

#### Transport Controls (Extended)
- **PlayButton** - Play/Pause mit Puls-Animation
- **StopButton** - Stop mit Shadow-Effekt
- **RecordButton** - Blink-Animation beim Recording
- **TempoDisplay** - BPM mit Tap Tempo und Arrows

#### Pattern/Sequencer Components (Extended)
- **GatePatternEditor** - 8-Step Gate Pattern Editor
- **RatchetPatternDisplay** - Ratchet Visualization
- **VelocityCurveEditor** - Velocity Curve Drawing

#### Scale/Music Components (Extended)
- **ScaleSlotPanel** - 8 Scale Slot Selector
- **ScaleKeyboard** - 2-Octave Keyboard mit Scale Highlighting

#### Data Input/Display (Extended)
- **NumericInput** - Editable Number Field
- **SegmentedControl** - Multi-Segment Toggle
- **LevelMeter** - Horizontal/Vertical Level Display

#### Visualization (Extended)
- **AccumulatorVisualizer** - Trajectory Display
- **XYPad** - 2D Parameter Control

#### Additional UI Elements (Extended)
- **TabBar** - Tab Navigation
- **LED** - Animated LED Indicator
- **ToastNotification** - Temporary Messages
- **SearchBox** - Search Input Field
- **ProgressBar** - Progress mit Animation

### Design System
- **8px Grid**: Alle Komponenten nutzen relatives Spacing basierend auf 8px
- **Track Colors**: 8 vordefinierte Neon-Farben (Mint, Cyan, Magenta, Orange, etc.)
- **Multi-Layer Shadows**: Realistische Tiefe durch geschichtete Schatten
- **Responsive Ratios**: Border width, corner radius, thumb size als Prozent der Größe

### UI Designer Tool
- **Separates Entwicklungstool**: Komplett getrennt von Hauptapp
- **Scrollbare Galerie**: Alle Komponenten organisiert in Sektionen
- **Grid-System**: 24x24 Grid (A-X, 1-24) für präzise Platzierung
- **Build & Run**:
```bash
cd /Users/philipkrieger/Desktop/HAM/Tools/UIDesigner
mkdir build && cd build
cmake .. && make
# App liegt auf Desktop als "HAM UI Designer.app"
```

### Verwendung der Komponenten
```cpp
// Verwende Pulse-Komponenten
auto slider = std::make_unique<HAM::PulseComponentLibrary::PulseVerticalSlider>(
    "PITCH", 0); // Name und Farbindex
slider->setValue(0.5f);
addAndMakeVisible(slider.get());

// Stage Card mit 2x2 Grid
auto stage = std::make_unique<HAM::PulseComponentLibrary::StageCard>(
    "STAGE_1", 1);
stage->setBounds(100, 100, 140, 420);
```

### UI Design Prinzipien (Exakte Pulse-Replikation)
- **Vertical Sliders ohne Thumb**: Line-Indicator statt Handle, 22px breite Tracks
- **Dark Void Aesthetic**: Dunkle Hintergründe (#000000 bis #3A3A3A) von Pulse
- **Subtle Accents**: Farben nur als kleine Akzente, nicht als Hauptelemente
- **2x2 Slider Grid**: Stage Cards mit PITCH/PULSE oben, VEL/GATE unten
- **Gradient Fills**: Sophisticated shading (0.9 zu 0.7 alpha)
- **3px Corner Radius**: Rechteckiger Look statt vollständig rund

### Stage Card Layout (NEU)
- **Größe**: 140x420px (doppelte Höhe für 2x2 Grid)
- **Slider Anordnung**:
  - Oben links: PITCH
  - Oben rechts: PULSE
  - Unten links: VEL (Velocity)
  - Unten rechts: GATE
- **Alle Slider gleich groß** im Grid-Layout
- **Buttons zentriert** am unteren Rand

---

## 📚 Weitere Dokumentation

- `HAM_ARCHITECTURE.md` - Detaillierte System-Architektur
- `MIDI_ROUTING.md` - MIDI Signal Flow Dokumentation
- `PERFORMANCE.md` - Optimization Guidelines
- `UI_DESIGN.md` - Komplettes UI Design System mit Component Specs
- `Source/UI/PulseComponentLibrary.h/cpp` - Exakte Pulse Component Library (30+ Komponenten)
- `Tools/UIDesigner/` - Separates UI Designer Tool für Entwicklung 

---

*Letzte Aktualisierung: 2025-08-07*

### Wichtige Änderungen:
- **UI-First Approach**: UI wird zuerst vollständig aufgebaut, dann mit Funktionen bestückt
- **Pulse Component Library**: Exakte Kopie aller Pulse-Elemente implementiert
- **UI Designer Tool**: Separates Entwicklungstool für UI-Design und Testing
