// SPDX-License-Identifier: MIT
#include <JuceHeader.h>

int main (int argc, char** argv)
{
    if (argc < 3)
        return 1;

    juce::ScopedJuceInitialiser_GUI init;

    const juce::String formatName = argv[1];
    const juce::String identifier = argv[2];
    const double sampleRate = 44100.0;
    const int blockSize = 512;

    juce::AudioPluginFormatManager fm;
    fm.addDefaultFormats();

    std::unique_ptr<juce::PluginDescription> pd;
    for (int fi = 0; fi < fm.getNumFormats(); ++fi)
    {
        auto* fmt = fm.getFormat(fi);
        if (fmt == nullptr) continue;
        if (!fmt->getName().equalsIgnoreCase(formatName)) continue;

        juce::OwnedArray<juce::PluginDescription> tmp;
        fmt->findAllTypesForFile(tmp, identifier);
        for (auto* t : tmp)
        {
            if (t != nullptr && (t->fileOrIdentifier == identifier || t->name == identifier))
            { pd.reset(new juce::PluginDescription(*t)); break; }
        }
        if (pd != nullptr) break;
    }

    if (pd == nullptr)
        return 1;

    juce::String error;
    std::unique_ptr<juce::AudioPluginInstance> inst (fm.createPluginInstance(*pd, sampleRate, blockSize, error));
    return inst != nullptr ? 0 : 1;
}


