/*
  ==============================================================================

    Scale.cpp
    Implementation of the Scale model

  ==============================================================================
*/

#include "Scale.h"
#include <algorithm>

namespace HAM {

//==============================================================================
// Static scale definitions

const Scale Scale::Chromatic("Chromatic", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
const Scale Scale::Major("Major", {0, 2, 4, 5, 7, 9, 11});
const Scale Scale::Minor("Minor", {0, 2, 3, 5, 7, 8, 10});
const Scale Scale::HarmonicMinor("Harmonic Minor", {0, 2, 3, 5, 7, 8, 11});
const Scale Scale::MelodicMinor("Melodic Minor", {0, 2, 3, 5, 7, 9, 11});
const Scale Scale::Dorian("Dorian", {0, 2, 3, 5, 7, 9, 10});
const Scale Scale::Phrygian("Phrygian", {0, 1, 3, 5, 7, 8, 10});
const Scale Scale::Lydian("Lydian", {0, 2, 4, 6, 7, 9, 11});
const Scale Scale::Mixolydian("Mixolydian", {0, 2, 4, 5, 7, 9, 10});
const Scale Scale::Locrian("Locrian", {0, 1, 3, 5, 6, 8, 10});
const Scale Scale::PentatonicMajor("Pentatonic Major", {0, 2, 4, 7, 9});
const Scale Scale::PentatonicMinor("Pentatonic Minor", {0, 3, 5, 7, 10});
const Scale Scale::Blues("Blues", {0, 3, 5, 6, 7, 10});
const Scale Scale::WholeTone("Whole Tone", {0, 2, 4, 6, 8, 10});
const Scale Scale::Diminished("Diminished", {0, 2, 3, 5, 6, 8, 9, 11});
const Scale Scale::Augmented("Augmented", {0, 3, 4, 7, 8, 11});

// Static preset scales map
const std::map<juce::String, Scale> Scale::s_presetScales = Scale::createPresetScales();

//==============================================================================
Scale::Scale()
    : m_name("Chromatic")
    , m_intervals({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11})
{
}

Scale::Scale(const juce::String& name, const std::vector<int>& intervals)
    : m_name(name)
    , m_intervals(intervals)
{
    // Ensure intervals are sorted and within 0-11 range
    std::sort(m_intervals.begin(), m_intervals.end());
    m_intervals.erase(std::unique(m_intervals.begin(), m_intervals.end()), m_intervals.end());
    
    for (auto& interval : m_intervals)
    {
        interval = interval % 12;
    }
}

//==============================================================================
// Scale Properties

void Scale::setIntervals(const std::vector<int>& intervals)
{
    m_intervals = intervals;
    
    // Ensure intervals are sorted and within 0-11 range
    std::sort(m_intervals.begin(), m_intervals.end());
    m_intervals.erase(std::unique(m_intervals.begin(), m_intervals.end()), m_intervals.end());
    
    for (auto& interval : m_intervals)
    {
        interval = interval % 12;
    }
}

bool Scale::isChromatic() const
{
    return m_intervals.size() == 12;
}

//==============================================================================
// Quantization

int Scale::quantize(int midiNote, int rootNote) const
{
    if (isChromatic())
        return midiNote;  // No quantization needed
    
    if (m_intervals.empty())
        return midiNote;
    
    // Get the chromatic pitch relative to root
    int relativePitch = (midiNote - rootNote) % 12;
    if (relativePitch < 0)
        relativePitch += 12;
    
    // Find the octave
    int octave = (midiNote - rootNote) / 12;
    if (midiNote < rootNote && (midiNote - rootNote) % 12 != 0)
        octave--;
    
    // Find nearest scale note
    int nearestInterval = findNearestScaleNote(relativePitch);
    
    // Calculate the quantized MIDI note
    return rootNote + (octave * 12) + nearestInterval;
}

int Scale::getDegree(int midiNote, int rootNote) const
{
    int relativePitch = (midiNote - rootNote) % 12;
    if (relativePitch < 0)
        relativePitch += 12;
    
    auto it = std::find(m_intervals.begin(), m_intervals.end(), relativePitch);
    if (it != m_intervals.end())
    {
        return static_cast<int>(std::distance(m_intervals.begin(), it));
    }
    
    return -1;  // Not in scale
}

int Scale::getNoteForDegree(int degree, int rootNote, int octave) const
{
    if (m_intervals.empty())
        return rootNote + (octave * 12);
    
    // Wrap degree to scale size
    int scaleDegree = degree % static_cast<int>(m_intervals.size());
    int octaveOffset = degree / static_cast<int>(m_intervals.size());
    
    if (scaleDegree < 0)
    {
        scaleDegree += static_cast<int>(m_intervals.size());
        octaveOffset--;
    }
    
    return rootNote + ((octave + octaveOffset) * 12) + m_intervals[scaleDegree];
}

bool Scale::contains(int midiNote, int rootNote) const
{
    if (isChromatic())
        return true;
    
    int relativePitch = (midiNote - rootNote) % 12;
    if (relativePitch < 0)
        relativePitch += 12;
    
    return std::find(m_intervals.begin(), m_intervals.end(), relativePitch) != m_intervals.end();
}

int Scale::findNearestScaleNote(int chromaticPitch) const
{
    if (m_intervals.empty())
        return chromaticPitch;
    
    // Check if already in scale
    if (std::find(m_intervals.begin(), m_intervals.end(), chromaticPitch) != m_intervals.end())
        return chromaticPitch;
    
    // Find nearest scale note - when tied, prefer the higher note (round up)
    int nearest = m_intervals[0];
    int minDistance = 12;
    
    for (int interval : m_intervals)
    {
        int distance = std::abs(interval - chromaticPitch);
        if (distance < minDistance)
        {
            minDistance = distance;
            nearest = interval;
        }
        else if (distance == minDistance && interval > nearest)
        {
            // When tied, prefer the higher note
            nearest = interval;
        }
    }
    
    // Also check wrapped distance (for notes near the octave boundary)
    if (!m_intervals.empty())
    {
        int firstInterval = m_intervals[0] + 12;
        int distance = std::abs(firstInterval - chromaticPitch);
        if (distance < minDistance)
        {
            nearest = m_intervals[0];
        }
        else if (distance == minDistance && m_intervals[0] > nearest)
        {
            // When tied with wrap-around, still prefer higher
            nearest = m_intervals[0];
        }
    }
    
    return nearest;
}

//==============================================================================
// Preset Scales

std::map<juce::String, Scale> Scale::createPresetScales()
{
    std::map<juce::String, Scale> scales;
    
    scales["chromatic"] = Chromatic;
    scales["major"] = Major;
    scales["minor"] = Minor;
    scales["harmonic_minor"] = HarmonicMinor;
    scales["melodic_minor"] = MelodicMinor;
    scales["dorian"] = Dorian;
    scales["phrygian"] = Phrygian;
    scales["lydian"] = Lydian;
    scales["mixolydian"] = Mixolydian;
    scales["locrian"] = Locrian;
    scales["pentatonic_major"] = PentatonicMajor;
    scales["pentatonic_minor"] = PentatonicMinor;
    scales["blues"] = Blues;
    scales["whole_tone"] = WholeTone;
    scales["diminished"] = Diminished;
    scales["augmented"] = Augmented;
    
    return scales;
}

Scale Scale::getPresetScale(const juce::String& scaleId)
{
    auto it = s_presetScales.find(scaleId);
    if (it != s_presetScales.end())
        return it->second;
    
    return Chromatic;  // Default to chromatic
}

juce::StringArray Scale::getPresetScaleIds()
{
    juce::StringArray ids;
    for (const auto& pair : s_presetScales)
    {
        ids.add(pair.first);
    }
    return ids;
}

juce::String Scale::getPresetScaleName(const juce::String& scaleId)
{
    auto it = s_presetScales.find(scaleId);
    if (it != s_presetScales.end())
        return it->second.getName();
    
    return "Unknown";
}

//==============================================================================
// Serialization

juce::ValueTree Scale::toValueTree() const
{
    juce::ValueTree tree("Scale");
    tree.setProperty("name", m_name, nullptr);
    
    juce::String intervalsStr;
    for (size_t i = 0; i < m_intervals.size(); ++i)
    {
        if (i > 0)
            intervalsStr += ",";
        intervalsStr += juce::String(m_intervals[i]);
    }
    tree.setProperty("intervals", intervalsStr, nullptr);
    
    return tree;
}

void Scale::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.hasType("Scale"))
        return;
    
    m_name = tree.getProperty("name", "Chromatic");
    
    m_intervals.clear();
    juce::String intervalsStr = tree.getProperty("intervals", "");
    if (intervalsStr.isNotEmpty())
    {
        juce::StringArray tokens = juce::StringArray::fromTokens(intervalsStr, ",", "");
        for (const auto& token : tokens)
        {
            m_intervals.push_back(token.getIntValue());
        }
    }
    
    if (m_intervals.empty())
    {
        // Default to chromatic if no intervals specified
        m_intervals = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    }
}

juce::String Scale::toString() const
{
    juce::String result = m_name + ":";
    for (size_t i = 0; i < m_intervals.size(); ++i)
    {
        if (i > 0)
            result += ",";
        result += juce::String(m_intervals[i]);
    }
    return result;
}

Scale Scale::fromString(const juce::String& str)
{
    int colonPos = str.indexOf(":");
    if (colonPos < 0)
        return Scale();  // Return chromatic scale
    
    juce::String name = str.substring(0, colonPos);
    juce::String intervalsStr = str.substring(colonPos + 1);
    
    std::vector<int> intervals;
    juce::StringArray tokens = juce::StringArray::fromTokens(intervalsStr, ",", "");
    for (const auto& token : tokens)
    {
        intervals.push_back(token.getIntValue());
    }
    
    return Scale(name, intervals);
}

//==============================================================================
// ScaleManager Implementation

ScaleManager& ScaleManager::getInstance()
{
    static ScaleManager instance;
    return instance;
}

void ScaleManager::addCustomScale(const juce::String& id, const Scale& scale)
{
    m_customScales[id] = scale;
}

void ScaleManager::removeCustomScale(const juce::String& id)
{
    m_customScales.erase(id);
}

Scale ScaleManager::getScale(const juce::String& id) const
{
    // Check custom scales first
    auto customIt = m_customScales.find(id);
    if (customIt != m_customScales.end())
        return customIt->second;
    
    // Fall back to preset scales
    return Scale::getPresetScale(id);
}

bool ScaleManager::hasScale(const juce::String& id) const
{
    return m_customScales.find(id) != m_customScales.end() ||
           Scale::s_presetScales.find(id) != Scale::s_presetScales.end();
}

juce::StringArray ScaleManager::getAllScaleIds() const
{
    juce::StringArray ids = Scale::getPresetScaleIds();
    
    for (const auto& pair : m_customScales)
    {
        ids.add(pair.first);
    }
    
    return ids;
}

juce::StringArray ScaleManager::getCustomScaleIds() const
{
    juce::StringArray ids;
    for (const auto& pair : m_customScales)
    {
        ids.add(pair.first);
    }
    return ids;
}

void ScaleManager::saveCustomScales(const juce::File& file)
{
    juce::ValueTree root("CustomScales");
    
    for (const auto& pair : m_customScales)
    {
        juce::ValueTree scaleTree = pair.second.toValueTree();
        scaleTree.setProperty("id", pair.first, nullptr);
        root.addChild(scaleTree, -1, nullptr);
    }
    
    auto xml = root.createXml();
    if (xml)
    {
        xml->writeTo(file);
    }
}

void ScaleManager::loadCustomScales(const juce::File& file)
{
    if (!file.existsAsFile())
        return;
    
    auto xml = juce::parseXML(file);
    if (!xml)
        return;
    
    auto root = juce::ValueTree::fromXml(*xml);
    if (!root.hasType("CustomScales"))
        return;
    
    m_customScales.clear();
    
    for (int i = 0; i < root.getNumChildren(); ++i)
    {
        auto scaleTree = root.getChild(i);
        juce::String id = scaleTree.getProperty("id", "");
        
        if (id.isNotEmpty())
        {
            Scale scale;
            scale.fromValueTree(scaleTree);
            m_customScales[id] = scale;
        }
    }
}

juce::File ScaleManager::getDefaultCustomScalesFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("HAM")
                        .getChildFile("CustomScales.xml");
    return appDataDir;
}

} // namespace HAM