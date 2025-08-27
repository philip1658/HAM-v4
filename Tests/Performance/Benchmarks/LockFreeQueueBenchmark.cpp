#include <benchmark/benchmark.h>
#include "../Core/BenchmarkHelpers.h"
#include "../Core/PerformanceMetrics.h"
#include "Infrastructure/Messaging/LockFreeMessageQueue.h"
#include "Infrastructure/Messaging/MessageTypes.h"
#include <thread>
#include <atomic>

using namespace HAM;
using namespace HAM::Performance;

/**
 * Benchmark lock-free message queue performance
 * Critical for UI <-> Audio thread communication
 */
static void BM_LockFreeQueue_SingleProducer(benchmark::State& state) {
    LockFreeMessageQueue<UIMessage, 1024> queue;
    
    int message_count = state.range(0);
    
    for (auto _ : state) {
        // Push messages
        for (int i = 0; i < message_count; ++i) {
            UIMessage msg;
            msg.type = UIMessage::Type::ParameterChanged;
            msg.parameterIndex = i;
            msg.value = static_cast<float>(i) / message_count;
            
            bool success = queue.push(msg);
            benchmark::DoNotOptimize(success);
        }
        
        // Pop messages
        UIMessage msg;
        while (queue.pop(msg)) {
            benchmark::DoNotOptimize(msg);
        }
    }
    
    state.counters["messages"] = message_count;
    state.counters["messages_per_sec"] = benchmark::Counter(
        state.iterations() * message_count, 
        benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_LockFreeQueue_SingleProducer)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark concurrent producer/consumer scenario
 */
static void BM_LockFreeQueue_Concurrent(benchmark::State& state) {
    LockFreeMessageQueue<UIMessage, 4096> queue;
    std::atomic<bool> stop{false};
    std::atomic<int> messages_produced{0};
    std::atomic<int> messages_consumed{0};
    
    // Producer thread (simulating UI)
    std::thread producer([&queue, &stop, &messages_produced] {
        while (!stop.load()) {
            UIMessage msg;
            msg.type = UIMessage::Type::ParameterChanged;
            msg.value = 0.5f;
            
            if (queue.push(msg)) {
                messages_produced.fetch_add(1);
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    // Consumer (audio thread)
    for (auto _ : state) {
        UIMessage msg;
        int consumed_this_iteration = 0;
        
        while (queue.pop(msg)) {
            benchmark::DoNotOptimize(msg);
            consumed_this_iteration++;
        }
        
        messages_consumed.fetch_add(consumed_this_iteration);
    }
    
    stop = true;
    producer.join();
    
    state.counters["produced"] = messages_produced.load();
    state.counters["consumed"] = messages_consumed.load();
    state.counters["throughput"] = benchmark::Counter(
        messages_consumed.load(), 
        benchmark::Counter::kIsRate
    );
}
HAM_BENCHMARK(BM_LockFreeQueue_Concurrent);

/**
 * Benchmark queue overflow behavior
 */
static void BM_LockFreeQueue_Overflow(benchmark::State& state) {
    constexpr size_t queue_size = 256;
    LockFreeMessageQueue<UIMessage, queue_size> queue;
    
    for (auto _ : state) {
        int successful_pushes = 0;
        int failed_pushes = 0;
        
        // Try to push more messages than queue capacity
        for (size_t i = 0; i < queue_size * 2; ++i) {
            UIMessage msg;
            msg.type = UIMessage::Type::ParameterChanged;
            msg.value = static_cast<float>(i);
            
            if (queue.push(msg)) {
                successful_pushes++;
            } else {
                failed_pushes++;
            }
        }
        
        // Clear queue
        UIMessage msg;
        while (queue.pop(msg)) {
            benchmark::DoNotOptimize(msg);
        }
        
        state.counters["successful_pushes"] = successful_pushes;
        state.counters["failed_pushes"] = failed_pushes;
    }
}
HAM_BENCHMARK(BM_LockFreeQueue_Overflow);

/**
 * Benchmark message dispatching
 */
static void BM_MessageDispatcher_Dispatch(benchmark::State& state) {
    MessageDispatcher dispatcher;
    
    // Register handlers
    dispatcher.registerHandler(UIMessage::Type::ParameterChanged,
        [](const UIMessage& msg) {
            benchmark::DoNotOptimize(msg.value);
        });
    
    dispatcher.registerHandler(UIMessage::Type::PatternChanged,
        [](const UIMessage& msg) {
            benchmark::DoNotOptimize(msg.patternData);
        });
    
    dispatcher.registerHandler(UIMessage::Type::TransportChanged,
        [](const UIMessage& msg) {
            benchmark::DoNotOptimize(msg.transportState);
        });
    
    std::vector<UIMessage> messages(state.range(0));
    for (size_t i = 0; i < messages.size(); ++i) {
        messages[i].type = static_cast<UIMessage::Type>(i % 3);
        messages[i].value = static_cast<float>(i);
    }
    
    for (auto _ : state) {
        for (const auto& msg : messages) {
            dispatcher.dispatch(msg);
        }
    }
    
    state.counters["messages"] = state.range(0);
}
BENCHMARK(BM_MessageDispatcher_Dispatch)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark lock-free queue with multiple producers
 */
static void BM_LockFreeQueue_MultiProducer(benchmark::State& state) {
    LockFreeMessageQueue<UIMessage, 8192> queue;
    int num_producers = state.range(0);
    std::atomic<bool> stop{false};
    std::atomic<int> total_produced{0};
    
    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&queue, &stop, &total_produced, i] {
            while (!stop.load()) {
                UIMessage msg;
                msg.type = UIMessage::Type::ParameterChanged;
                msg.parameterIndex = i;
                msg.value = 0.5f;
                
                if (queue.push(msg)) {
                    total_produced.fetch_add(1);
                }
                
                std::this_thread::yield();
            }
        });
    }
    
    ThreadContentionMonitor contention_monitor;
    
    for (auto _ : state) {
        UIMessage msg;
        int consumed = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        while (queue.pop(msg)) {
            benchmark::DoNotOptimize(msg);
            consumed++;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        // Check for contention
        double duration_us = std::chrono::duration<double, std::micro>(end - start).count();
        if (duration_us > 100 && consumed == 0) { // Taking long with no messages = contention
            contention_monitor.recordContention();
        }
    }
    
    stop = true;
    for (auto& producer : producers) {
        producer.join();
    }
    
    auto contention_stats = contention_monitor.getStats();
    state.counters["producers"] = num_producers;
    state.counters["total_produced"] = total_produced.load();
    state.counters["contentions"] = contention_stats.total_contentions;
}
BENCHMARK(BM_LockFreeQueue_MultiProducer)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Unit(benchmark::kMicrosecond);