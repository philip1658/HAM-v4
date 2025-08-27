/*
  ==============================================================================

    StageGrid.cpp
    Implementation for the StageGrid component

  ==============================================================================
*/

#include "StageGrid.h"
#include "../../Infrastructure/Audio/HAMAudioProcessor.h"

namespace HAM::UI {

void StageGrid::timerCallback() 
{
    if (m_processor && m_processor->isPlaying()) {
        m_isPlaying = true;
        
        // Calculate current stage from beat and pulse
        int beat = m_processor->getCurrentBeat();
        int pulse = m_processor->getCurrentPulse();
        
        // Simple mapping: each stage gets 12 pulses (half beat)
        // This gives us 8 stages across 4 beats
        int stageIndex = (beat * 2) + (pulse / 12);
        
        if (stageIndex != m_currentStageIndex) {
            m_currentStageIndex = stageIndex % 8;
            
            // Highlight the active stage card
            for (int i = 0; i < static_cast<int>(m_stageCards.size()); ++i) {
                int stage = i % 8;
                if (m_stageCards[i]) {
                    m_stageCards[i]->setAlpha(stage == m_currentStageIndex ? 1.0f : 0.7f);
                }
            }
            
            repaint();
        }
    } else if (m_isPlaying) {
        m_isPlaying = false;
        m_currentStageIndex = -1;
        
        // Reset all cards to full opacity
        for (auto& card : m_stageCards) {
            if (card) card->setAlpha(1.0f);
        }
        
        repaint();
    }
}

} // namespace HAM::UI