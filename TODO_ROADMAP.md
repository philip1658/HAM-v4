# HAM TODO Roadmap

## Executive Summary
**Total TODOs**: 85 across the codebase  
**Last Updated**: August 27, 2025  
**Target**: Reduce to < 50 within current sprint

## TODO Categories by Priority

### 🔥 **Critical** (12 TODOs) - Must implement before v1.0
These TODOs represent core functionality gaps that prevent the application from working properly.

#### Audio Engine & Core (6 TODOs)
- `Infrastructure/Audio/HAMAudioProcessor.h:227` - Re-enable TrackGateProcessor (BLOCKER)
- `Infrastructure/Audio/HAMAudioProcessor.cpp:924` - Re-enable TrackGateProcessor (BLOCKER)  
- `Infrastructure/Audio/HAMAudioProcessor.cpp:1166` - Re-enable TrackGateProcessor (BLOCKER)
- `Domain/Engines/SequencerEngine.cpp:545` - Apply scale quantization integration
- `Domain/Engines/SequencerEngine.cpp:874` - Implement skip condition logic
- `Infrastructure/Audio/HAMAudioProcessor.cpp:1485` - Full plugin restoration logic

#### Transport & Timing (4 TODOs)
- `Presentation/Core/AppController.cpp:161` - Proper pause with position maintenance
- `Domain/Transport/SyncManager.cpp:450` - Apply master clock timing compensation
- `Infrastructure/Audio/HAMAudioProcessor.cpp:421` - Per-track swing application
- `Infrastructure/Audio/HAMAudioProcessor.cpp:434` - Pattern length management

#### UI Core (2 TODOs)
- `Main.cpp:121` - Add save prompt on quit
- `Presentation/Core/UICoordinator.cpp:253` - All notes off functionality

### 🟡 **High Priority** (23 TODOs) - Next release cycle
Important features and improvements that enhance usability.

#### Plugin System (8 TODOs)
- `Infrastructure/Plugins/PluginSandbox.cpp:483` - Parameter control via IPC
- `Infrastructure/Plugins/PluginSandbox.cpp:489` - Parameter query via IPC
- `Infrastructure/Plugins/PluginSandbox.cpp:501` - Editor showing via IPC
- `Infrastructure/Plugins/PluginSandbox.cpp:506` - Editor closing via IPC
- `Infrastructure/Plugins/PluginSandbox.cpp:523` - CPU usage calculation
- `Infrastructure/Plugins/PluginSandbox.cpp:885` - User notification on recovery failure
- `Infrastructure/Plugins/PluginSandbox.cpp:914` - Sandboxed wrapper interface
- `Presentation/Core/AppController.cpp:383` - Plugin loading via PluginManager

#### Track Management (8 TODOs)
- `Presentation/Core/UICoordinator.cpp:362` - setTrackMidiChannel implementation
- `Presentation/Core/UICoordinator.cpp:368` - setTrackVoiceMode implementation  
- `Presentation/Core/UICoordinator.cpp:374` - setTrackDivision implementation
- `Presentation/Core/UICoordinator.cpp:380` - setTrackSwing implementation
- `Presentation/Core/UICoordinator.cpp:386` - setTrackOctave implementation
- `Presentation/Core/UICoordinator.cpp:392` - setTrackMaxPulseLength implementation
- `Presentation/Core/UICoordinator.cpp:400` - selectTrack implementation
- `Presentation/Core/AppController.cpp:335` - SET_TRACK_PAN message type

#### Project Management (4 TODOs)
- `Presentation/Core/AppController.cpp:200` - Pattern saving implementation
- `Presentation/Core/AppController.cpp:439` - Project loading implementation
- `Presentation/Core/AppController.cpp:446` - Project saving implementation
- `MainComponent.cpp:100` - MIDI export functionality

#### Debug & Monitoring (3 TODOs)
- `Infrastructure/Audio/HAMAudioProcessor.cpp:654` - Debug mode implementation
- `Infrastructure/Audio/HAMAudioProcessor.cpp:659` - Debug mode implementation
- `Infrastructure/Audio/HAMAudioProcessor.cpp:699` - Debug mode check

### 🟠 **Medium Priority** (28 TODOs) - Future versions
Features that would be nice to have but aren't essential.

#### External Sync (13 TODOs)
- `Domain/Transport/SyncManager.h:109` - Link SDK implementation
- `Domain/Transport/SyncManager.h:118` - Link peer count
- `Domain/Transport/SyncManager.h:213` - Link session management
- `Domain/Transport/SyncManager.cpp:196` - Link session initialization
- `Domain/Transport/SyncManager.cpp:202` - Link session disable
- `Domain/Transport/SyncManager.cpp:209` - Link phase alignment
- `Domain/Transport/SyncManager.cpp:286` - Link connection status
- Plus 6 more Link-related TODOs

#### UI Enhancements (9 TODOs)
- `Presentation/Core/MainWindow.cpp:131` - Menu bar model
- `Presentation/Core/MainWindow.cpp:162` - Edit menu actions
- `Presentation/Core/MainWindow.cpp:171` - Fullscreen toggle
- `Presentation/Core/MainWindow.cpp:174` - UI layout reset
- `Presentation/Core/MainWindow.cpp:177` - MIDI monitor toggle
- `Presentation/Core/MainWindow.cpp:190` - Documentation viewer
- `Presentation/Core/UICoordinator.cpp:241` - Pattern selection
- `Presentation/Core/UICoordinator.cpp:247` - Pattern chaining
- `Presentation/Views/CustomScaleEditor.cpp:588` - MIDI scale preview

#### Track Features (6 TODOs)  
- `Infrastructure/Audio/HAMAudioProcessor.cpp:558` - Scale object usage
- `Infrastructure/Audio/HAMAudioProcessor.cpp:581` - Per-stage GateType
- `Infrastructure/Audio/HAMAudioProcessor.cpp:1270` - Volume and pan support
- `Infrastructure/Audio/HAMAudioProcessor.cpp:1426` - Volume/pan restoration
- `Presentation/Core/UICoordinator.cpp:349` - Accumulator editor
- `Presentation/Views/ScaleSlotSelector.cpp:434` - Copy functionality

### 🔵 **Low Priority** (22 TODOs) - Nice to have
Non-essential improvements and legacy cleanup.

#### Legacy Cleanup (7 TODOs)
All TODOs in `*.backup` files can be safely removed when backup files are deleted:
- `MainComponent.cpp.backup` (6 TODOs)
- `HAMAudioProcessor.cpp.backup` (1 TODO)

#### HAMEditorPanel (8 TODOs)  
Panel-related TODOs that depend on HAMEditorPanel implementation:
- `Presentation/Core/UICoordinator.h:123` - Panel declaration
- `Presentation/Core/UICoordinator.cpp:583` - Panel creation
- `Presentation/Core/UICoordinator.cpp:598` - Panel hiding
- `Presentation/Core/UICoordinator.cpp:745` - Panel positioning
- Plus 4 backup file references

#### Future Features (7 TODOs)
- `Presentation/Core/UICoordinator.cpp:407` - addTrack implementation
- `Presentation/Core/UICoordinator.cpp:414` - removeTrack implementation
- `Presentation/Core/UICoordinator.cpp:535` - Plugin browser configuration
- `Presentation/Core/AppController.cpp:394` - Plugin removal
- `Presentation/Core/AppController.cpp:403` - Plugin editor window
- `Presentation/Core/AppController.cpp:500` - Engine message handlers
- Plus 1 more future feature

## Implementation Strategy

### Phase 1: Critical Fixes (Target: 1-2 days)
1. **TrackGateProcessor Integration** - Fix the 3 blocking TODOs
2. **Transport Fixes** - Implement proper pause and timing  
3. **Scale Integration** - Connect scale quantization
4. **Basic Safety** - Add save prompt on quit

**Estimated Effort**: 8-12 hours  
**Impact**: High - Makes app stable and usable  
**Dependencies**: None

### Phase 2: High Priority Features (Target: 1 week)
1. **Plugin System IPC** - Complete sandboxing communication
2. **Track Management** - Implement all track property setters  
3. **Project Management** - Save/load functionality
4. **Debug Infrastructure** - Monitoring and diagnostics

**Estimated Effort**: 20-30 hours  
**Impact**: High - Makes app feature-complete  
**Dependencies**: Phase 1 completion

### Phase 3: Polish & Enhancement (Target: 2-3 weeks)  
1. **External Sync** - Link integration for professional use
2. **UI Polish** - Menu systems, fullscreen, layout
3. **Advanced Features** - Pattern chaining, advanced editing

**Estimated Effort**: 40-60 hours  
**Impact**: Medium - Makes app professional-grade  
**Dependencies**: Phases 1-2 completion

### Phase 4: Cleanup (Target: 1 day)
1. **Legacy Removal** - Delete backup files and related TODOs
2. **Code Organization** - Final refactoring
3. **Documentation** - Update remaining inline docs

**Estimated Effort**: 4-8 hours  
**Impact**: Low - Code cleanliness  
**Dependencies**: All phases completion

## Success Metrics

### Immediate Goals (Current Sprint)
- [ ] Reduce TODO count from 85 to < 50 
- [ ] Fix all 12 Critical TODOs
- [ ] Implement at least 10 High Priority TODOs
- [ ] Establish 60%+ code coverage

### Release Goals (v1.0)  
- [ ] All Critical and High Priority TODOs resolved
- [ ] 80%+ code coverage
- [ ] < 20 remaining TODOs (all Low Priority)
- [ ] Zero backup file TODOs

## Quick Reference

### Easiest Wins (< 30 mins each)
1. `Main.cpp:121` - Add save dialog
2. Delete all `*.backup` files (removes 7 TODOs instantly)  
3. `Presentation/Views/ScaleSlotSelector.cpp:434` - Copy placeholder
4. `Infrastructure/Audio/HAMAudioProcessor.cpp:558` - Use Scale object
5. Various debug mode implementations

### Biggest Impact
1. TrackGateProcessor integration (fixes 3 critical bugs)
2. Plugin IPC system (enables plugin hosting)  
3. Transport timing fixes (improves accuracy)
4. Track management (enables track operations)

### Dependencies Map
```
TrackGateProcessor → Audio Engine Stability
Plugin IPC → Plugin System Features  
Transport Timing → Accurate Playback
Scale Integration → Musical Functionality
Project Management → User Workflow
```

---

**Next Actions**: Start with Phase 1 Critical fixes, focusing on TrackGateProcessor integration as the highest impact item.