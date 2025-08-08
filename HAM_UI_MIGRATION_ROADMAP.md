# HAM UI Migration Roadmap

## Project Overview
**Goal**: Migrate HAM sequencer from basic placeholder UI to full Pulse-style component library
**Status**: Migration Starting
**Last Updated**: 2025-08-08
**Developer**: Philip Krieger via Claude CLI

## Migration Principles
- Clean integration only - no quick fixes or temporary solutions
- Delete old files completely when replacing
- Maintain separation between UI and audio engine
- Use lock-free message queues for UI↔Engine communication
- Follow 8px grid system and Pulse dark void aesthetic
- All components must be ResizableComponent subclasses

---

## Phase 1: Foundation Setup ✅
**Status**: Complete
**Completed**: 2025-08-08

### ✅ 1.1 Component Libraries Created
- [x] HAMComponentLibrary.h - Base resizable components
- [x] HAMComponentLibraryExtended.h - 30+ extended components
- [x] PulseLookAndFeel.h - Exact Pulse colors and styling
- [x] UI Designer Tool - Separate app for testing components

### ✅ 1.2 Core UI Structure
- [x] Presentation layer folder structure
- [x] ViewModels for data binding
- [x] BaseComponent abstractions
- [x] DesignSystem tokens

---

## Phase 2: Message Queue Infrastructure ✅
**Status**: Complete with JUCE 8 fixes needed
**Completed**: 2025-08-08
**Target**: Implement lock-free UI↔Engine communication

### ✅ 2.1 Lock-Free Message Queue
- [x] Implement LockFreeMessageQueue.h using juce::AbstractFifo - NEEDS JUCE 8 FIX
- [x] Create MessageTypes.h with all UI→Engine messages - COMPLETE
- [x] Create MessageDispatcher for routing - COMPLETE
- [x] Add response queue for Engine→UI updates - COMPLETE
- [ ] Fix AbstractFifo API changes for JUCE 8 - BLOCKED

### 2.2 Parameter System (Deferred)
- [ ] Create ParameterManager for centralized state - Can use MessageDispatcher
- [ ] Implement atomic parameter updates - Already in message system
- [ ] Add parameter change listeners - Part of dispatcher
- [ ] Create undo/redo command pattern - Future enhancement
- [ ] Test thread safety with stress tests - Built into queue

---

## Phase 3: Transport Controls 🟡
**Status**: In Progress
**Target**: Replace placeholder buttons with Pulse components

### 3.1 PlayButton Integration
- [x] Replace TextButton with HAM::UI::PlayButton
- [x] Connect to Transport via message queue
- [x] Add pulse animation when playing
- [ ] Implement space bar shortcut
- [ ] Test MIDI clock start/stop

### 3.2 StopButton Integration
- [x] Add HAM::UI::StopButton
- [x] Connect to Transport stop
- [x] Add shadow effect on press
- [ ] Test immediate vs quantized stop

### 3.3 TempoDisplay Integration
- [x] Replace Slider with HAM::UI::TempoDisplay
- [x] Add tap tempo functionality
- [x] Connect BPM changes to MasterClock
- [ ] Add arrow key adjustments
- [ ] Test tempo glide smoothing

### 3.4 Transport Bar Layout
- [x] Create TransportBar container component
- [x] Use proper layout management
- [x] Add record button
- [x] Style with Panel background
- [ ] Test responsive resizing

---

## Phase 4: Stage Grid Implementation 🟡
**Status**: In Progress
**Target**: Replace placeholder with 8 StageCard components

### 4.1 StageCard Component
- [x] Use HAM::UI::StageCard from library
- [x] Setup 2x2 slider grid (PITCH/PULSE/VEL/GATE)
- [x] All sliders 22px wide tracks, no thumbs
- [x] Setup callbacks for parameter changes
- [ ] Test parameter updates with engine

### 4.2 StageGrid Container
- [x] Create 8-card horizontal layout
- [x] Add selection management
- [ ] Implement keyboard navigation (arrows)
- [ ] Add multi-select with Cmd key
- [ ] Test with different window sizes

### 4.3 Stage-Engine Connection
- [x] Route parameter changes through message queue
- [x] Add message handlers for stage parameters
- [x] Add active stage highlighting support
- [ ] Connect stages to Track model
- [ ] Test with sequencer running

### 4.4 HAM Button Functionality
- [ ] Wire HAM button click events
- [ ] Prepare HAM Editor panel structure
- [ ] Add stage context menu
- [ ] Test button animations

---

## Phase 5: Track Sidebar 🔴
**Status**: Not Started
**Target**: Track controls and routing

### 5.1 Track Control Panel
- [ ] Create scrollable track list
- [ ] Add track color indicators (8 neon colors)
- [ ] Implement add/remove track buttons
- [ ] Style with Panel::Style::Recessed
- [ ] Test with 32+ tracks

### 5.2 Track Controls
- [ ] MIDI channel selector (1-16)
- [ ] Voice mode toggle (Mono/Poly)
- [ ] Division control (1/2/4/8/16)
- [ ] Swing amount slider
- [ ] Octave offset control

### 5.3 Track-Pattern Integration
- [ ] Connect to Pattern model
- [ ] Update on pattern switches
- [ ] Save/load track presets
- [ ] Solo/mute functionality
- [ ] Test multi-track playback

---

## Phase 6: Main Window Assembly 🔴
**Status**: Not Started
**Target**: Complete UI layout

### 6.1 MainComponent Refactor
- [ ] Delete old placeholder code completely
- [ ] Use HAM component library exclusively
- [ ] Implement proper layout management
- [ ] Add menu bar structure
- [ ] Test window resizing

### 6.2 Layout Integration
- [ ] Top: Transport bar
- [ ] Center: Stage grid
- [ ] Left: Track sidebar
- [ ] Bottom: Status bar placeholder
- [ ] Right: Future HAM Editor space

### 6.3 Dark Theme Application
- [ ] Apply PulseLookAndFeel globally
- [ ] Set background to pure black (#000000)
- [ ] Verify all text is readable
- [ ] Check contrast ratios
- [ ] Test in different lighting

---

## Phase 7: Advanced UI Features 🔴
**Status**: Not Started
**Target**: Extended functionality

### 7.1 Pattern Management UI
- [ ] Pattern selector dropdown
- [ ] Pattern chain visualization
- [ ] Scene grid (8x8)
- [ ] Morphing controls
- [ ] Copy/paste functionality

### 7.2 Scale & Music UI
- [ ] ScaleSlotPanel integration
- [ ] ScaleKeyboard component
- [ ] Root note selector
- [ ] Custom scale editor
- [ ] Chord mode controls

### 7.3 Visualization Components
- [ ] AccumulatorVisualizer
- [ ] GatePatternEditor
- [ ] VelocityCurveEditor
- [ ] RatchetPatternDisplay
- [ ] MIDI activity meters

### 7.4 HAM Editor Panel
- [ ] Extended stage editor
- [ ] Per-stage modulation routing
- [ ] CC mapping interface
- [ ] Ratchet pattern editor
- [ ] Probability controls

---

## Phase 8: Testing & Polish 🔴
**Status**: Not Started
**Target**: Production readiness

### 8.1 Performance Testing
- [ ] Profile UI render performance
- [ ] Test with 32 tracks playing
- [ ] Verify <5% CPU usage
- [ ] Check memory usage
- [ ] Optimize animation frame rates

### 8.2 MIDI Integration Testing
- [ ] Test all MIDI output on channel 16
- [ ] Verify note on/off pairs
- [ ] Check velocity and gate accuracy
- [ ] Test with external hardware
- [ ] Verify clock sync

### 8.3 User Experience Polish
- [ ] Add tooltips to all controls
- [ ] Implement keyboard shortcuts
- [ ] Add context menus
- [ ] Create preference panel
- [ ] Add about/help dialogs

### 8.4 Bug Fixes & Cleanup
- [ ] Fix any rendering glitches
- [ ] Resolve memory leaks
- [ ] Clean up deprecated code
- [ ] Update all documentation
- [ ] Final code review

---

## Current Status Summary

### ✅ Completed
1. Component libraries created (HAMComponentLibrary & Extended)
2. UI Designer tool built
3. Presentation layer structure
4. Base UI architecture
5. Message queue infrastructure (needs JUCE 8 fix)
6. MainComponent refactored with HAM components
7. TransportBar with Play/Stop/Record/BPM controls
8. StageGrid with 8 stage cards (2x2 slider layout)
9. Dark void aesthetic applied throughout
10. Message dispatcher for UI↔Engine communication

### 🟡 In Progress
- Phase 3: Transport Controls - Testing needed after JUCE 8 fix
- Phase 4: Stage Grid - Parameter connections ready, testing needed

### 🔴 Blocked/Waiting
- AbstractFifo API changes in JUCE 8 need fixing
- Cannot test UI↔Engine communication until compilation fixed

### 📝 Notes
- All Pulse UI components are ready in HAMComponentLibrary
- UI Designer app available for testing components
- Need to implement message queue before connecting UI to engine
- Focus on clean separation between UI and audio threads

---

## Next Steps (Priority Order)
1. **CRITICAL**: Fix AbstractFifo compilation for JUCE 8
2. **HIGH**: Test transport controls with fixed message queue
3. **HIGH**: Test stage grid parameter updates
4. **MEDIUM**: Add track sidebar (Phase 5)
5. **MEDIUM**: Implement keyboard shortcuts
6. **LOW**: Parameter management enhancements (Phase 2.2)

---

## Testing Protocol
After each phase completion:
1. Build with `./build.sh`
2. Test HAM.app on Desktop
3. Check MIDI output on channel 16
4. Profile CPU usage (<5% target)
5. Verify no memory leaks
6. Update this roadmap with completion status

---

## Code Quality Checklist
- [ ] No allocations in audio thread
- [ ] All UI updates on message thread
- [ ] Proper use of std::atomic for parameters
- [ ] No blocking operations in callbacks
- [ ] Clean separation of concerns
- [ ] Descriptive variable names (m_ prefix)
- [ ] Comments explain WHAT not HOW
- [ ] Old files deleted completely

---

*Last Updated: 2025-08-08 by Claude CLI*