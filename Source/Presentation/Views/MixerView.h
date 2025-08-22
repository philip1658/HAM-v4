// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>

namespace HAM::UI {

class MixerChannelStrip : public juce::Component
{
public:
    MixerChannelStrip(int trackIndex)
        : m_trackIndex(trackIndex)
    {
        m_label.setText("Track " + juce::String(trackIndex + 1), juce::dontSendNotification);
        m_label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_label);

        m_gainSlider.setRange(0.0, 1.5, 0.001);
        m_gainSlider.setValue(1.0);
        m_gainSlider.setSliderStyle(juce::Slider::LinearVertical);
        addAndMakeVisible(m_gainSlider);

        m_panSlider.setRange(-1.0, 1.0, 0.001);
        m_panSlider.setValue(0.0);
        m_panSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        addAndMakeVisible(m_panSlider);

        m_pluginAlias.setButtonText("Instrument");
        m_pluginAlias.onClick = [this]{ if (onAliasInstrumentPlugin) onAliasInstrumentPlugin(m_trackIndex); };
        addAndMakeVisible(m_pluginAlias);

        m_addFx.setButtonText("+ FX");
        m_addFx.onClick = [this]{ if (onAddFxPlugin) onAddFxPlugin(m_trackIndex); };
        addAndMakeVisible(m_addFx);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(6);
        m_label.setBounds(b.removeFromTop(20));
        m_gainSlider.setBounds(b.removeFromTop(b.getHeight() - 80));
        auto bottom = b;
        m_panSlider.setBounds(bottom.removeFromLeft(60).reduced(4));
        m_pluginAlias.setBounds(bottom.removeFromLeft(bottom.getWidth()/2).reduced(2));
        m_addFx.setBounds(bottom.reduced(2));
    }

    std::function<void(int)> onAliasInstrumentPlugin;
    std::function<void(int)> onAddFxPlugin;

private:
    int m_trackIndex;
    juce::Label m_label;
    juce::Slider m_gainSlider, m_panSlider;
    juce::TextButton m_pluginAlias, m_addFx;
};

class MixerView : public juce::Component
{
public:
    MixerView()
    {
        m_viewport.setViewedComponent(&m_container, false);
        addAndMakeVisible(m_viewport);
        setTrackCount(1);
    }

    void setTrackCount(int count)
    {
        m_container.removeAllChildren();
        m_strips.clear();
        count = std::max(1, count);
        for (int i = 0; i < count; ++i)
        {
            auto* strip = m_strips.add(new MixerChannelStrip(i));
            strip->onAliasInstrumentPlugin = [this](int t){ if (onAliasInstrumentPlugin) onAliasInstrumentPlugin(t); };
            strip->onAddFxPlugin = [this](int t){ if (onAddFxPlugin) onAddFxPlugin(t); };
            m_container.addAndMakeVisible(strip);
        }
        resized();
    }

    void resized() override
    {
        auto b = getLocalBounds();
        m_viewport.setBounds(b);
        int widthPer = 140;
        int gap = 8;
        int x = 8;
        int maxH = 0;
        for (auto* strip : m_strips)
        {
            if (strip)
            {
                strip->setBounds(x, 8, widthPer, std::max(280, b.getHeight() - 16));
                x += widthPer + gap;
                maxH = std::max(maxH, strip->getBottom());
            }
        }
        m_container.setSize(std::max(b.getWidth(), x + 8), std::max(b.getHeight(), maxH + 8));
    }

    std::function<void(int)> onAliasInstrumentPlugin;
    std::function<void(int)> onAddFxPlugin;

private:
    juce::Viewport m_viewport;
    juce::Component m_container;
    juce::OwnedArray<MixerChannelStrip> m_strips;
};

} // namespace HAM::UI


