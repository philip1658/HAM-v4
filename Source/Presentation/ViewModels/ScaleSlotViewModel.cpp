/*
  ==============================================================================

    ScaleSlotViewModel.cpp
    Implementation of scale slot view model

  ==============================================================================
*/

#include "ScaleSlotViewModel.h"

namespace HAM {
namespace UI {

//==============================================================================
ScaleSlotViewModel::ScaleSlotViewModel()
{
    // Register as listener to ScaleSlotManager
    ScaleSlotManager::getInstance().addListener(this);
    
    // Initialize cache
    updateAllSlotInfo();
}

ScaleSlotViewModel::~ScaleSlotViewModel()
{
    // Stop auto-progression if active
    stopAutoProgression();
    
    // Unregister from ScaleSlotManager
    ScaleSlotManager::getInstance().removeListener(this);
}

//==============================================================================
// UI → Backend: User actions

void ScaleSlotViewModel::selectSlot(int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        ScaleSlotManager::getInstance().selectSlot(slotIndex);
        m_pendingSlotIndex = slotIndex;
        sendChangeMessage();
    }
}

void ScaleSlotViewModel::loadScaleIntoSlot(int slotIndex, const Scale& scale, const juce::String& name)
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        ScaleSlotManager::getInstance().setSlot(slotIndex, scale, name);
        updateSlotInfo(slotIndex);
        sendChangeMessage();
    }
}

void ScaleSlotViewModel::clearSlot(int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        // Load an empty scale
        Scale emptyScale;
        ScaleSlotManager::getInstance().setSlot(slotIndex, emptyScale, "Empty");
        updateSlotInfo(slotIndex);
        sendChangeMessage();
    }
}

void ScaleSlotViewModel::setGlobalRoot(int rootNote)
{
    ScaleSlotManager::getInstance().setGlobalRoot(rootNote);
    m_globalRoot = rootNote;
    sendChangeMessage();
}

void ScaleSlotViewModel::selectNextSlot()
{
    int currentSlot = getActiveSlotIndex();
    int nextSlot = findNextNonEmptySlot(currentSlot);
    if (nextSlot != currentSlot)
    {
        selectSlot(nextSlot);
    }
}

void ScaleSlotViewModel::selectPreviousSlot()
{
    int currentSlot = getActiveSlotIndex();
    // Search backwards for non-empty slot
    for (int i = 1; i <= 8; ++i)
    {
        int prevSlot = (currentSlot - i + 8) % 8;
        if (!m_slotInfoCache[prevSlot].isEmpty)
        {
            selectSlot(prevSlot);
            break;
        }
    }
}

void ScaleSlotViewModel::setAutoMode(AutoMode mode)
{
    m_autoMode = mode;
    
    // Calculate bars per slot based on mode
    switch (mode)
    {
        case AutoMode::OFF:
            m_barsPerSlot = 0;
            break;
        case AutoMode::QUARTER_BAR:
            m_barsPerSlot = 0; // Special case - handled in timer
            break;
        case AutoMode::ONE_BAR:
            m_barsPerSlot = 1;
            break;
        case AutoMode::TWO_BARS:
            m_barsPerSlot = 2;
            break;
        case AutoMode::FOUR_BARS:
            m_barsPerSlot = 4;
            break;
        case AutoMode::EIGHT_BARS:
            m_barsPerSlot = 8;
            break;
        case AutoMode::SIXTEEN_BARS:
            m_barsPerSlot = 16;
            break;
    }
    
    // Restart timer if auto-progression is active
    if (m_autoProgressionActive && mode != AutoMode::OFF)
    {
        startTimerHz(4); // 4Hz for smooth progress updates
    }
}

void ScaleSlotViewModel::startAutoProgression()
{
    if (m_autoMode != AutoMode::OFF)
    {
        m_autoProgressionActive = true;
        m_autoBarCounter = 0;
        m_autoProgress = 0.0f;
        startTimerHz(4); // 4Hz for smooth visual updates
        sendChangeMessage();
    }
}

void ScaleSlotViewModel::stopAutoProgression()
{
    m_autoProgressionActive = false;
    m_autoProgress = 0.0f;
    stopTimer();
    sendChangeMessage();
}

//==============================================================================
// Backend → UI: State queries

std::array<ScaleSlotViewModel::SlotInfo, 8> ScaleSlotViewModel::getAllSlotInfo() const
{
    if (!m_cacheValid)
    {
        const_cast<ScaleSlotViewModel*>(this)->updateAllSlotInfo();
    }
    return m_slotInfoCache;
}

ScaleSlotViewModel::SlotInfo ScaleSlotViewModel::getSlotInfo(int slotIndex) const
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        if (!m_cacheValid)
        {
            const_cast<ScaleSlotViewModel*>(this)->updateAllSlotInfo();
        }
        return m_slotInfoCache[slotIndex];
    }
    return SlotInfo();
}

int ScaleSlotViewModel::getActiveSlotIndex() const
{
    return ScaleSlotManager::getInstance().getActiveSlotIndex();
}

int ScaleSlotViewModel::getPendingSlotIndex() const
{
    if (ScaleSlotManager::getInstance().hasPendingChange())
    {
        return ScaleSlotManager::getInstance().getPendingSlot();
    }
    return -1;
}

int ScaleSlotViewModel::getGlobalRoot() const
{
    return ScaleSlotManager::getInstance().getGlobalRoot();
}

//==============================================================================
// ScaleSlotManager::Listener implementation

void ScaleSlotViewModel::scaleSlotSelected(int slotIndex)
{
    m_activeSlotIndex = slotIndex;
    m_pendingSlotIndex = -1; // Clear pending when change is applied
    updateSlotInfo(slotIndex);
    sendChangeMessage();
}

void ScaleSlotViewModel::scaleChanged(int slotIndex)
{
    updateSlotInfo(slotIndex);
    sendChangeMessage();
}

void ScaleSlotViewModel::globalRootChanged(int rootNote)
{
    m_globalRoot = rootNote;
    sendChangeMessage();
}

//==============================================================================
// Timer callback for auto-progression

void ScaleSlotViewModel::timerCallback()
{
    if (!m_autoProgressionActive || m_autoMode == AutoMode::OFF)
    {
        stopTimer();
        return;
    }
    
    // Calculate progress based on current bar position
    // This is simplified - in a real implementation, we'd sync with MasterClock
    m_autoProgress += 0.25f / static_cast<float>(m_barsPerSlot > 0 ? m_barsPerSlot : 1);
    
    if (m_autoProgress >= 1.0f)
    {
        // Time to switch to next slot
        m_autoProgress = 0.0f;
        selectNextSlot();
    }
    
    sendChangeMessage(); // Update UI with new progress
}

//==============================================================================
// Private implementation

void ScaleSlotViewModel::updateSlotInfo(int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        auto& manager = ScaleSlotManager::getInstance();
        const ScaleSlot& slot = manager.getSlot(slotIndex);
        
        auto& info = m_slotInfoCache[slotIndex];
        info.isEmpty = slot.scale.getIntervals().empty();
        info.isActive = (slotIndex == manager.getActiveSlotIndex());
        info.isPending = (slotIndex == manager.getPendingSlot());
        info.displayName = slot.displayName;
        info.scaleType = slot.scale.getName();
        info.rootNote = slot.rootNote >= 0 ? slot.rootNote : manager.getGlobalRoot();
    }
}

void ScaleSlotViewModel::updateAllSlotInfo()
{
    auto& manager = ScaleSlotManager::getInstance();
    int activeIndex = manager.getActiveSlotIndex();
    int pendingIndex = manager.getPendingSlot();
    
    for (int i = 0; i < 8; ++i)
    {
        const ScaleSlot& slot = manager.getSlot(i);
        auto& info = m_slotInfoCache[i];
        
        info.isEmpty = slot.scale.getIntervals().empty();
        info.isActive = (i == activeIndex);
        info.isPending = (i == pendingIndex);
        info.displayName = slot.displayName;
        info.scaleType = slot.scale.getName();
        info.rootNote = slot.rootNote >= 0 ? slot.rootNote : manager.getGlobalRoot();
    }
    
    m_cacheValid = true;
}

int ScaleSlotViewModel::findNextNonEmptySlot(int currentSlot) const
{
    // Search forward for non-empty slot
    for (int i = 1; i <= 8; ++i)
    {
        int nextSlot = (currentSlot + i) % 8;
        if (!m_slotInfoCache[nextSlot].isEmpty)
        {
            return nextSlot;
        }
    }
    return currentSlot; // No non-empty slots found
}

int ScaleSlotViewModel::calculateTimerInterval() const
{
    // This would normally sync with MasterClock BPM
    // For now, return a fixed interval
    switch (m_autoMode)
    {
        case AutoMode::QUARTER_BAR:
            return 125; // 8Hz for quarter bar at 120 BPM
        case AutoMode::ONE_BAR:
            return 500; // 2Hz for one bar
        case AutoMode::TWO_BARS:
            return 1000; // 1Hz for two bars
        case AutoMode::FOUR_BARS:
            return 2000; // 0.5Hz for four bars
        case AutoMode::EIGHT_BARS:
            return 4000; // 0.25Hz for eight bars
        case AutoMode::SIXTEEN_BARS:
            return 8000; // 0.125Hz for sixteen bars
        default:
            return 1000;
    }
}

} // namespace UI
} // namespace HAM