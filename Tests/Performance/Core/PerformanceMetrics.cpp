#include "PerformanceMetrics.h"
#include <JuceHeader.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#endif

namespace HAM::Performance {

// Platform-specific implementations

#ifdef __APPLE__
static double getMacOSCPUTime() {
    thread_port_t thread = mach_thread_self();
    mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
    thread_basic_info_data_t info;
    
    kern_return_t kr = thread_info(thread, 
                                    THREAD_BASIC_INFO, 
                                    (thread_info_t)&info, 
                                    &count);
    
    mach_port_deallocate(mach_task_self(), thread);
    
    if (kr == KERN_SUCCESS) {
        return info.user_time.seconds + info.user_time.microseconds / 1000000.0 +
               info.system_time.seconds + info.system_time.microseconds / 1000000.0;
    }
    
    return 0.0;
}

static size_t getMacOSMemoryUsage() {
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), 
                  TASK_BASIC_INFO, 
                  (task_info_t)&info, 
                  &size) == KERN_SUCCESS) {
        return info.resident_size;
    }
    
    return 0;
}
#endif

// Global performance monitoring instance (singleton)
class GlobalPerformanceMonitor {
public:
    static GlobalPerformanceMonitor& getInstance() {
        static GlobalPerformanceMonitor instance;
        return instance;
    }
    
    CPUMonitor cpuMonitor;
    MemoryMonitor memoryMonitor;
    LatencyMonitor audioLatencyMonitor;
    LatencyMonitor midiLatencyMonitor;
    ThreadContentionMonitor threadMonitor;
    
    PerformanceSnapshot captureSnapshot() {
        PerformanceSnapshot snapshot;
        snapshot.cpu_usage = cpuMonitor.getMetrics();
        snapshot.memory = memoryMonitor.getStats();
        snapshot.audio_latency = audioLatencyMonitor.getMetrics();
        snapshot.midi_latency = midiLatencyMonitor.getMetrics();
        snapshot.midi_jitter = midiLatencyMonitor.getJitter();
        snapshot.thread_contention = threadMonitor.getStats();
        snapshot.timestamp = std::chrono::system_clock::now();
        return snapshot;
    }
    
    void reset() {
        cpuMonitor.reset();
        memoryMonitor.reset();
        audioLatencyMonitor.reset();
        midiLatencyMonitor.reset();
        threadMonitor.reset();
    }
    
private:
    GlobalPerformanceMonitor() = default;
};

// Export convenience functions
namespace {
    GlobalPerformanceMonitor& getMonitor() {
        return GlobalPerformanceMonitor::getInstance();
    }
}

void startCPUMeasurement() {
    getMonitor().cpuMonitor.startMeasurement();
}

void endCPUMeasurement() {
    getMonitor().cpuMonitor.endMeasurement();
}

void recordMemoryAllocation(size_t bytes) {
    getMonitor().memoryMonitor.recordAllocation(bytes);
}

void recordMemoryDeallocation(size_t bytes) {
    getMonitor().memoryMonitor.recordDeallocation(bytes);
}

void recordAudioLatency(double latency_ms) {
    getMonitor().audioLatencyMonitor.recordLatency(latency_ms);
}

void recordMidiLatency(double latency_ms) {
    getMonitor().midiLatencyMonitor.recordLatency(latency_ms);
}

void startLatencyEvent(const std::string& event_id, bool is_midi) {
    if (is_midi) {
        getMonitor().midiLatencyMonitor.startEvent(event_id);
    } else {
        getMonitor().audioLatencyMonitor.startEvent(event_id);
    }
}

void endLatencyEvent(const std::string& event_id, bool is_midi) {
    if (is_midi) {
        getMonitor().midiLatencyMonitor.endEvent(event_id);
    } else {
        getMonitor().audioLatencyMonitor.endEvent(event_id);
    }
}

void recordThreadContention() {
    getMonitor().threadMonitor.recordContention();
}

void recordLockWait(double wait_ms) {
    getMonitor().threadMonitor.recordLockWait(wait_ms);
}

PerformanceSnapshot capturePerformanceSnapshot() {
    return getMonitor().captureSnapshot();
}

void resetPerformanceMonitoring() {
    getMonitor().reset();
}

} // namespace HAM::Performance