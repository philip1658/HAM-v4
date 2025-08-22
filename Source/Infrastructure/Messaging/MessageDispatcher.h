/*
  ==============================================================================

    MessageDispatcher.h
    Central message dispatcher for UI<->Engine communication
    Manages message routing with priorities and performance monitoring

  ==============================================================================
*/

#pragma once

#include "LockFreeMessageQueue.h"
#include "MessageTypes.h"
#include <functional>
#include <unordered_map>

namespace HAM
{

/**
 * Message dispatcher for managing UI<->Engine communication
 * Provides high-level interface over lock-free queues
 */
class MessageDispatcher
{
public:
    //==============================================================================
    // Type aliases
    
    using UIMessageQueue = LockFreeMessageQueue<UIToEngineMessage, 2048>;
    using EngineMessageQueue = LockFreeMessageQueue<EngineToUIMessage, 4096>;
    using UIMessageHandler = std::function<void(const UIToEngineMessage&)>;
    using EngineMessageHandler = std::function<void(const EngineToUIMessage&)>;
    
    //==============================================================================
    // Constructor/Destructor
    
    MessageDispatcher()
    {
        setupDefaultHandlers();
    }
    
    ~MessageDispatcher() = default;
    
    //==============================================================================
    // UI to Engine Communication
    
    /**
     * Send message from UI to engine
     * Automatically determines priority based on message type
     */
    bool sendToEngine(const UIToEngineMessage& message)
    {
        auto priority = getPriorityForUIMessage(message.type);
        return m_uiToEngineQueue.push(message, priority);
    }
    
    /**
     * Send critical message (bypass priority queue)
     */
    bool sendCriticalToEngine(const UIToEngineMessage& message)
    {
        return m_uiToEngineQueue.push(message, UIMessageQueue::Priority::CRITICAL);
    }
    
    /**
     * Batch send multiple messages
     */
    void sendBatchToEngine(const MessageBatch<UIToEngineMessage>& batch)
    {
        for (size_t i = 0; i < batch.count; ++i)
        {
            sendToEngine(batch.messages[i]);
        }
    }
    
    //==============================================================================
    // Engine to UI Communication
    
    /**
     * Send message from engine to UI
     */
    bool sendToUI(const EngineToUIMessage& message)
    {
        auto priority = getPriorityForEngineMessage(message.type);
        return m_engineToUIQueue.push(message, priority);
    }
    
    /**
     * Send high-priority status update
     */
    bool sendStatusToUI(const EngineToUIMessage& message)
    {
        return m_engineToUIQueue.push(message, EngineMessageQueue::Priority::HIGH);
    }
    
    //==============================================================================
    // Message Processing (called from respective threads)
    
    /**
     * Process messages (generic interface)
     * @return Number of messages processed
     */
    int processMessages(int maxMessages = 50)
    {
        processUIMessages(maxMessages);
        return maxMessages;  // Return estimated count
    }
    
    /**
     * Process UI messages in audio thread
     * Call this from processBlock()
     */
    void processUIMessages(int maxMessages = 50)
    {
        UIToEngineMessage msg;
        int processed = 0;
        
        // Process critical messages first
        while (processed < 10 && m_uiToEngineQueue.getNumReady(UIMessageQueue::Priority::CRITICAL) > 0)
        {
            if (m_uiToEngineQueue.pop(msg))
            {
                dispatchUIMessage(msg);
                processed++;
            }
        }
        
        // Then process other messages up to limit
        m_uiToEngineQueue.popBatch(
            UIMessageQueue::Priority::NORMAL,
            [this, &processed, maxMessages](const UIToEngineMessage& msg)
            {
                if (processed < maxMessages)
                {
                    dispatchUIMessage(msg);
                    processed++;
                }
            },
            maxMessages - processed
        );
    }
    
    /**
     * Process engine messages in UI thread
     * Call this from timer callback
     */
    void processEngineMessages(int maxMessages = 100)
    {
        m_engineToUIQueue.popBatch(
            EngineMessageQueue::Priority::LOW,
            [this](const EngineToUIMessage& msg)
            {
                dispatchEngineMessage(msg);
            },
            maxMessages
        );
    }
    
    //==============================================================================
    // Handler Registration
    
    /**
     * Register handler for specific UI message type
     */
    void registerUIHandler(UIToEngineMessage::Type type, UIMessageHandler handler)
    {
        m_uiHandlers[type] = handler;
    }
    
    /**
     * Register handler for specific engine message type
     */
    void registerEngineHandler(EngineToUIMessage::Type type, EngineMessageHandler handler)
    {
        m_engineHandlers[type] = handler;
    }
    
    /**
     * Set default handler for unhandled UI messages
     */
    void setDefaultUIHandler(UIMessageHandler handler)
    {
        m_defaultUIHandler = handler;
    }
    
    /**
     * Set default handler for unhandled engine messages
     */
    void setDefaultEngineHandler(EngineMessageHandler handler)
    {
        m_defaultEngineHandler = handler;
    }
    
    //==============================================================================
    // Statistics & Monitoring
    
    /**
     * Get UI queue statistics
     */
    const UIMessageQueue::Statistics& getUIQueueStats() const
    {
        return m_uiToEngineQueue.getStatistics();
    }
    
    /**
     * Get engine queue statistics
     */
    const EngineMessageQueue::Statistics& getEngineQueueStats() const
    {
        return m_engineToUIQueue.getStatistics();
    }
    
    /**
     * Get combined performance report
     */
    juce::String getPerformanceReport() const
    {
        auto& uiStats = m_uiToEngineQueue.getStatistics();
        auto& engineStats = m_engineToUIQueue.getStatistics();
        
        return juce::String::formatted(
            "Message Queue Performance:\n"
            "UI->Engine:\n"
            "  Sent: %llu, Received: %llu, Dropped: %llu\n"
            "  Pool Hits: %llu, Misses: %llu (%.1f%% hit rate)\n"
            "  Avg Latency: %.2fms, Max: %.2fms\n"
            "  Queue Depth: %d/%d\n"
            "Engine->UI:\n"
            "  Sent: %llu, Received: %llu, Dropped: %llu\n"
            "  Pool Hits: %llu, Misses: %llu (%.1f%% hit rate)\n"
            "  Avg Latency: %.2fms, Max: %.2fms\n"
            "  Queue Depth: %d/%d\n",
            static_cast<unsigned long long>(uiStats.messagesSent.load()),
            static_cast<unsigned long long>(uiStats.messagesReceived.load()),
            static_cast<unsigned long long>(uiStats.messagesDropped.load()),
            static_cast<unsigned long long>(uiStats.poolHits.load()),
            static_cast<unsigned long long>(uiStats.poolMisses.load()),
            uiStats.poolHits > 0 ? 
                (100.0 * uiStats.poolHits) / (uiStats.poolHits + uiStats.poolMisses) : 0.0,
            static_cast<double>(uiStats.averageLatencyMs.load()),
            static_cast<double>(uiStats.maxLatencyMs.load()),
            static_cast<int>(uiStats.currentQueueDepth.load()),
            static_cast<int>(uiStats.maxQueueDepth.load()),
            static_cast<unsigned long long>(engineStats.messagesSent.load()),
            static_cast<unsigned long long>(engineStats.messagesReceived.load()),
            static_cast<unsigned long long>(engineStats.messagesDropped.load()),
            static_cast<unsigned long long>(engineStats.poolHits.load()),
            static_cast<unsigned long long>(engineStats.poolMisses.load()),
            engineStats.poolHits > 0 ?
                (100.0 * engineStats.poolHits) / (engineStats.poolHits + engineStats.poolMisses) : 0.0,
            static_cast<double>(engineStats.averageLatencyMs.load()),
            static_cast<double>(engineStats.maxLatencyMs.load()),
            static_cast<int>(engineStats.currentQueueDepth.load()),
            static_cast<int>(engineStats.maxQueueDepth.load())
        );
    }
    
    /**
     * Reset all statistics
     */
    void resetStatistics()
    {
        m_uiToEngineQueue.resetStatistics();
        m_engineToUIQueue.resetStatistics();
    }
    
    //==============================================================================
    // Queue Management
    
    /**
     * Clear all pending messages
     */
    void clearAll()
    {
        m_uiToEngineQueue.clear();
        m_engineToUIQueue.clear();
    }
    
    /**
     * Get number of pending UI messages
     */
    int getNumPendingUIMessages() const
    {
        return m_uiToEngineQueue.getNumReady();
    }
    
    /**
     * Get number of pending engine messages
     */
    int getNumPendingEngineMessages() const
    {
        return m_engineToUIQueue.getNumReady();
    }
    
private:
    //==============================================================================
    // Priority Mapping
    
    UIMessageQueue::Priority getPriorityForUIMessage(UIToEngineMessage::Type type) const
    {
        switch (type)
        {
            // Critical priority - immediate processing
            case UIToEngineMessage::TRANSPORT_PANIC:
                return UIMessageQueue::Priority::CRITICAL;
            
            // High priority - transport and timing
            case UIToEngineMessage::TRANSPORT_PLAY:
            case UIToEngineMessage::TRANSPORT_STOP:
            case UIToEngineMessage::TRANSPORT_PAUSE:
            case UIToEngineMessage::TRANSPORT_RECORD:
            case UIToEngineMessage::SET_BPM:
                return UIMessageQueue::Priority::HIGH;
            
            // Normal priority - most parameter changes
            case UIToEngineMessage::SET_SWING:
            case UIToEngineMessage::SET_MASTER_VOLUME:
            case UIToEngineMessage::SET_PATTERN_LENGTH:
            case UIToEngineMessage::LOAD_PATTERN:
            case UIToEngineMessage::UPDATE_STAGE:
            case UIToEngineMessage::UPDATE_TRACK:
            case UIToEngineMessage::SET_TRACK_MUTE:
            case UIToEngineMessage::SET_TRACK_SOLO:
            case UIToEngineMessage::ADD_TRACK:
            case UIToEngineMessage::REMOVE_TRACK:
            case UIToEngineMessage::START_MORPH:
                return UIMessageQueue::Priority::NORMAL;
            
            // Low priority - configuration
            case UIToEngineMessage::SET_SCALE:
            case UIToEngineMessage::SET_ACCUMULATOR_MODE:
            case UIToEngineMessage::SET_GATE_TYPE:
                return UIMessageQueue::Priority::LOW;
            
            // Deferred - debug and statistics
            case UIToEngineMessage::REQUEST_STATE_DUMP:
            case UIToEngineMessage::RESET_STATISTICS:
            case UIToEngineMessage::ENABLE_DEBUG_MODE:
            case UIToEngineMessage::DISABLE_DEBUG_MODE:
                return UIMessageQueue::Priority::DEFERRED;
            
            default:
                return UIMessageQueue::Priority::NORMAL;
        }
    }
    
    EngineMessageQueue::Priority getPriorityForEngineMessage(EngineToUIMessage::Type type) const
    {
        switch (type)
        {
            // High priority - transport status
            case EngineToUIMessage::TRANSPORT_STATUS:
            case EngineToUIMessage::ERROR_CPU_OVERLOAD:
            case EngineToUIMessage::BUFFER_UNDERRUN:
                return EngineMessageQueue::Priority::HIGH;
            
            // Normal priority - regular updates
            case EngineToUIMessage::PLAYHEAD_POSITION:
            case EngineToUIMessage::CURRENT_STAGE:
            case EngineToUIMessage::ACTIVE_VOICE_COUNT:
            case EngineToUIMessage::MIDI_NOTE_ON:
            case EngineToUIMessage::MIDI_NOTE_OFF:
                return EngineMessageQueue::Priority::NORMAL;
            
            // Low priority - statistics
            case EngineToUIMessage::CPU_USAGE:
            case EngineToUIMessage::TIMING_DRIFT:
            case EngineToUIMessage::DEBUG_TIMING_INFO:
            case EngineToUIMessage::DEBUG_QUEUE_STATS:
                return EngineMessageQueue::Priority::LOW;
            
            default:
                return EngineMessageQueue::Priority::NORMAL;
        }
    }
    
    //==============================================================================
    // Message Dispatching
    
    void dispatchUIMessage(const UIToEngineMessage& msg)
    {
        auto it = m_uiHandlers.find(msg.type);
        if (it != m_uiHandlers.end() && it->second)
        {
            it->second(msg);
        }
        else if (m_defaultUIHandler)
        {
            m_defaultUIHandler(msg);
        }
    }
    
    void dispatchEngineMessage(const EngineToUIMessage& msg)
    {
        auto it = m_engineHandlers.find(msg.type);
        if (it != m_engineHandlers.end() && it->second)
        {
            it->second(msg);
        }
        else if (m_defaultEngineHandler)
        {
            m_defaultEngineHandler(msg);
        }
    }
    
    void setupDefaultHandlers()
    {
        // Set up default unhandled message logging
        m_defaultUIHandler = [](const UIToEngineMessage& msg) {
            // Minimal default: no spam in real-time
        };
        
        m_defaultEngineHandler = [](const EngineToUIMessage& msg) {
            // Minimal default: no spam in UI
        };
    }
    
    //==============================================================================
    // Data Members
    
    UIMessageQueue m_uiToEngineQueue;
    EngineMessageQueue m_engineToUIQueue;
    
    std::unordered_map<UIToEngineMessage::Type, UIMessageHandler> m_uiHandlers;
    std::unordered_map<EngineToUIMessage::Type, EngineMessageHandler> m_engineHandlers;
    
    UIMessageHandler m_defaultUIHandler;
    EngineMessageHandler m_defaultEngineHandler;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MessageDispatcher)
};

} // namespace HAM