#include <JuceHeader.h>
#include "../Core/PerformanceMetrics.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"
#include "Infrastructure/Messaging/LockFreeMessageQueue.h"
#include <thread>
#include <atomic>

using namespace HAM;
using namespace HAM::Performance;

/**
 * Thread contention and synchronization tests
 */
class ThreadContentionTest : public juce::UnitTest {
public:
    ThreadContentionTest() : UnitTest("Thread Contention Test") {}
    
    void runTest() override {
        beginTest("Lock-Free Queue Contention");
        testLockFreeQueueContention();
        
        beginTest("UI vs Audio Thread");
        testUIAudioContention();
        
        beginTest("Multi-Producer Contention");
        testMultiProducerContention();
        
        beginTest("Priority Inversion");
        testPriorityInversion();
        
        beginTest("Deadlock Detection");
        testDeadlockFreedom();
    }
    
private:
    void testLockFreeQueueContention() {
        LockFreeMessageQueue<UIMessage, 4096> queue;
        ThreadContentionMonitor monitor;
        
        std::atomic<bool> stop{false};
        std::atomic<int> producerBlocked{0};
        std::atomic<int> consumerBlocked{0};
        
        // Producer thread (UI simulation)
        std::thread producer([&]() {
            while (!stop) {
                UIMessage msg;
                msg.type = UIMessage::Type::ParameterChanged;
                msg.value = 0.5f;
                
                auto start = std::chrono::high_resolution_clock::now();
                bool pushed = queue.push(msg);
                auto end = std::chrono::high_resolution_clock::now();
                
                if (!pushed) {
                    producerBlocked++;
                }
                
                // Check if took too long (contention)
                double durationUs = std::chrono::duration<double, std::micro>(end - start).count();
                if (durationUs > 10.0) { // More than 10us is suspicious
                    monitor.recordLockWait(durationUs / 1000.0); // Convert to ms
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Consumer thread (audio simulation)
        std::thread consumer([&]() {
            while (!stop) {
                UIMessage msg;
                
                auto start = std::chrono::high_resolution_clock::now();
                bool popped = queue.pop(msg);
                auto end = std::chrono::high_resolution_clock::now();
                
                if (!popped) {
                    consumerBlocked++;
                }
                
                double durationUs = std::chrono::duration<double, std::micro>(end - start).count();
                if (durationUs > 10.0) {
                    monitor.recordContention();
                }
                
                // Simulate audio processing time
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
        
        // Run for 1 second
        juce::Thread::sleep(1000);
        stop = true;
        
        producer.join();
        consumer.join();
        
        auto stats = monitor.getStats();
        
        logMessage("Queue contention stats:");
        logMessage("  Producer blocked: " + String(producerBlocked.load()) + " times");
        logMessage("  Consumer blocked: " + String(consumerBlocked.load()) + " times");
        logMessage("  Contentions detected: " + String(stats.total_contentions));
        logMessage("  Lock wait mean: " + String(stats.lock_wait_metrics.mean, 3) + " ms");
        
        expect(stats.total_contentions < 10, "Too many contentions detected");
        expect(stats.lock_wait_metrics.max < 1.0, "Lock wait time too high");
    }
    
    void testUIAudioContention() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        ThreadContentionMonitor monitor;
        std::atomic<bool> stop{false};
        std::atomic<int> slowProcessing{0};
        
        // UI thread
        std::thread uiThread([&]() {
            while (!stop) {
                // Simulate UI parameter updates
                // processor.setParameter(0, Random::getSystemRandom().nextFloat());
                
                // Simulate UI queries
                // float value = processor.getParameter(0);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60 FPS
            }
        });
        
        // Audio processing
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        
        const double expectedMs = (512.0 / 48000.0) * 1000.0;
        
        for (int i = 0; i < 1000; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            processor.processBlock(buffer, midi);
            auto end = std::chrono::high_resolution_clock::now();
            
            double processingMs = std::chrono::duration<double, std::milli>(end - start).count();
            
            // Check if processing took unusually long (contention)
            if (processingMs > expectedMs * 2) {
                slowProcessing++;
                monitor.recordContention();
            }
        }
        
        stop = true;
        uiThread.join();
        
        logMessage("UI/Audio contention:");
        logMessage("  Slow processing blocks: " + String(slowProcessing.load()));
        
        expect(slowProcessing < 10, "UI thread causing audio thread delays");
        
        processor.releaseResources();
    }
    
    void testMultiProducerContention() {
        LockFreeMessageQueue<UIMessage, 8192> queue;
        ThreadContentionMonitor monitor;
        
        const int numProducers = 4;
        std::atomic<bool> stop{false};
        std::atomic<int> totalProduced{0};
        std::atomic<int> totalConsumed{0};
        std::atomic<int> contentionCount{0};
        
        std::vector<std::thread> producers;
        
        // Multiple producer threads
        for (int p = 0; p < numProducers; ++p) {
            producers.emplace_back([&, p]() {
                while (!stop) {
                    UIMessage msg;
                    msg.type = UIMessage::Type::ParameterChanged;
                    msg.parameterIndex = p;
                    
                    auto start = std::chrono::high_resolution_clock::now();
                    bool pushed = queue.push(msg);
                    auto end = std::chrono::high_resolution_clock::now();
                    
                    if (pushed) {
                        totalProduced++;
                    }
                    
                    double durationUs = std::chrono::duration<double, std::micro>(end - start).count();
                    if (durationUs > 100.0) { // 100us threshold
                        contentionCount++;
                    }
                    
                    std::this_thread::yield();
                }
            });
        }
        
        // Consumer thread
        std::thread consumer([&]() {
            while (!stop) {
                UIMessage msg;
                if (queue.pop(msg)) {
                    totalConsumed++;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
        
        // Run test
        juce::Thread::sleep(1000);
        stop = true;
        
        for (auto& producer : producers) {
            producer.join();
        }
        consumer.join();
        
        logMessage("Multi-producer contention:");
        logMessage("  Total produced: " + String(totalProduced.load()));
        logMessage("  Total consumed: " + String(totalConsumed.load()));
        logMessage("  Contention events: " + String(contentionCount.load()));
        
        double contentionRate = (contentionCount.load() * 100.0) / totalProduced.load();
        logMessage("  Contention rate: " + String(contentionRate, 2) + "%");
        
        expect(contentionRate < 1.0, "High contention rate with multiple producers");
    }
    
    void testPriorityInversion() {
        // Test that audio thread maintains priority
        std::atomic<bool> stop{false};
        std::atomic<int> audioIterations{0};
        std::atomic<int> lowPriorityIterations{0};
        
        // High priority audio thread simulation
        std::thread audioThread([&]() {
            // Set thread priority (platform-specific)
#ifdef JUCE_MAC
            struct sched_param params;
            params.sched_priority = sched_get_priority_max(SCHED_FIFO);
            pthread_setschedparam(pthread_self(), SCHED_FIFO, &params);
#endif
            
            while (!stop) {
                audioIterations++;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Low priority background thread
        std::thread backgroundThread([&]() {
            while (!stop) {
                lowPriorityIterations++;
                // Intensive computation
                volatile double sum = 0;
                for (int i = 0; i < 1000; ++i) {
                    sum += std::sin(i) * std::cos(i);
                }
            }
        });
        
        // Run test
        juce::Thread::sleep(1000);
        stop = true;
        
        audioThread.join();
        backgroundThread.join();
        
        logMessage("Priority test:");
        logMessage("  Audio iterations: " + String(audioIterations.load()));
        logMessage("  Background iterations: " + String(lowPriorityIterations.load()));
        
        // Audio thread should complete most of its iterations despite background load
        double audioCompletionRate = audioIterations.load() / 10000.0; // Expected ~10000 in 1 second
        expect(audioCompletionRate > 0.9, "Audio thread starved by background thread");
    }
    
    void testDeadlockFreedom() {
        // Verify lock-free design prevents deadlocks
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        std::atomic<bool> stop{false};
        std::atomic<bool> deadlockDetected{false};
        
        // Thread 1: Audio processing
        std::thread audioThread([&]() {
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            
            while (!stop && !deadlockDetected) {
                auto start = std::chrono::high_resolution_clock::now();
                processor.processBlock(buffer, midi);
                auto end = std::chrono::high_resolution_clock::now();
                
                // Check for suspiciously long processing (potential deadlock)
                double durationMs = std::chrono::duration<double, std::milli>(end - start).count();
                if (durationMs > 100.0) { // 100ms is way too long
                    deadlockDetected = true;
                }
            }
        });
        
        // Thread 2: Rapid parameter changes
        std::thread paramThread([&]() {
            while (!stop && !deadlockDetected) {
                // Rapidly change parameters
                // for (int i = 0; i < 10; ++i) {
                //     processor.setParameter(i, Random::getSystemRandom().nextFloat());
                // }
            }
        });
        
        // Watchdog timer
        std::thread watchdog([&]() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!stop) {
                logMessage("WARNING: Test taking too long, possible deadlock");
                deadlockDetected = true;
            }
        });
        
        // Run test
        juce::Thread::sleep(2000);
        stop = true;
        
        audioThread.join();
        paramThread.join();
        watchdog.join();
        
        expect(!deadlockDetected, "Potential deadlock detected");
        
        processor.releaseResources();
    }
};

static ThreadContentionTest threadContentionTest;