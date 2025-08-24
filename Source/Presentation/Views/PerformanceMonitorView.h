#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

namespace HAM {

class HAMAudioProcessor;
class ChannelManager;

/**
 * @brief Performance monitoring dashboard view
 * 
 * Displays real-time performance metrics including CPU usage,
 * memory usage, MIDI events processed, and other statistics.
 */
class PerformanceMonitorView : public juce::Component,
                                private juce::Timer
{
public:
    PerformanceMonitorView();
    ~PerformanceMonitorView() override;
    
    /** Connect to audio processor for performance data */
    void setAudioProcessor(HAMAudioProcessor* processor);
    
    /** Connect to channel manager for MIDI statistics */
    void setChannelManager(ChannelManager* channelManager);
    
    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    //==============================================================================
    // Timer callback for updating display
    void timerCallback() override;
    
    //==============================================================================
    // UI Components
    
    struct MetricDisplay
    {
        juce::Label nameLabel;
        juce::Label valueLabel;
        juce::ProgressBar progressBar;
        std::atomic<float> progressValue{0.0f};
        
        void setup(const juce::String& name, juce::Component* parent);
        void setValue(float value, const juce::String& suffix = "");
        void setProgress(float normalizedValue);
    };
    
    MetricDisplay m_cpuDisplay;
    MetricDisplay m_memoryDisplay;
    MetricDisplay m_eventsDisplay;
    MetricDisplay m_droppedDisplay;
    MetricDisplay m_bufferDisplay;
    MetricDisplay m_latencyDisplay;
    
    //==============================================================================
    // Data sources
    HAMAudioProcessor* m_audioProcessor{nullptr};
    ChannelManager* m_channelManager{nullptr};
    
    //==============================================================================
    // Update rate
    static constexpr int UPDATE_RATE_MS = 100; // 10 Hz update rate
    
    //==============================================================================
    // Formatting helpers
    static juce::String formatMemory(size_t bytes);
    static juce::String formatNumber(int value);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceMonitorView)
};

} // namespace HAM