// SPDX-License-Identifier: MIT
#pragma once

#include "HAMComponentLibrary.h"
#include <vector>
#include <array>

namespace HAM::UI {

// ==========================================
// Transport Controls
// ==========================================

class PlayButton : public ResizableComponent, private juce::Timer {
public:
    PlayButton() {
        setInterceptsMouseClicks(true, false);
        startTimerHz(30);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(4));
        auto center = bounds.getCentre();
        
        // Pulsing animation when playing
        if (m_isPlaying) {
            float pulse = 0.5f + 0.5f * std::sin(m_animPhase);
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_GREEN).withAlpha(pulse * 0.3f));
            g.fillEllipse(bounds.expanded(scaled(4)));
        }
        
        // Button background
        g.setColour(m_isPlaying 
                   ? juce::Colour(DesignTokens::Colors::ACCENT_GREEN)
                   : juce::Colour(DesignTokens::Colors::BG_RAISED));
        g.fillEllipse(bounds);
        
        // Border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawEllipse(bounds, scaled(1));
        
        // Icon
        g.setColour(juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        if (m_isPlaying) {
            // Pause icon
            float w = scaled(4);
            float h = scaled(12);
            float gap = scaled(3);
            g.fillRect(center.x - gap - w, center.y - h/2, w, h);
            g.fillRect(center.x + gap, center.y - h/2, w, h);
        } else {
            // Play triangle
            juce::Path triangle;
            float size = scaled(10);
            triangle.addTriangle(center.x - size/2, center.y - size,
                                center.x - size/2, center.y + size,
                                center.x + size, center.y);
            g.fillPath(triangle);
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        m_isPlaying = !m_isPlaying;
        repaint();
        if (onPlayStateChanged) onPlayStateChanged(m_isPlaying);
    }
    
    void timerCallback() override {
        if (m_isPlaying) {
            m_animPhase += 0.1f;
            repaint();
        }
    }
    
    void setPlaying(bool playing) {
        m_isPlaying = playing;
        repaint();
    }
    
    std::function<void(bool)> onPlayStateChanged;
    
private:
    bool m_isPlaying = false;
    float m_animPhase = 0;
};

class StopButton : public ResizableComponent {
public:
    StopButton() {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(4));
        
        // Shadow effect
        g.setColour(juce::Colour(0x40000000));
        g.fillEllipse(bounds.translated(0, scaled(2)));
        
        // Button background
        g.setColour(m_isDown 
                   ? juce::Colour(DesignTokens::Colors::ACCENT_RED).darker(0.2f)
                   : juce::Colour(DesignTokens::Colors::BG_RAISED));
        g.fillEllipse(bounds);
        
        // Border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawEllipse(bounds, scaled(1));
        
        // Stop square
        auto squareSize = scaled(12);
        auto square = juce::Rectangle<float>(squareSize, squareSize).withCentre(bounds.getCentre());
        g.setColour(juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        g.fillRect(square);
    }
    
    void mouseDown(const juce::MouseEvent&) override {
        m_isDown = true;
        repaint();
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        m_isDown = false;
        repaint();
        if (onStop) onStop();
    }
    
    std::function<void()> onStop;
    
private:
    bool m_isDown = false;
};

class RecordButton : public ResizableComponent, private juce::Timer {
public:
    RecordButton() {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(4));
        
        // Blinking animation when recording
        if (m_isRecording) {
            float alpha = m_blinkState ? 0.8f : 0.3f;
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_RED).withAlpha(alpha));
        } else {
            g.setColour(juce::Colour(DesignTokens::Colors::BG_RAISED));
        }
        
        g.fillEllipse(bounds);
        
        // Border
        g.setColour(m_isRecording 
                   ? juce::Colour(DesignTokens::Colors::ACCENT_RED)
                   : juce::Colour(DesignTokens::Colors::BORDER));
        g.drawEllipse(bounds, scaled(1));
        
        // Inner circle
        if (!m_isRecording) {
            auto innerCircle = bounds.reduced(bounds.getWidth() * 0.3f);
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_RED));
            g.fillEllipse(innerCircle);
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        m_isRecording = !m_isRecording;
        if (m_isRecording) {
            startTimerHz(2); // Blink twice per second
        } else {
            stopTimer();
            m_blinkState = false;
        }
        repaint();
        if (onRecordStateChanged) onRecordStateChanged(m_isRecording);
    }
    
    void timerCallback() override {
        m_blinkState = !m_blinkState;
        repaint();
    }
    
    std::function<void(bool)> onRecordStateChanged;
    
private:
    bool m_isRecording = false;
    bool m_blinkState = false;
};

class TempoDisplay : public ResizableComponent, private juce::Timer {
public:
    TempoDisplay() {
        startTimerHz(10);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED));
        g.fillRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        // Border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS), scaled(1));
        
        // BPM value
        g.setColour(juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        g.setFont(juce::Font(juce::FontOptions(scaled(24))).withStyle(juce::Font::bold));
        
        auto textBounds = bounds.reduced(scaled(4));
        g.drawText(juce::String(m_bpm, 1), textBounds, juce::Justification::centred);
        
        // BPM label
        g.setFont(juce::Font(juce::FontOptions(scaled(10))));
        g.setColour(juce::Colour(DesignTokens::Colors::TEXT_MUTED));
        g.drawText("BPM", textBounds, juce::Justification::centredBottom);
        
        // Tap indicator
        if (m_tapFlash > 0) {
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_BLUE).withAlpha(m_tapFlash));
            g.drawRoundedRectangle(bounds.reduced(scaled(2)), 
                                  scaled(DesignTokens::Dimensions::CORNER_RADIUS), scaled(2));
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isLeftButtonDown()) {
            // Tap tempo
            auto now = juce::Time::currentTimeMillis();
            if (m_lastTapTime > 0) {
                auto interval = now - m_lastTapTime;
                if (interval < 2000) { // Max 2 seconds between taps
                    float newBpm = 60000.0f / interval;
                    setBPM(juce::jlimit(20.0f, 999.0f, newBpm));
                    m_tapFlash = 1.0f;
                }
            }
            m_lastTapTime = now;
        }
    }
    
    void timerCallback() override {
        if (m_tapFlash > 0) {
            m_tapFlash -= 0.05f;
            repaint();
        }
    }
    
    void setBPM(float bpm) {
        m_bpm = bpm;
        repaint();
        if (onBPMChanged) onBPMChanged(m_bpm);
    }
    
    float getBPM() const { return m_bpm; }
    
    std::function<void(float)> onBPMChanged;
    
private:
    float m_bpm = 120.0f;
    float m_tapFlash = 0;
    juce::int64 m_lastTapTime = 0;
};

// ==========================================
// Pattern/Sequencer Components
// ==========================================

class GatePatternEditor : public ResizableComponent {
public:
    GatePatternEditor(int steps = 8) : m_steps(steps) {
        m_pattern.resize(steps, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        float stepWidth = bounds.getWidth() / m_steps;
        
        for (int i = 0; i < m_steps; ++i) {
            auto stepBounds = juce::Rectangle<float>(i * stepWidth, 0, stepWidth - scaled(2), bounds.getHeight());
            
            // Step background
            g.setColour(m_pattern[i] 
                       ? juce::Colour(DesignTokens::Colors::ACCENT_BLUE)
                       : juce::Colour(DesignTokens::Colors::BG_RECESSED));
            g.fillRoundedRectangle(stepBounds, scaled(2));
            
            // Border
            g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
            g.drawRoundedRectangle(stepBounds, scaled(2), scaled(1));
            
            // Step number
            g.setColour(juce::Colour(DesignTokens::Colors::TEXT_MUTED));
            g.setFont(juce::Font(juce::FontOptions(scaled(10))));
            g.drawText(juce::String(i + 1), stepBounds, juce::Justification::centred);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        int step = (e.position.x / getWidth()) * m_steps;
        if (step >= 0 && step < m_steps) {
            m_pattern[step] = !m_pattern[step];
            repaint();
            if (onPatternChanged) onPatternChanged(m_pattern);
        }
    }
    
    void setPattern(const std::vector<bool>& pattern) {
        m_pattern = pattern;
        repaint();
    }
    
    std::function<void(const std::vector<bool>&)> onPatternChanged;
    
private:
    int m_steps;
    std::vector<bool> m_pattern;
};

class RatchetPatternDisplay : public ResizableComponent {
public:
    RatchetPatternDisplay() {}
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED));
        g.fillRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        if (m_ratchets.empty()) return;
        
        float stepWidth = bounds.getWidth() / m_ratchets.size();
        
        for (size_t i = 0; i < m_ratchets.size(); ++i) {
            float x = i * stepWidth;
            int ratchetCount = m_ratchets[i];
            
            if (ratchetCount > 0) {
                float ratchetHeight = bounds.getHeight() / 8.0f;
                float ratchetSpacing = ratchetHeight * 0.2f;
                
                for (int r = 0; r < ratchetCount; ++r) {
                    float y = bounds.getBottom() - (r + 1) * (ratchetHeight + ratchetSpacing);
                    auto ratchetBounds = juce::Rectangle<float>(x + scaled(2), y, 
                                                                stepWidth - scaled(4), ratchetHeight);
                    
                    // Gradient fill
                    float intensity = 1.0f - (r * 0.1f);
                    g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_CYAN).withAlpha(intensity));
                    g.fillRoundedRectangle(ratchetBounds, scaled(1));
                }
            }
        }
    }
    
    void setRatchets(const std::vector<int>& ratchets) {
        m_ratchets = ratchets;
        repaint();
    }
    
private:
    std::vector<int> m_ratchets;
};

class VelocityCurveEditor : public ResizableComponent {
public:
    VelocityCurveEditor(int points = 8) : m_points(points) {
        m_velocities.resize(points, 0.5f);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED));
        g.fillRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        // Grid lines
        g.setColour(juce::Colour(DesignTokens::Colors::GRID_LINE));
        for (int i = 1; i < 4; ++i) {
            float y = bounds.getHeight() * i / 4.0f;
            g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
        }
        
        // Velocity curve
        juce::Path curve;
        float stepWidth = bounds.getWidth() / (m_points - 1);
        
        for (int i = 0; i < m_points; ++i) {
            float x = bounds.getX() + i * stepWidth;
            float y = bounds.getBottom() - (m_velocities[i] * bounds.getHeight());
            
            if (i == 0) {
                curve.startNewSubPath(x, y);
            } else {
                curve.lineTo(x, y);
            }
            
            // Draw point
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_BLUE));
            g.fillEllipse(juce::Rectangle<float>(scaled(6), scaled(6)).withCentre({x, y}));
        }
        
        // Draw curve
        g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_BLUE).withAlpha(0.5f));
        g.strokePath(curve, juce::PathStrokeType(scaled(2)));
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        updateVelocity(e.position);
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        updateVelocity(e.position);
    }
    
private:
    int m_points;
    std::vector<float> m_velocities;
    
    void updateVelocity(juce::Point<float> pos) {
        int index = (pos.x / getWidth()) * m_points;
        if (index >= 0 && index < m_points) {
            m_velocities[index] = juce::jlimit(0.0f, 1.0f, 1.0f - (pos.y / getHeight()));
            repaint();
        }
    }
};

// ==========================================
// Scale/Music Components  
// ==========================================

class ScaleSlotPanel : public ResizableComponent {
public:
    ScaleSlotPanel() {
        for (int i = 0; i < 8; ++i) {
            m_slots[i] = false;
        }
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        float slotSize = bounds.getWidth() / 8.0f;
        
        for (int i = 0; i < 8; ++i) {
            auto slotBounds = juce::Rectangle<float>(i * slotSize, 0, slotSize - scaled(2), bounds.getHeight());
            
            // Slot background
            g.setColour(m_slots[i] 
                       ? juce::Colour(DesignTokens::Colors::getTrackColor(i))
                       : juce::Colour(DesignTokens::Colors::BG_RECESSED));
            g.fillRoundedRectangle(slotBounds, scaled(2));
            
            // Slot number
            g.setColour(juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
            g.setFont(juce::Font(juce::FontOptions(scaled(12))));
            g.drawText(juce::String(i + 1), slotBounds, juce::Justification::centred);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        int slot = (e.position.x / getWidth()) * 8;
        if (slot >= 0 && slot < 8) {
            // Clear others if not shift-clicking
            if (!e.mods.isShiftDown()) {
                for (int i = 0; i < 8; ++i) {
                    m_slots[i] = false;
                }
            }
            m_slots[slot] = !m_slots[slot];
            repaint();
            if (onSlotSelected) onSlotSelected(slot, m_slots[slot]);
        }
    }
    
    std::function<void(int, bool)> onSlotSelected;
    
private:
    std::array<bool, 8> m_slots;
};

class ScaleKeyboard : public ResizableComponent {
public:
    ScaleKeyboard() {
        // Initialize scale (C major by default)
        m_scaleNotes = {0, 2, 4, 5, 7, 9, 11}; // C D E F G A B
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Draw 2 octaves
        float whiteKeyWidth = bounds.getWidth() / 14.0f; // 7 white keys per octave
        float blackKeyWidth = whiteKeyWidth * 0.6f;
        float whiteKeyHeight = bounds.getHeight();
        float blackKeyHeight = whiteKeyHeight * 0.6f;
        
        // White keys first
        for (int octave = 0; octave < 2; ++octave) {
            for (int key = 0; key < 7; ++key) {
                float x = (octave * 7 + key) * whiteKeyWidth;
                auto keyBounds = juce::Rectangle<float>(x, 0, whiteKeyWidth - scaled(1), whiteKeyHeight);
                
                int noteNumber = octave * 12 + whiteKeyToNote(key);
                bool inScale = isNoteInScale(noteNumber % 12);
                
                g.setColour(inScale 
                           ? juce::Colour(DesignTokens::Colors::ACCENT_BLUE).withAlpha(0.3f)
                           : juce::Colour(DesignTokens::Colors::BG_RAISED));
                g.fillRect(keyBounds);
                
                g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
                g.drawRect(keyBounds, scaled(1));
            }
        }
        
        // Black keys
        for (int octave = 0; octave < 2; ++octave) {
            for (int key = 0; key < 7; ++key) {
                if (key == 2 || key == 6) continue; // No black key after E and B
                
                float x = (octave * 7 + key) * whiteKeyWidth + whiteKeyWidth - blackKeyWidth/2;
                auto keyBounds = juce::Rectangle<float>(x, 0, blackKeyWidth, blackKeyHeight);
                
                int noteNumber = octave * 12 + whiteKeyToNote(key) + 1;
                bool inScale = isNoteInScale(noteNumber % 12);
                
                g.setColour(inScale 
                           ? juce::Colour(DesignTokens::Colors::ACCENT_BLUE).withAlpha(0.7f)
                           : juce::Colour(DesignTokens::Colors::BG_VOID));
                g.fillRect(keyBounds);
                
                g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
                g.drawRect(keyBounds, scaled(1));
            }
        }
    }
    
    void setScale(const std::vector<int>& scaleNotes) {
        m_scaleNotes = scaleNotes;
        repaint();
    }
    
private:
    std::vector<int> m_scaleNotes;
    
    int whiteKeyToNote(int whiteKey) {
        static const int notes[] = {0, 2, 4, 5, 7, 9, 11}; // C D E F G A B
        return notes[whiteKey % 7];
    }
    
    bool isNoteInScale(int note) {
        return std::find(m_scaleNotes.begin(), m_scaleNotes.end(), note) != m_scaleNotes.end();
    }
};

// ==========================================
// Data Input/Display
// ==========================================

class NumericInput : public ResizableComponent {
public:
    NumericInput(float min = 0, float max = 100, float step = 1) 
        : m_min(min), m_max(max), m_step(step), m_value(min) {}
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(m_isEditing 
                   ? juce::Colour(DesignTokens::Colors::BG_RAISED)
                   : juce::Colour(DesignTokens::Colors::BG_RECESSED));
        g.fillRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        // Border
        g.setColour(m_isEditing 
                   ? juce::Colour(DesignTokens::Colors::ACCENT_BLUE)
                   : juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS), scaled(1));
        
        // Value
        g.setColour(juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        g.setFont(juce::Font(juce::FontOptions(scaled(14))));
        g.drawText(juce::String(m_value, 1), bounds, juce::Justification::centred);
        
        // Up/Down arrows
        float arrowSize = scaled(8);
        float arrowX = bounds.getRight() - scaled(12);
        
        // Up arrow
        juce::Path upArrow;
        upArrow.addTriangle(arrowX, bounds.getCentreY() - scaled(8),
                           arrowX - arrowSize/2, bounds.getCentreY() - scaled(4),
                           arrowX + arrowSize/2, bounds.getCentreY() - scaled(4));
        
        // Down arrow
        juce::Path downArrow;
        downArrow.addTriangle(arrowX, bounds.getCentreY() + scaled(8),
                             arrowX - arrowSize/2, bounds.getCentreY() + scaled(4),
                             arrowX + arrowSize/2, bounds.getCentreY() + scaled(4));
        
        g.setColour(juce::Colour(DesignTokens::Colors::TEXT_MUTED));
        g.fillPath(upArrow);
        g.fillPath(downArrow);
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.position.x > getWidth() * 0.7f) {
            // Click on arrows
            if (e.position.y < getHeight() / 2) {
                setValue(m_value + m_step);
            } else {
                setValue(m_value - m_step);
            }
        } else {
            m_isEditing = true;
            repaint();
        }
    }
    
    void setValue(float value) {
        m_value = juce::jlimit(m_min, m_max, value);
        repaint();
        if (onValueChanged) onValueChanged(m_value);
    }
    
    std::function<void(float)> onValueChanged;
    
private:
    float m_min, m_max, m_step, m_value;
    bool m_isEditing = false;
};

class SegmentedControl : public ResizableComponent {
public:
    SegmentedControl(const juce::StringArray& segments) : m_segments(segments) {}
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        float segmentWidth = bounds.getWidth() / m_segments.size();
        
        // Background
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED));
        g.fillRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        // Segments
        for (int i = 0; i < m_segments.size(); ++i) {
            auto segmentBounds = juce::Rectangle<float>(i * segmentWidth, 0, segmentWidth, bounds.getHeight());
            
            if (i == m_selectedIndex) {
                // Selected segment
                g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_BLUE));
                g.fillRoundedRectangle(segmentBounds.reduced(scaled(2)), scaled(DesignTokens::Dimensions::CORNER_RADIUS));
            }
            
            // Text
            g.setColour(i == m_selectedIndex 
                       ? juce::Colours::white
                       : juce::Colour(DesignTokens::Colors::TEXT_MUTED));
            g.setFont(juce::Font(juce::FontOptions(scaled(12))));
            g.drawText(m_segments[i], segmentBounds, juce::Justification::centred);
            
            // Separator
            if (i < m_segments.size() - 1) {
                g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
                g.drawVerticalLine(segmentBounds.getRight(), 
                                 segmentBounds.getY() + scaled(4), 
                                 segmentBounds.getBottom() - scaled(4));
            }
        }
        
        // Border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS), scaled(1));
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        int index = (e.position.x / getWidth()) * m_segments.size();
        if (index >= 0 && index < m_segments.size()) {
            m_selectedIndex = index;
            repaint();
            if (onSelectionChanged) onSelectionChanged(m_selectedIndex);
        }
    }
    
    std::function<void(int)> onSelectionChanged;
    
private:
    juce::StringArray m_segments;
    int m_selectedIndex = 0;
};

// ==========================================
// Visualization
// ==========================================

class AccumulatorVisualizer : public ResizableComponent, private juce::Timer {
public:
    AccumulatorVisualizer() {
        startTimerHz(30);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED));
        g.fillRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        // Grid
        g.setColour(juce::Colour(DesignTokens::Colors::GRID_LINE));
        for (int i = 1; i < 8; ++i) {
            float x = bounds.getWidth() * i / 8.0f;
            g.drawVerticalLine(x, bounds.getY(), bounds.getBottom());
        }
        for (int i = 1; i < 4; ++i) {
            float y = bounds.getHeight() * i / 4.0f;
            g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
        }
        
        // Trajectory path
        if (!m_trajectory.empty()) {
            juce::Path path;
            for (size_t i = 0; i < m_trajectory.size(); ++i) {
                float x = bounds.getX() + (i / (float)(m_trajectory.size() - 1)) * bounds.getWidth();
                float y = bounds.getBottom() - (m_trajectory[i] * bounds.getHeight());
                
                if (i == 0) {
                    path.startNewSubPath(x, y);
                } else {
                    path.lineTo(x, y);
                }
            }
            
            // Gradient stroke
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_CYAN));
            g.strokePath(path, juce::PathStrokeType(scaled(2)));
            
            // Current position indicator
            if (m_currentPosition >= 0 && m_currentPosition < m_trajectory.size()) {
                float x = bounds.getX() + (m_currentPosition / (float)(m_trajectory.size() - 1)) * bounds.getWidth();
                float y = bounds.getBottom() - (m_trajectory[m_currentPosition] * bounds.getHeight());
                
                // Glow
                g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_CYAN).withAlpha(0.3f));
                g.fillEllipse(juce::Rectangle<float>(scaled(16), scaled(16)).withCentre({x, y}));
                
                // Dot
                g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_CYAN));
                g.fillEllipse(juce::Rectangle<float>(scaled(8), scaled(8)).withCentre({x, y}));
            }
        }
    }
    
    void setTrajectory(const std::vector<float>& trajectory) {
        m_trajectory = trajectory;
        repaint();
    }
    
    void setCurrentPosition(int position) {
        m_currentPosition = position;
        repaint();
    }
    
    void timerCallback() override {
        if (m_animating && !m_trajectory.empty()) {
            m_currentPosition = (m_currentPosition + 1) % m_trajectory.size();
            repaint();
        }
    }
    
    void setAnimating(bool animate) { m_animating = animate; }
    
private:
    std::vector<float> m_trajectory;
    int m_currentPosition = 0;
    bool m_animating = false;
};

// ==========================================
// Additional UI Elements
// ==========================================

class LED : public ResizableComponent, private juce::Timer {
public:
    LED(juce::Colour color = juce::Colour(DesignTokens::Colors::ACCENT_GREEN)) 
        : m_color(color) {
        startTimerHz(30);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(2));
        
        // Glow effect
        if (m_isOn) {
            float glowSize = scaled(4) * (1.0f + 0.2f * std::sin(m_animPhase));
            g.setColour(m_color.withAlpha(0.3f));
            g.fillEllipse(bounds.expanded(glowSize));
        }
        
        // LED body
        g.setColour(m_isOn ? m_color : m_color.withAlpha(0.2f));
        g.fillEllipse(bounds);
        
        // Highlight
        auto highlight = bounds.reduced(bounds.getWidth() * 0.3f).translated(0, -bounds.getHeight() * 0.2f);
        g.setColour(juce::Colours::white.withAlpha(m_isOn ? 0.5f : 0.1f));
        g.fillEllipse(highlight);
    }
    
    void setOn(bool on) {
        m_isOn = on;
        repaint();
    }
    
    void timerCallback() override {
        if (m_isOn) {
            m_animPhase += 0.2f;
            repaint();
        }
    }
    
private:
    juce::Colour m_color;
    bool m_isOn = false;
    float m_animPhase = 0;
};

class ProgressBar : public ResizableComponent {
public:
    ProgressBar() {}
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED));
        g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);
        
        // Progress fill
        if (m_progress > 0) {
            auto fillBounds = bounds.withWidth(bounds.getWidth() * m_progress);
            
            // Gradient fill
            juce::ColourGradient gradient(
                juce::Colour(DesignTokens::Colors::ACCENT_BLUE),
                fillBounds.getTopLeft(),
                juce::Colour(DesignTokens::Colors::ACCENT_CYAN),
                fillBounds.getTopRight(),
                false
            );
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(fillBounds, fillBounds.getHeight() * 0.5f);
        }
        
        // Border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(bounds, bounds.getHeight() * 0.5f, scaled(1));
    }
    
    void setProgress(float progress) {
        m_progress = juce::jlimit(0.0f, 1.0f, progress);
        repaint();
    }
    
private:
    float m_progress = 0;
};

// ==========================================
// New Transport Bar Components
// ==========================================

class PatternButton : public ResizableComponent, private juce::Timer {
public:
    PatternButton(int patternNumber) 
        : m_patternNumber(patternNumber) {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(2));
        
        // Pulsing glow when chaining
        if (m_isChaining) {
            float pulse = 0.3f + 0.2f * std::sin(m_animPhase);
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_CYAN).withAlpha(pulse));
            g.fillRoundedRectangle(bounds.expanded(scaled(3)), scaled(4));
        }
        
        // Button background
        juce::Colour bgColor;
        if (m_isActive) {
            bgColor = juce::Colour(DesignTokens::Colors::ACCENT_GREEN);
        } else if (m_isHovered) {
            bgColor = juce::Colour(DesignTokens::Colors::BG_RAISED).brighter(0.1f);
        } else {
            bgColor = juce::Colour(DesignTokens::Colors::BG_RAISED);
        }
        
        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, scaled(4));
        
        // Border
        g.setColour(m_isActive 
                   ? juce::Colour(DesignTokens::Colors::ACCENT_GREEN).brighter(0.3f)
                   : juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(bounds, scaled(4), scaled(1));
        
        // Pattern number/letter
        g.setColour(m_isActive 
                   ? juce::Colours::black
                   : juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        g.setFont(juce::Font(juce::FontOptions(scaled(16))).withStyle(juce::Font::bold));
        
        juce::String label = m_useLetters 
            ? juce::String::charToString('A' + m_patternNumber - 1)
            : juce::String(m_patternNumber);
        g.drawText(label, bounds, juce::Justification::centred);
        
        // Activity LED in corner
        if (m_hasActivity) {
            auto topRight = bounds.removeFromTop(scaled(8)).removeFromRight(scaled(8));
            auto ledBounds = topRight.reduced(scaled(1));
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_AMBER));
            g.fillEllipse(ledBounds);
        }
    }
    
    void mouseEnter(const juce::MouseEvent&) override {
        m_isHovered = true;
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override {
        m_isHovered = false;
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isShiftDown()) {
            // Chain pattern
            m_isChaining = !m_isChaining;
            if (m_isChaining) {
                startTimerHz(30);
            } else {
                stopTimer();
            }
            if (onPatternChain) onPatternChain(m_patternNumber, m_isChaining);
        } else if (e.mods.isRightButtonDown()) {
            // Show context menu
            if (onPatternMenu) onPatternMenu(m_patternNumber);
        } else {
            // Select pattern
            if (onPatternSelected) onPatternSelected(m_patternNumber);
        }
        repaint();
    }
    
    void timerCallback() override {
        m_animPhase += 0.2f;
        repaint();
    }
    
    void setActive(bool active) {
        m_isActive = active;
        repaint();
    }
    
    void setActivity(bool hasActivity) {
        m_hasActivity = hasActivity;
        repaint();
    }
    
    void setUseLetters(bool useLetters) {
        m_useLetters = useLetters;
        repaint();
    }
    
    std::function<void(int)> onPatternSelected;
    std::function<void(int, bool)> onPatternChain;
    std::function<void(int)> onPatternMenu;
    
private:
    int m_patternNumber;
    bool m_isActive = false;
    bool m_isHovered = false;
    bool m_isChaining = false;
    bool m_hasActivity = false;
    bool m_useLetters = false;
    float m_animPhase = 0;
};

class TempoArrows : public ResizableComponent, private juce::Timer {
public:
    TempoArrows() {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto upBounds = bounds.removeFromTop(bounds.getHeight() * 0.5f).reduced(scaled(2));
        auto downBounds = bounds.reduced(scaled(2));
        
        // Up arrow button
        g.setColour(m_upHovered 
                   ? juce::Colour(DesignTokens::Colors::BG_RAISED).brighter(0.2f)
                   : juce::Colour(DesignTokens::Colors::BG_RAISED));
        g.fillRoundedRectangle(upBounds, scaled(3));
        
        // Down arrow button
        g.setColour(m_downHovered 
                   ? juce::Colour(DesignTokens::Colors::BG_RAISED).brighter(0.2f)
                   : juce::Colour(DesignTokens::Colors::BG_RAISED));
        g.fillRoundedRectangle(downBounds, scaled(3));
        
        // Borders
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(upBounds, scaled(3), scaled(0.5f));
        g.drawRoundedRectangle(downBounds, scaled(3), scaled(0.5f));
        
        // Draw arrows
        g.setColour(juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        
        // Up arrow
        juce::Path upArrow;
        auto upCenter = upBounds.getCentre();
        float arrowSize = scaled(5);
        upArrow.addTriangle(upCenter.x - arrowSize, upCenter.y + arrowSize/2,
                           upCenter.x + arrowSize, upCenter.y + arrowSize/2,
                           upCenter.x, upCenter.y - arrowSize/2);
        g.fillPath(upArrow);
        
        // Down arrow
        juce::Path downArrow;
        auto downCenter = downBounds.getCentre();
        downArrow.addTriangle(downCenter.x - arrowSize, downCenter.y - arrowSize/2,
                             downCenter.x + arrowSize, downCenter.y - arrowSize/2,
                             downCenter.x, downCenter.y + arrowSize/2);
        g.fillPath(downArrow);
    }
    
    void mouseMove(const juce::MouseEvent& e) override {
        auto bounds = getLocalBounds();
        bool inUpper = e.y < bounds.getHeight() / 2;
        
        if (inUpper != m_upHovered || !inUpper != m_downHovered) {
            m_upHovered = inUpper;
            m_downHovered = !inUpper;
            repaint();
        }
    }
    
    void mouseExit(const juce::MouseEvent&) override {
        m_upHovered = false;
        m_downHovered = false;
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        auto bounds = getLocalBounds();
        bool isUp = e.y < bounds.getHeight() / 2;
        
        float increment;
        if (e.mods.isShiftDown()) {
            increment = isUp ? 1.0f : -1.0f;  // Coarse: +/- 1 BPM
        } else if (e.mods.isCommandDown()) {
            increment = isUp ? 10.0f : -10.0f;  // Super coarse: +/- 10 BPM
        } else {
            increment = isUp ? 0.1f : -0.1f;  // Fine: +/- 0.1 BPM
        }
        
        if (onTempoChange) onTempoChange(increment);
        
        // Start repeat timer for held button
        m_isHolding = true;
        m_holdIncrement = increment;
        startTimer(500);  // Initial delay before repeat
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        m_isHolding = false;
        stopTimer();
    }
    
    void timerCallback() override {
        if (m_isHolding) {
            if (onTempoChange) onTempoChange(m_holdIncrement);
            startTimer(50);  // Faster repeat rate
        }
    }
    
    std::function<void(float)> onTempoChange;
    
private:
    bool m_upHovered = false;
    bool m_downHovered = false;
    bool m_isHolding = false;
    float m_holdIncrement = 0;
};

class CompactSwingKnob : public ResizableComponent {
public:
    CompactSwingKnob() {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(4));
        auto center = bounds.getCentre();
        
        // Background circle
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED));
        g.fillEllipse(bounds);
        
        // Value arc
        float startAngle = juce::MathConstants<float>::pi * 0.75f;
        float endAngle = juce::MathConstants<float>::pi * 2.25f;
        float currentAngle = startAngle + (endAngle - startAngle) * m_value;
        
        juce::Path arc;
        arc.addCentredArc(center.x, center.y,
                         bounds.getWidth() * 0.4f, bounds.getHeight() * 0.4f,
                         0, startAngle, currentAngle, true);
        
        g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_AMBER));
        g.strokePath(arc, juce::PathStrokeType(scaled(2)));
        
        // Center dot
        g.setColour(juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        g.fillEllipse(bounds.reduced(bounds.getWidth() * 0.35f));
        
        // Value indicator line
        float indicatorLength = bounds.getWidth() * 0.3f;
        juce::Point<float> indicatorEnd(
            center.x + indicatorLength * std::cos(currentAngle),
            center.y + indicatorLength * std::sin(currentAngle)
        );
        
        g.drawLine(center.x, center.y, indicatorEnd.x, indicatorEnd.y, scaled(2));
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        m_dragStart = e.position;
        m_dragStartValue = m_value;
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        float deltaY = m_dragStart.y - e.position.y;
        float newValue = m_dragStartValue + (deltaY / 100.0f);
        setValue(juce::jlimit(0.0f, 1.0f, newValue));
        
        if (onValueChange) onValueChange(m_value);
    }
    
    void setValue(float value) {
        m_value = juce::jlimit(0.0f, 1.0f, value);
        repaint();
    }
    
    std::function<void(float)> onValueChange;
    
private:
    float m_value = 0.5f;  // 0.5 = no swing
    juce::Point<float> m_dragStart;
    float m_dragStartValue = 0.5f;
};

class PanicButton : public ResizableComponent {
public:
    PanicButton() {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(2));
        
        // Red alert background when pressed
        g.setColour(m_isPressed 
                   ? juce::Colour(DesignTokens::Colors::ACCENT_RED)
                   : juce::Colour(DesignTokens::Colors::BG_RAISED));
        g.fillRoundedRectangle(bounds, scaled(4));
        
        // Border
        g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_RED));
        g.drawRoundedRectangle(bounds, scaled(4), scaled(m_isPressed ? 2 : 1));
        
        // Text
        g.setColour(m_isPressed 
                   ? juce::Colours::white
                   : juce::Colour(DesignTokens::Colors::ACCENT_RED));
        g.setFont(juce::Font(juce::FontOptions(scaled(11))).withStyle(juce::Font::bold));
        g.drawText("PANIC", bounds, juce::Justification::centred);
    }
    
    void mouseDown(const juce::MouseEvent&) override {
        m_isPressed = true;
        repaint();
        if (onPanic) onPanic();
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        m_isPressed = false;
        repaint();
    }
    
    std::function<void()> onPanic;
    
private:
    bool m_isPressed = false;
};

// ==========================================
// Pattern Management Buttons
// ==========================================

class PatternManagementButton : public ResizableComponent {
public:
    enum class ButtonType {
        Save, Load, Copy, Paste, New, Delete
    };
    
    PatternManagementButton(ButtonType type) : m_type(type) {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(1));
        
        // Different colors for different operations
        juce::Colour bgColor, textColor;
        switch (m_type) {
            case ButtonType::Save:
            case ButtonType::Load:
                bgColor = m_isHovered ? juce::Colour(DesignTokens::Colors::ACCENT_BLUE).brighter(0.2f) 
                                      : juce::Colour(DesignTokens::Colors::BG_RAISED);
                textColor = m_isHovered ? juce::Colours::white 
                                        : juce::Colour(DesignTokens::Colors::ACCENT_BLUE);
                break;
            case ButtonType::Copy:
            case ButtonType::Paste:
                bgColor = m_isHovered ? juce::Colour(DesignTokens::Colors::ACCENT_CYAN).brighter(0.2f)
                                      : juce::Colour(DesignTokens::Colors::BG_RAISED);
                textColor = m_isHovered ? juce::Colours::black
                                        : juce::Colour(DesignTokens::Colors::ACCENT_CYAN);
                break;
            case ButtonType::New:
                bgColor = m_isHovered ? juce::Colour(DesignTokens::Colors::ACCENT_GREEN).brighter(0.2f)
                                      : juce::Colour(DesignTokens::Colors::BG_RAISED);
                textColor = m_isHovered ? juce::Colours::black
                                        : juce::Colour(DesignTokens::Colors::ACCENT_GREEN);
                break;
            case ButtonType::Delete:
                bgColor = m_isHovered ? juce::Colour(DesignTokens::Colors::ACCENT_RED).brighter(0.2f)
                                      : juce::Colour(DesignTokens::Colors::BG_RAISED);
                textColor = m_isHovered ? juce::Colours::white
                                        : juce::Colour(DesignTokens::Colors::ACCENT_RED);
                break;
        }
        
        // Button background
        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, scaled(3));
        
        // Border
        g.setColour(textColor.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds, scaled(3), scaled(0.5f));
        
        // Icon or text
        g.setColour(textColor);
        g.setFont(juce::Font(juce::FontOptions(scaled(10))));
        
        juce::String label;
        switch (m_type) {
            case ButtonType::Save:   label = "SAVE"; break;
            case ButtonType::Load:   label = "LOAD"; break;
            case ButtonType::Copy:   label = "COPY"; break;
            case ButtonType::Paste:  label = "PASTE"; break;
            case ButtonType::New:    label = "NEW"; break;
            case ButtonType::Delete: label = "DEL"; break;
        }
        
        g.drawText(label, bounds, juce::Justification::centred);
    }
    
    void mouseEnter(const juce::MouseEvent&) override {
        m_isHovered = true;
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override {
        m_isHovered = false;
        repaint();
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        if (onClick) onClick();
    }
    
    std::function<void()> onClick;
    
private:
    ButtonType m_type;
    bool m_isHovered = false;
};

// Enhanced transport buttons with visual hierarchy
class LargeTransportButton : public ResizableComponent, private juce::Timer {
public:
    enum class ButtonType {
        Play, Stop, Record
    };
    
    LargeTransportButton(ButtonType type) : m_type(type) {
        setInterceptsMouseClicks(true, false);
        if (type == ButtonType::Play || type == ButtonType::Record) {
            startTimerHz(30);
        }
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(4));
        auto center = bounds.getCentre();
        
        // Larger size for primary controls
        float scale = (m_type == ButtonType::Stop) ? 1.0f : 1.2f;
        bounds = bounds.withSizeKeepingCentre(bounds.getWidth() * scale, bounds.getHeight() * scale);
        
        // Glow effect for active states
        if ((m_type == ButtonType::Play && m_isPlaying) || 
            (m_type == ButtonType::Record && m_isRecording)) {
            float pulse = 0.5f + 0.5f * std::sin(m_animPhase);
            juce::Colour glowColor = (m_type == ButtonType::Play) 
                ? juce::Colour(DesignTokens::Colors::ACCENT_GREEN)
                : juce::Colour(DesignTokens::Colors::ACCENT_RED);
            g.setColour(glowColor.withAlpha(pulse * 0.3f));
            g.fillEllipse(bounds.expanded(scaled(6)));
        }
        
        // Button background with color coding
        juce::Colour bgColor;
        switch (m_type) {
            case ButtonType::Play:
                bgColor = m_isPlaying ? juce::Colour(DesignTokens::Colors::ACCENT_GREEN)
                                      : juce::Colour(DesignTokens::Colors::BG_RAISED);
                break;
            case ButtonType::Stop:
                bgColor = juce::Colour(DesignTokens::Colors::BG_RAISED);
                break;
            case ButtonType::Record:
                bgColor = m_isRecording ? juce::Colour(DesignTokens::Colors::ACCENT_RED)
                                        : juce::Colour(DesignTokens::Colors::BG_RAISED);
                break;
        }
        
        g.setColour(bgColor);
        g.fillEllipse(bounds);
        
        // Stronger border for visual prominence
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawEllipse(bounds, scaled(2));
        
        // Larger icons
        g.setColour(m_isPlaying || m_isRecording ? juce::Colours::black
                                                  : juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        
        switch (m_type) {
            case ButtonType::Play:
                if (m_isPlaying) {
                    // Pause icon (larger)
                    float w = scaled(6);
                    float h = scaled(16);
                    float gap = scaled(4);
                    g.fillRect(center.x - gap - w, center.y - h/2, w, h);
                    g.fillRect(center.x + gap, center.y - h/2, w, h);
                } else {
                    // Play triangle (larger)
                    juce::Path triangle;
                    float size = scaled(14);
                    triangle.addTriangle(center.x - size/2, center.y - size,
                                       center.x - size/2, center.y + size,
                                       center.x + size, center.y);
                    g.fillPath(triangle);
                }
                break;
                
            case ButtonType::Stop:
                {
                    // Stop square (larger)
                    auto squareSize = scaled(16);
                    auto square = juce::Rectangle<float>(squareSize, squareSize)
                                       .withCentre(bounds.getCentre());
                    g.fillRect(square);
                }
                break;
                
            case ButtonType::Record:
                {
                    // Record circle (larger with animation)
                    auto innerCircle = bounds.reduced(bounds.getWidth() * 0.25f);
                    if (m_isRecording) {
                        float pulse = 0.7f + 0.3f * std::sin(m_animPhase * 2);
                        g.setColour(juce::Colours::white.withAlpha(pulse));
                    }
                    g.fillEllipse(innerCircle);
                }
                break;
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        switch (m_type) {
            case ButtonType::Play:
                m_isPlaying = !m_isPlaying;
                if (onPlayStateChanged) onPlayStateChanged(m_isPlaying);
                break;
            case ButtonType::Stop:
                if (onStop) onStop();
                break;
            case ButtonType::Record:
                m_isRecording = !m_isRecording;
                if (onRecordStateChanged) onRecordStateChanged(m_isRecording);
                break;
        }
        repaint();
    }
    
    void timerCallback() override {
        if (m_isPlaying || m_isRecording) {
            m_animPhase += 0.1f;
            repaint();
        }
    }
    
    void setPlaying(bool playing) {
        m_isPlaying = playing;
        repaint();
    }
    
    void setRecording(bool recording) {
        m_isRecording = recording;
        repaint();
    }
    
    std::function<void(bool)> onPlayStateChanged;
    std::function<void()> onStop;
    std::function<void(bool)> onRecordStateChanged;
    
private:
    ButtonType m_type;
    bool m_isPlaying = false;
    bool m_isRecording = false;
    float m_animPhase = 0;
};

} // namespace HAM::UI