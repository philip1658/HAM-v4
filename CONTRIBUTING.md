# Contributing to HAM

## Development Workflow

### 1. Check ROADMAP.md
Always read `/Users/philipkrieger/Desktop/HAM/ROADMAP.md` before starting work.

### 2. Branch Strategy
- `main` - Stable, tested code only
- `develop` - Active development
- `feature/*` - New features
- `fix/*` - Bug fixes

### 3. Commit Messages
Use conventional commits:
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation
- `test:` Test additions
- `perf:` Performance improvements
- `refactor:` Code refactoring

### 4. Testing Requirements
Before marking any task complete:
1. Run `./Tests/Scripts/daily_test.sh`
2. Create test evidence in `Tests/Evidence/`
3. Update ROADMAP.md status to "Awaiting Approval"
4. Wait for Philip's approval

### 5. Code Standards

#### C++ Style
```cpp
// Classes: PascalCase
class SequencerEngine {};

// Members: m_ prefix
float m_currentBpm;

// Functions: camelCase
void processNextStage();

// Constants: UPPER_SNAKE
const int MAX_VOICES = 16;
```

#### Real-Time Audio Rules
- ❌ NO allocations in audio thread
- ❌ NO locks or mutexes
- ❌ NO unbounded operations
- ✅ Lock-free data structures
- ✅ Pre-allocated buffers
- ✅ Atomic operations

### 6. Performance Targets
- CPU: < 5% on M3 Max
- Memory: < 200MB base
- MIDI Jitter: < 0.1ms

### 7. Clean Code Policy
- Delete old files when replacing
- No commented-out code
- No temporary solutions
- Fix root causes, not symptoms

## Pull Request Process

1. Create feature branch
2. Implement with tests
3. Run all test scripts
4. Update documentation
5. Create PR with evidence
6. Wait for CI checks
7. Request Philip's review

## Questions?
Contact Philip Krieger