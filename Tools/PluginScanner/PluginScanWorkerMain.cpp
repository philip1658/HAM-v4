// SPDX-License-Identifier: MIT
#include <JuceHeader.h>

static juce::File getSettingsDir()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("CloneHAM");
    if (!dir.isDirectory()) dir.createDirectory();
    return dir;
}

static juce::File getPluginsXmlFile()    { return getSettingsDir().getChildFile("Plugins.xml"); }
static juce::File getDeadMansPedalFile() { return getSettingsDir().getChildFile("DeadMansPedal.txt"); }
static juce::File getScanStatusFile()    { return getSettingsDir().getChildFile("ScanStatus.json"); }

struct Progress
{
    juce::String current;
    int scanned = 0;
    int total   = 0;
    bool isScanning = true;

    juce::var toVar() const
    {
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty("current", current);
        o->setProperty("scanned", scanned);
        o->setProperty("total", total);
        o->setProperty("isScanning", isScanning);
        return o.get();
    }
};

static void writeStatus(const Progress& p)
{
    getScanStatusFile().replaceWithText(juce::JSON::toString(p.toVar()));
}

int main (int, char**)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::AudioPluginFormatManager fm;
    fm.addDefaultFormats();

    juce::KnownPluginList list;

    if (auto f = getPluginsXmlFile(); f.existsAsFile())
    {
        juce::XmlDocument doc(f);
        if (auto xml = doc.getDocumentElement())
            list.recreateFromXml(*xml);
    }

    auto dmp = getDeadMansPedalFile();

    Progress prog;
    // estimate total for VST3
    for (int fi = 0; fi < fm.getNumFormats(); ++fi)
        if (auto* fmt = fm.getFormat(fi))
            if (fmt->getName().containsIgnoreCase("vst3"))
            {
                auto paths = fmt->getDefaultLocationsToSearch();
                for (int pi = 0; pi < paths.getNumPaths(); ++pi)
                {
                    juce::Array<juce::File> found;
                    paths[pi].findChildFiles(found, juce::File::findDirectories, true, "*.vst3");
                    prog.total += found.size();
                }
            }
    writeStatus(prog);

    for (int fi = 0; fi < fm.getNumFormats(); ++fi)
    {
        auto* fmt = fm.getFormat(fi);
        if (fmt == nullptr) continue;

        auto paths = fmt->getDefaultLocationsToSearch();

        std::unique_ptr<juce::PluginDirectoryScanner> scanner =
            std::make_unique<juce::PluginDirectoryScanner>(
                list, *fmt, paths, true, dmp, false);

        juce::String pluginName;
        bool finished = false;

        while (!finished)
        {
            finished = !scanner->scanNextFile(true, pluginName);

            prog.current = pluginName;
            ++prog.scanned;
            writeStatus(prog);

            if (auto xml = list.createXml())
                xml->writeTo(getPluginsXmlFile());
        }
    }

    if (auto xml = list.createXml())
        xml->writeTo(getPluginsXmlFile());

    prog.isScanning = false;
    writeStatus(prog);
    return 0;
}


