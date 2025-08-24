#include "PerformanceMonitorView.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"
#include "Domain/Services/ChannelManager.h"

namespace HAM {

//==============================================================================
// MetricDisplay implementation

void PerformanceMonitorView::MetricDisplay::setup(const juce::String& name, 
                                                   juce::Component* parent)
{
    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::left);
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    parent->addAndMakeVisible(nameLabel);
    
    valueLabel.setText("0", juce::dontSendNotification);
    valueLabel.setJustificationType(juce::Justification::right);
    valueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    parent->addAndMakeVisible(valueLabel);
    
    parent->addAndMakeVisible(progressBar);
}

void PerformanceMonitorView::MetricDisplay::setValue(float value, const juce::String& suffix)
{
    valueLabel.setText(juce::String(value, 1) + suffix, juce::dontSendNotification);
}

void PerformanceMonitorView::MetricDisplay::setProgress(float normalizedValue)
{
    progressValue.store(juce::jlimit(0.0f, 1.0f, normalizedValue));
}

//==============================================================================
// PerformanceMonitorView implementation

PerformanceMonitorView::PerformanceMonitorView()
{
    // Setup metric displays
    m_cpuDisplay.setup("CPU Usage", this);
    m_memoryDisplay.setup("Memory", this);
    m_eventsDisplay.setup("MIDI Events", this);
    m_droppedDisplay.setup("Dropped", this);
    m_bufferDisplay.setup("Buffer Size", this);
    m_latencyDisplay.setup("Latency", this);
    
    // Start update timer
    startTimer(UPDATE_RATE_MS);
    
    setSize(400, 300);
}

PerformanceMonitorView::~PerformanceMonitorView()
{
    stopTimer();
}

void PerformanceMonitorView::setAudioProcessor(HAMAudioProcessor* processor)
{
    m_audioProcessor = processor;
}

void PerformanceMonitorView::setChannelManager(ChannelManager* channelManager)
{
    m_channelManager = channelManager;
}

//==============================================================================
// Component overrides

void PerformanceMonitorView::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Title
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Performance Monitor", getLocalBounds().removeFromTop(30),
               juce::Justification::centred);
    
    // Draw separators between metrics
    g.setColour(juce::Colour(0xff333333));
    int y = 30;
    for (int i = 0; i < 6; ++i)
    {
        y += 40;
        g.drawLine(10, y, getWidth() - 10, y, 0.5f);
    }
}

void PerformanceMonitorView::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(30); // Title space
    
    auto setupMetric = [&bounds](MetricDisplay& metric)
    {
        auto row = bounds.removeFromTop(40);
        row.reduce(10, 5);
        
        metric.nameLabel.setBounds(row.removeFromLeft(100));
        metric.valueLabel.setBounds(row.removeFromRight(80));
        metric.progressBar.setBounds(row.reduced(5, 8));
    };
    
    setupMetric(m_cpuDisplay);
    setupMetric(m_memoryDisplay);
    setupMetric(m_eventsDisplay);
    setupMetric(m_droppedDisplay);
    setupMetric(m_bufferDisplay);
    setupMetric(m_latencyDisplay);
}

//==============================================================================
// Timer callback

void PerformanceMonitorView::timerCallback()
{
    if (m_audioProcessor)
    {
        // Update CPU usage
        float cpuUsage = m_audioProcessor->getCpuUsage();
        m_cpuDisplay.setValue(cpuUsage, "%");
        m_cpuDisplay.setProgress(cpuUsage / 100.0f);
        
        // Update memory usage
        size_t memoryBytes = m_audioProcessor->getMemoryUsage();
        m_memoryDisplay.valueLabel.setText(formatMemory(memoryBytes), 
                                           juce::dontSendNotification);
        
        // Update dropped messages
        int dropped = m_audioProcessor->getDroppedMessages();
        m_droppedDisplay.setValue(static_cast<float>(dropped), "");
        
        // Update buffer size (this would need to be added to HAMAudioProcessor)
        // For now, show a placeholder
        m_bufferDisplay.valueLabel.setText("512", juce::dontSendNotification);
        
        // Update latency (placeholder)
        m_latencyDisplay.setValue(10.7f, " ms");
    }
    
    if (m_channelManager)
    {
        // Update MIDI events processed
        auto stats = m_channelManager->getPerformanceStats();
        m_eventsDisplay.setValue(static_cast<float>(stats.totalEventsProcessed), "");
        
        // Could also show other channel manager stats
        // e.g., voice stealing, buffer allocations, etc.
    }
    
    // Update progress bars
    m_cpuDisplay.progressBar.setCurrentValue(m_cpuDisplay.progressValue.load());
    m_memoryDisplay.progressBar.setCurrentValue(m_memoryDisplay.progressValue.load());
    m_eventsDisplay.progressBar.setCurrentValue(m_eventsDisplay.progressValue.load());
    m_droppedDisplay.progressBar.setCurrentValue(m_droppedDisplay.progressValue.load());
    m_bufferDisplay.progressBar.setCurrentValue(m_bufferDisplay.progressValue.load());
    m_latencyDisplay.progressBar.setCurrentValue(m_latencyDisplay.progressValue.load());
}

//==============================================================================
// Formatting helpers

juce::String PerformanceMonitorView::formatMemory(size_t bytes)
{
    if (bytes < 1024)
        return juce::String(bytes) + " B";
    else if (bytes < 1024 * 1024)
        return juce::String(bytes / 1024.0, 1) + " KB";
    else
        return juce::String(bytes / (1024.0 * 1024.0), 1) + " MB";
}

juce::String PerformanceMonitorView::formatNumber(int value)
{
    if (value < 1000)
        return juce::String(value);
    else if (value < 1000000)
        return juce::String(value / 1000.0, 1) + "K";
    else
        return juce::String(value / 1000000.0, 1) + "M";
}

} // namespace HAM