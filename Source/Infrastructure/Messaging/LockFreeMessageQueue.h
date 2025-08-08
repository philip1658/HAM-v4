/*
  ==============================================================================

    LockFreeMessageQueue.h
    Enhanced lock-free message queue with pooling and priorities
    Zero allocations in real-time thread

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <chrono>
#include <memory>

namespace HAM
{

/**
 * Lock-free message queue with pooling and priority support
 * Designed for real-time audio thread communication
 */
template<typename MessageType, size_t QueueSize = 2048>
class LockFreeMessageQueue
{
public:
    //==============================================================================
    // Message Priority Levels
    
    enum class Priority
    {
        CRITICAL = 0,   // Process immediately (e.g., stop all notes)
        HIGH = 1,       // High priority (e.g., transport changes)
        NORMAL = 2,     // Normal priority (e.g., parameter changes)
        LOW = 3,        // Low priority (e.g., UI updates)
        DEFERRED = 4    // Process when idle
    };
    
    //==============================================================================
    // Message Wrapper with metadata
    
    struct MessageWrapper
    {
        MessageType message;
        Priority priority = Priority::NORMAL;
        std::chrono::high_resolution_clock::time_point timestamp;
        uint32_t sequenceNumber = 0;
        bool isPooled = false;
    };
    
    //==============================================================================
    // Performance Statistics
    
    struct Statistics
    {
        std::atomic<uint64_t> messagesSent{0};
        std::atomic<uint64_t> messagesReceived{0};
        std::atomic<uint64_t> messagesDropped{0};
        std::atomic<uint64_t> poolHits{0};
        std::atomic<uint64_t> poolMisses{0};
        std::atomic<double> averageLatencyMs{0.0};
        std::atomic<double> maxLatencyMs{0.0};
        std::atomic<int> currentQueueDepth{0};
        std::atomic<int> maxQueueDepth{0};
        
        void reset()
        {
            messagesSent = 0;
            messagesReceived = 0;
            messagesDropped = 0;
            poolHits = 0;
            poolMisses = 0;
            averageLatencyMs = 0.0;
            maxLatencyMs = 0.0;
            currentQueueDepth = 0;
            maxQueueDepth = 0;
        }
    };
    
    //==============================================================================
    // Constructor/Destructor
    
    LockFreeMessageQueue()
    {
        // Initialize message pool
        for (size_t i = 0; i < QueueSize; ++i)
        {
            m_messagePool[i].isPooled = true;
            m_poolIndices[i] = static_cast<int>(i);
        }
        m_poolHead = 0;
        m_poolTail = QueueSize - 1;
        
        // Priority queues are already initialized with their size
    }
    
    ~LockFreeMessageQueue() = default;
    
    //==============================================================================
    // Core Operations
    
    /**
     * Push message to queue (lock-free, real-time safe)
     * @param message The message to send
     * @param priority Message priority level
     * @return true if message was queued, false if queue full
     */
    bool push(const MessageType& message, Priority priority = Priority::NORMAL)
    {
        // Get message wrapper from pool
        MessageWrapper* wrapper = allocateFromPool();
        if (!wrapper)
        {
            m_stats.messagesDropped++;
            return false;
        }
        
        // Fill wrapper
        wrapper->message = message;
        wrapper->priority = priority;
        wrapper->timestamp = std::chrono::high_resolution_clock::now();
        wrapper->sequenceNumber = m_sequenceCounter.fetch_add(1);
        
        // Add to appropriate priority queue
        auto& queue = m_priorityQueues[static_cast<int>(priority)].fifo;
        
        // Use ScopedWrite for JUCE 8
        if (queue.getFreeSpace() > 0)
        {
            auto scope = queue.write(1);
            if (scope.blockSize1 > 0)
            {
                m_messages[static_cast<int>(priority)][scope.startIndex1] = wrapper;
            }
            else if (scope.blockSize2 > 0)
            {
                m_messages[static_cast<int>(priority)][scope.startIndex2] = wrapper;
            }
            
            // Update statistics
            m_stats.messagesSent++;
            int depth = updateQueueDepth(1);
            int maxDepth = m_stats.maxQueueDepth.load();
            if (depth > maxDepth)
            {
                m_stats.maxQueueDepth = depth;
            }
            
            return true;
        }
        else
        {
            // Queue full, return to pool
            returnToPool(wrapper);
            m_stats.messagesDropped++;
            return false;
        }
    }
    
    /**
     * Pop highest priority message from queue (lock-free, real-time safe)
     * @param message Output parameter for received message
     * @return true if message was retrieved, false if queue empty
     */
    bool pop(MessageType& message)
    {
        // Check queues in priority order
        for (int p = 0; p < static_cast<int>(Priority::DEFERRED) + 1; ++p)
        {
            auto& queue = m_priorityQueues[p].fifo;
            
            if (queue.getNumReady() > 0)
            {
                auto scope = queue.read(1);
                MessageWrapper* wrapper = nullptr;
                
                if (scope.blockSize1 > 0)
                {
                    wrapper = m_messages[p][scope.startIndex1];
                }
                else if (scope.blockSize2 > 0)
                {
                    wrapper = m_messages[p][scope.startIndex2];
                }
                
                if (wrapper)
                {
                    // Copy message
                    message = wrapper->message;
                    
                    // Calculate latency
                    auto now = std::chrono::high_resolution_clock::now();
                    auto latency = std::chrono::duration<double, std::milli>(
                        now - wrapper->timestamp).count();
                    updateLatencyStats(latency);
                    
                    // Return wrapper to pool
                    returnToPool(wrapper);
                    
                    // Update statistics
                    m_stats.messagesReceived++;
                    updateQueueDepth(-1);
                    
                    return true;
                }
            }
        }
        
        return false;
    }
    
    /**
     * Pop all messages up to a certain priority level
     * Useful for batch processing
     */
    template<typename Callback>
    void popBatch(Priority maxPriority, Callback&& callback, int maxMessages = 100)
    {
        int processed = 0;
        MessageType msg;
        
        for (int p = 0; p <= static_cast<int>(maxPriority) && processed < maxMessages; ++p)
        {
            auto& queue = m_priorityQueues[p].fifo;
            int numReady = queue.getNumReady();
            
            while (numReady > 0 && processed < maxMessages)
            {
                if (pop(msg))
                {
                    callback(msg);
                    processed++;
                }
                numReady--;
            }
        }
    }
    
    /**
     * Peek at next message without removing it
     */
    bool peek(MessageType& message) const
    {
        for (int p = 0; p < static_cast<int>(Priority::DEFERRED) + 1; ++p)
        {
            const auto& queue = m_priorityQueues[p].fifo;
            
            if (queue.getNumReady() > 0)
            {
                // This is a peek operation - would need custom implementation
                // For now, return false as peek isn't directly supported by AbstractFifo
                return false;
            }
        }
        
        return false;
    }
    
    //==============================================================================
    // Queue Management
    
    /**
     * Get number of messages waiting in queue
     */
    int getNumReady() const
    {
        int total = 0;
        for (const auto& pq : m_priorityQueues)
        {
            total += pq.fifo.getNumReady();
        }
        return total;
    }
    
    /**
     * Get number of messages at specific priority
     */
    int getNumReady(Priority priority) const
    {
        return m_priorityQueues[static_cast<int>(priority)].fifo.getNumReady();
    }
    
    /**
     * Check if queue is empty
     */
    bool isEmpty() const
    {
        return getNumReady() == 0;
    }
    
    /**
     * Clear all messages
     */
    void clear()
    {
        MessageType dummy;
        while (pop(dummy)) {}
        m_stats.currentQueueDepth = 0;
    }
    
    //==============================================================================
    // Statistics & Monitoring
    
    /**
     * Get performance statistics
     */
    const Statistics& getStatistics() const { return m_stats; }
    
    /**
     * Reset statistics
     */
    void resetStatistics() { m_stats.reset(); }
    
    /**
     * Get pool utilization (0.0 to 1.0)
     */
    float getPoolUtilization() const
    {
        int available = 0;
        int head = m_poolHead.load();
        int tail = m_poolTail.load();
        
        if (tail >= head)
        {
            available = tail - head + 1;
        }
        else
        {
            available = (QueueSize - head) + tail + 1;
        }
        
        return 1.0f - (static_cast<float>(available) / QueueSize);
    }
    
private:
    //==============================================================================
    // Message Pool Management
    
    /**
     * Allocate message wrapper from pool (lock-free)
     */
    MessageWrapper* allocateFromPool()
    {
        int head = m_poolHead.load();
        int nextHead = (head + 1) % QueueSize;
        
        // Check if pool is empty
        if (head == m_poolTail.load())
        {
            m_stats.poolMisses++;
            return nullptr;
        }
        
        // Try to advance head
        while (!m_poolHead.compare_exchange_weak(head, nextHead))
        {
            nextHead = (head + 1) % QueueSize;
            if (head == m_poolTail.load())
            {
                m_stats.poolMisses++;
                return nullptr;
            }
        }
        
        m_stats.poolHits++;
        int index = m_poolIndices[head];
        return &m_messagePool[index];
    }
    
    /**
     * Return message wrapper to pool (lock-free)
     */
    void returnToPool(MessageWrapper* wrapper)
    {
        if (!wrapper || !wrapper->isPooled)
            return;
        
        // Find index in pool
        int index = static_cast<int>(wrapper - m_messagePool.data());
        if (index < 0 || index >= QueueSize)
            return;
        
        // Return to pool
        int tail = m_poolTail.load();
        int nextTail = (tail + 1) % QueueSize;
        
        while (!m_poolTail.compare_exchange_weak(tail, nextTail))
        {
            nextTail = (tail + 1) % QueueSize;
        }
        
        m_poolIndices[nextTail] = index;
    }
    
    //==============================================================================
    // Statistics Helpers
    
    void updateLatencyStats(double latencyMs)
    {
        // Update max latency
        double maxLatency = m_stats.maxLatencyMs.load();
        while (latencyMs > maxLatency && 
               !m_stats.maxLatencyMs.compare_exchange_weak(maxLatency, latencyMs))
        {
            // Retry
        }
        
        // Update average latency (simplified exponential moving average)
        double avgLatency = m_stats.averageLatencyMs.load();
        double newAvg = avgLatency * 0.95 + latencyMs * 0.05;
        m_stats.averageLatencyMs = newAvg;
    }
    
    int updateQueueDepth(int delta)
    {
        int newDepth = m_stats.currentQueueDepth.fetch_add(delta) + delta;
        return newDepth;
    }
    
    //==============================================================================
    // Data Members
    
    // Priority queues (one per priority level)
    // Each queue needs to be initialized with a size
    struct PriorityQueue {
        juce::AbstractFifo fifo{QueueSize / 5};
    };
    std::array<PriorityQueue, 5> m_priorityQueues;
    
    // Message storage per priority
    std::array<std::array<MessageWrapper*, QueueSize/5>, 5> m_messages;
    
    // Message pool for zero-allocation operation
    std::array<MessageWrapper, QueueSize> m_messagePool;
    std::array<int, QueueSize> m_poolIndices;
    std::atomic<int> m_poolHead{0};
    std::atomic<int> m_poolTail{0};
    
    // Statistics
    mutable Statistics m_stats;
    
    // Sequence counter for message ordering
    std::atomic<uint32_t> m_sequenceCounter{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LockFreeMessageQueue)
};

} // namespace HAM