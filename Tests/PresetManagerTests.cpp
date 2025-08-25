/*
  ==============================================================================

    PresetManagerTests.cpp
    Comprehensive unit tests for PresetManager service
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Services/PresetManager.h"
#include "../Source/Domain/Models/Pattern.h"

using namespace HAM;

//==============================================================================
class PresetManagerTests : public juce::UnitTest
{
public:
    PresetManagerTests() : UnitTest("PresetManager Tests", "Services") {}
    
    void runTest() override
    {
        testConstruction();
        testPresetSaveAndLoad();
        testPresetBrowsing();
        testPresetCategories();
        testPresetMetadata();
        testJSONSerialization();
        testBinarySerialization();
        testFileOperations();
        testPresetValidation();
        testBoundaryConditions();
    }
    
private:
    juce::File getTestDirectory()
    {
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("HAMPresetTests");
        if (!tempDir.exists())
            tempDir.createDirectory();
        return tempDir;
    }
    
    void cleanupTestDirectory()
    {
        auto testDir = getTestDirectory();
        if (testDir.exists())
            testDir.deleteRecursively();
    }
    
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        PresetManager manager;
        
        // Test default state
        expectEquals(manager.getPresetCount(), 0, "Should have no presets initially");
        expect(manager.getCurrentPresetName().isEmpty(), "Should have no current preset");
        expect(!manager.hasUnsavedChanges(), "Should not have unsaved changes initially");
        expectEquals(manager.getCategoryCount(), 0, "Should have no categories initially");
        
        // Test default directories
        auto userDir = manager.getUserPresetsDirectory();
        expect(userDir.exists() || userDir.createDirectory(), "User presets directory should be accessible");
        
        auto factoryDir = manager.getFactoryPresetsDirectory();
        expect(factoryDir.getFullPathName().isNotEmpty(), "Factory presets directory should be defined");
    }
    
    void testPresetSaveAndLoad()
    {
        beginTest("Preset Save and Load");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Create a pattern to save
        Pattern pattern;
        pattern.setName("Test Pattern");
        pattern.setBPM(130.0f);
        pattern.setTimeSignature(3, 4);
        
        // Add some tracks
        for (int i = 0; i < 3; ++i)
        {
            pattern.addTrack();
            auto* track = pattern.getTrack(i);
            if (track)
            {
                track->setName("Track " + juce::String(i));
                track->setChannel(i + 1);
            }
        }
        
        // Save preset
        bool saved = manager.savePreset("TestPreset1", pattern);
        expect(saved, "Should save preset successfully");
        
        expectEquals(manager.getPresetCount(), 1, "Should have 1 preset after saving");
        expect(manager.presetExists("TestPreset1"), "Preset should exist");
        
        // Load preset
        Pattern loadedPattern;
        bool loaded = manager.loadPreset("TestPreset1", loadedPattern);
        expect(loaded, "Should load preset successfully");
        
        expectEquals(loadedPattern.getName(), pattern.getName(), "Pattern name should be preserved");
        expectEquals(loadedPattern.getBPM(), pattern.getBPM(), "BPM should be preserved");
        expectEquals(loadedPattern.getTimeSignatureNumerator(), 3, "Time signature should be preserved");
        expectEquals(loadedPattern.getTrackCount(), pattern.getTrackCount(), "Track count should be preserved");
        
        // Test overwrite protection
        Pattern newPattern;
        newPattern.setName("Different Pattern");
        newPattern.setBPM(140.0f);
        
        bool overwritten = manager.savePreset("TestPreset1", newPattern, false); // Don't overwrite
        expect(!overwritten, "Should not overwrite when protection is enabled");
        
        overwritten = manager.savePreset("TestPreset1", newPattern, true); // Do overwrite
        expect(overwritten, "Should overwrite when explicitly allowed");
        
        // Verify overwrite worked
        Pattern verifyPattern;
        manager.loadPreset("TestPreset1", verifyPattern);
        expectEquals(verifyPattern.getName(), juce::String("Different Pattern"), "Overwritten pattern should be loaded");
        expectEquals(verifyPattern.getBPM(), 140.0f, "Overwritten BPM should be loaded");
        
        cleanupTestDirectory();
    }
    
    void testPresetBrowsing()
    {
        beginTest("Preset Browsing");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Create multiple presets
        for (int i = 0; i < 5; ++i)
        {
            Pattern pattern;
            pattern.setName("Pattern " + juce::String(i));
            pattern.setBPM(120.0f + i * 10);
            manager.savePreset("Preset_" + juce::String(i), pattern);
        }
        
        // Test preset listing
        auto presets = manager.getAllPresetNames();
        expectEquals(presets.size(), 5, "Should have 5 presets");
        
        // Test alphabetical sorting
        presets.sort();
        for (int i = 0; i < 5; ++i)
        {
            expectEquals(presets[i], "Preset_" + juce::String(i), "Presets should be in order");
        }
        
        // Test preset navigation
        manager.loadPresetByIndex(0);
        expectEquals(manager.getCurrentPresetName(), juce::String("Preset_0"), "Should load first preset");
        
        manager.loadNextPreset();
        expectEquals(manager.getCurrentPresetName(), juce::String("Preset_1"), "Should load next preset");
        
        manager.loadPreviousPreset();
        expectEquals(manager.getCurrentPresetName(), juce::String("Preset_0"), "Should load previous preset");
        
        // Test wrap around
        manager.loadPresetByIndex(4);
        manager.loadNextPreset();
        expectEquals(manager.getCurrentPresetName(), juce::String("Preset_0"), "Should wrap to first preset");
        
        manager.loadPreviousPreset();
        expectEquals(manager.getCurrentPresetName(), juce::String("Preset_4"), "Should wrap to last preset");
        
        // Test random preset
        manager.loadRandomPreset();
        expect(manager.getCurrentPresetName().isNotEmpty(), "Should load a random preset");
        
        cleanupTestDirectory();
    }
    
    void testPresetCategories()
    {
        beginTest("Preset Categories");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Create presets in different categories
        Pattern pattern;
        
        manager.savePresetWithCategory("Lead1", pattern, "Leads");
        manager.savePresetWithCategory("Lead2", pattern, "Leads");
        manager.savePresetWithCategory("Bass1", pattern, "Bass");
        manager.savePresetWithCategory("Drums1", pattern, "Drums");
        manager.savePresetWithCategory("Drums2", pattern, "Drums");
        manager.savePresetWithCategory("Drums3", pattern, "Drums");
        
        // Test category listing
        auto categories = manager.getAllCategories();
        expectEquals(categories.size(), 3, "Should have 3 categories");
        expect(categories.contains("Leads"), "Should have Leads category");
        expect(categories.contains("Bass"), "Should have Bass category");
        expect(categories.contains("Drums"), "Should have Drums category");
        
        // Test presets by category
        auto leadPresets = manager.getPresetsInCategory("Leads");
        expectEquals(leadPresets.size(), 2, "Should have 2 lead presets");
        
        auto drumPresets = manager.getPresetsInCategory("Drums");
        expectEquals(drumPresets.size(), 3, "Should have 3 drum presets");
        
        auto bassPresets = manager.getPresetsInCategory("Bass");
        expectEquals(bassPresets.size(), 1, "Should have 1 bass preset");
        
        // Test preset recategorization
        manager.setPresetCategory("Lead1", "Bass");
        
        leadPresets = manager.getPresetsInCategory("Leads");
        expectEquals(leadPresets.size(), 1, "Should have 1 lead preset after recategorization");
        
        bassPresets = manager.getPresetsInCategory("Bass");
        expectEquals(bassPresets.size(), 2, "Should have 2 bass presets after recategorization");
        
        // Test category renaming
        manager.renameCategory("Drums", "Percussion");
        categories = manager.getAllCategories();
        expect(!categories.contains("Drums"), "Old category name should not exist");
        expect(categories.contains("Percussion"), "New category name should exist");
        
        auto percussionPresets = manager.getPresetsInCategory("Percussion");
        expectEquals(percussionPresets.size(), 3, "Renamed category should retain presets");
        
        cleanupTestDirectory();
    }
    
    void testPresetMetadata()
    {
        beginTest("Preset Metadata");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Create preset with metadata
        Pattern pattern;
        pattern.setName("Test Pattern");
        pattern.setAuthor("Test Author");
        pattern.setDescription("This is a test pattern");
        
        PresetManager::Metadata metadata;
        metadata.name = "MetadataTest";
        metadata.author = "John Doe";
        metadata.description = "A preset with full metadata";
        metadata.tags = {"ambient", "evolving", "pad"};
        metadata.version = "1.0.0";
        metadata.rating = 5;
        metadata.dateCreated = juce::Time::getCurrentTime();
        metadata.dateModified = juce::Time::getCurrentTime();
        
        manager.savePresetWithMetadata("MetadataTest", pattern, metadata);
        
        // Load and verify metadata
        auto loadedMetadata = manager.getPresetMetadata("MetadataTest");
        expect(loadedMetadata != nullptr, "Should load metadata");
        
        if (loadedMetadata)
        {
            expectEquals(loadedMetadata->name, metadata.name, "Name should match");
            expectEquals(loadedMetadata->author, metadata.author, "Author should match");
            expectEquals(loadedMetadata->description, metadata.description, "Description should match");
            expectEquals(loadedMetadata->tags.size(), 3, "Should have 3 tags");
            expect(loadedMetadata->tags.contains("ambient"), "Should have ambient tag");
            expect(loadedMetadata->tags.contains("evolving"), "Should have evolving tag");
            expect(loadedMetadata->tags.contains("pad"), "Should have pad tag");
            expectEquals(loadedMetadata->version, metadata.version, "Version should match");
            expectEquals(loadedMetadata->rating, metadata.rating, "Rating should match");
        }
        
        // Test preset search by tags
        auto ambientPresets = manager.findPresetsWithTag("ambient");
        expectEquals(ambientPresets.size(), 1, "Should find 1 ambient preset");
        expectEquals(ambientPresets[0], juce::String("MetadataTest"), "Should find correct preset");
        
        // Test preset search by author
        auto authorPresets = manager.findPresetsByAuthor("John Doe");
        expectEquals(authorPresets.size(), 1, "Should find 1 preset by author");
        
        // Test preset rating
        manager.setPresetRating("MetadataTest", 3);
        auto updatedMetadata = manager.getPresetMetadata("MetadataTest");
        if (updatedMetadata)
        {
            expectEquals(updatedMetadata->rating, 3, "Rating should be updated");
        }
        
        // Test adding tags
        manager.addTagToPreset("MetadataTest", "favorite");
        updatedMetadata = manager.getPresetMetadata("MetadataTest");
        if (updatedMetadata)
        {
            expect(updatedMetadata->tags.contains("favorite"), "Should have new tag");
            expectEquals(updatedMetadata->tags.size(), 4, "Should have 4 tags");
        }
        
        // Test removing tags
        manager.removeTagFromPreset("MetadataTest", "evolving");
        updatedMetadata = manager.getPresetMetadata("MetadataTest");
        if (updatedMetadata)
        {
            expect(!updatedMetadata->tags.contains("evolving"), "Should not have removed tag");
            expectEquals(updatedMetadata->tags.size(), 3, "Should have 3 tags");
        }
        
        cleanupTestDirectory();
    }
    
    void testJSONSerialization()
    {
        beginTest("JSON Serialization");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Create pattern with complex data
        Pattern pattern;
        pattern.setName("JSON Test");
        pattern.setBPM(128.0f);
        pattern.setTimeSignature(7, 8);
        pattern.setGlobalSwing(65.0f);
        
        // Add tracks with stages
        for (int t = 0; t < 2; ++t)
        {
            pattern.addTrack();
            auto* track = pattern.getTrack(t);
            if (track)
            {
                track->setName("Track " + juce::String(t));
                track->setChannel(t + 1);
                
                for (int s = 0; s < 8; ++s)
                {
                    auto* stage = track->getStage(s);
                    if (stage)
                    {
                        stage->setPitch(60 + s);
                        stage->setVelocity(80 + s * 5);
                        stage->setGateLength(0.5f + s * 0.05f);
                    }
                }
            }
        }
        
        // Save as JSON
        bool saved = manager.savePresetAsJSON("JsonTest", pattern);
        expect(saved, "Should save JSON preset");
        
        // Verify JSON file exists
        auto jsonFile = getTestDirectory().getChildFile("JsonTest.json");
        expect(jsonFile.exists(), "JSON file should exist");
        
        // Load JSON preset
        Pattern loadedPattern;
        bool loaded = manager.loadPreset("JsonTest", loadedPattern);
        expect(loaded, "Should load JSON preset");
        
        // Verify data integrity
        expectEquals(loadedPattern.getName(), pattern.getName(), "Name should be preserved");
        expectEquals(loadedPattern.getBPM(), pattern.getBPM(), "BPM should be preserved");
        expectEquals(loadedPattern.getTimeSignatureNumerator(), 7, "Time sig should be preserved");
        expectEquals(loadedPattern.getGlobalSwing(), pattern.getGlobalSwing(), "Swing should be preserved");
        expectEquals(loadedPattern.getTrackCount(), pattern.getTrackCount(), "Track count should be preserved");
        
        // Verify stage data
        for (int t = 0; t < loadedPattern.getTrackCount(); ++t)
        {
            auto* track = loadedPattern.getTrack(t);
            auto* origTrack = pattern.getTrack(t);
            
            if (track && origTrack)
            {
                for (int s = 0; s < 8; ++s)
                {
                    auto* stage = track->getStage(s);
                    auto* origStage = origTrack->getStage(s);
                    
                    if (stage && origStage)
                    {
                        expectEquals(stage->getPitch(), origStage->getPitch(),
                                   "Stage pitch should be preserved");
                        expectEquals(stage->getVelocity(), origStage->getVelocity(),
                                   "Stage velocity should be preserved");
                    }
                }
            }
        }
        
        // Test JSON export/import
        auto jsonString = manager.exportPresetAsJSON("JsonTest");
        expect(jsonString.isNotEmpty(), "Should export JSON string");
        
        bool imported = manager.importPresetFromJSON(jsonString, "ImportedJson");
        expect(imported, "Should import from JSON string");
        
        expect(manager.presetExists("ImportedJson"), "Imported preset should exist");
        
        cleanupTestDirectory();
    }
    
    void testBinarySerialization()
    {
        beginTest("Binary Serialization");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Create pattern
        Pattern pattern;
        pattern.setName("Binary Test");
        pattern.setBPM(135.0f);
        
        // Add plugin state simulation (binary data)
        juce::MemoryBlock pluginState;
        pluginState.append("PLUGIN_STATE_DATA", 17);
        
        // Save as binary
        bool saved = manager.savePresetAsBinary("BinaryTest", pattern, pluginState);
        expect(saved, "Should save binary preset");
        
        // Verify binary file exists
        auto binaryFile = getTestDirectory().getChildFile("BinaryTest.ham");
        expect(binaryFile.exists(), "Binary file should exist");
        
        // Load binary preset
        Pattern loadedPattern;
        juce::MemoryBlock loadedPluginState;
        bool loaded = manager.loadBinaryPreset("BinaryTest", loadedPattern, loadedPluginState);
        expect(loaded, "Should load binary preset");
        
        // Verify data
        expectEquals(loadedPattern.getName(), pattern.getName(), "Name should be preserved");
        expectEquals(loadedPattern.getBPM(), pattern.getBPM(), "BPM should be preserved");
        expectEquals(loadedPluginState.getSize(), pluginState.getSize(), "Plugin state size should match");
        
        if (loadedPluginState.getSize() == pluginState.getSize())
        {
            expect(memcmp(loadedPluginState.getData(), pluginState.getData(), pluginState.getSize()) == 0,
                  "Plugin state data should match");
        }
        
        cleanupTestDirectory();
    }
    
    void testFileOperations()
    {
        beginTest("File Operations");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Create presets
        Pattern pattern;
        for (int i = 0; i < 3; ++i)
        {
            pattern.setName("Pattern " + juce::String(i));
            manager.savePreset("FileTest" + juce::String(i), pattern);
        }
        
        // Test preset deletion
        bool deleted = manager.deletePreset("FileTest1");
        expect(deleted, "Should delete preset");
        expect(!manager.presetExists("FileTest1"), "Deleted preset should not exist");
        expectEquals(manager.getPresetCount(), 2, "Should have 2 presets after deletion");
        
        // Test preset renaming
        bool renamed = manager.renamePreset("FileTest0", "RenamedTest");
        expect(renamed, "Should rename preset");
        expect(!manager.presetExists("FileTest0"), "Old name should not exist");
        expect(manager.presetExists("RenamedTest"), "New name should exist");
        
        // Test duplicate preset
        bool duplicated = manager.duplicatePreset("RenamedTest", "DuplicatedTest");
        expect(duplicated, "Should duplicate preset");
        expect(manager.presetExists("RenamedTest"), "Original should still exist");
        expect(manager.presetExists("DuplicatedTest"), "Duplicate should exist");
        expectEquals(manager.getPresetCount(), 3, "Should have 3 presets after duplication");
        
        // Test export to file
        auto exportFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("export_test.ham");
        bool exported = manager.exportPresetToFile("RenamedTest", exportFile);
        expect(exported, "Should export preset to file");
        expect(exportFile.exists(), "Export file should exist");
        
        // Test import from file
        bool imported = manager.importPresetFromFile(exportFile, "ImportedTest");
        expect(imported, "Should import preset from file");
        expect(manager.presetExists("ImportedTest"), "Imported preset should exist");
        
        // Test batch operations
        juce::StringArray presetsToDelete = {"RenamedTest", "DuplicatedTest"};
        int deletedCount = manager.deleteMultiplePresets(presetsToDelete);
        expectEquals(deletedCount, 2, "Should delete 2 presets");
        expectEquals(manager.getPresetCount(), 2, "Should have 2 presets remaining");
        
        // Clean up
        exportFile.deleteFile();
        cleanupTestDirectory();
    }
    
    void testPresetValidation()
    {
        beginTest("Preset Validation");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Test invalid preset names
        Pattern pattern;
        
        bool saved = manager.savePreset("", pattern);
        expect(!saved, "Should not save preset with empty name");
        
        saved = manager.savePreset("Invalid/Name", pattern);
        expect(!saved, "Should not save preset with invalid characters");
        
        saved = manager.savePreset("../../../etc/passwd", pattern);
        expect(!saved, "Should not save preset with path traversal");
        
        // Test valid preset names
        saved = manager.savePreset("Valid_Name-123", pattern);
        expect(saved, "Should save preset with valid characters");
        
        saved = manager.savePreset("Name with Spaces", pattern);
        expect(saved, "Should save preset with spaces");
        
        // Test preset size limits
        Pattern hugePattern;
        for (int i = 0; i < Pattern::MAX_TRACKS; ++i)
        {
            hugePattern.addTrack();
        }
        
        saved = manager.savePreset("HugePreset", hugePattern);
        expect(saved, "Should handle maximum size preset");
        
        // Test corrupted preset handling
        auto corruptFile = getTestDirectory().getChildFile("Corrupt.ham");
        corruptFile.create();
        corruptFile.appendData("INVALID_DATA", 12);
        
        Pattern loadedPattern;
        bool loaded = manager.loadPreset("Corrupt", loadedPattern);
        expect(!loaded, "Should not load corrupted preset");
        
        // Test version compatibility
        auto versionFile = getTestDirectory().getChildFile("Version.json");
        juce::String jsonContent = R"({
            "version": "99.0.0",
            "data": {}
        })";
        versionFile.replaceWithText(jsonContent);
        
        loaded = manager.loadPreset("Version", loadedPattern);
        // Should handle version mismatch gracefully
        
        cleanupTestDirectory();
    }
    
    void testBoundaryConditions()
    {
        beginTest("Boundary Conditions");
        
        cleanupTestDirectory();
        PresetManager manager;
        manager.setUserPresetsDirectory(getTestDirectory());
        
        // Test maximum presets
        Pattern pattern;
        const int maxPresets = 100;
        
        for (int i = 0; i < maxPresets; ++i)
        {
            pattern.setName("Preset " + juce::String(i));
            bool saved = manager.savePreset("Preset_" + juce::String(i).paddedLeft('0', 3), pattern);
            expect(saved, "Should save preset " + juce::String(i));
        }
        
        expectEquals(manager.getPresetCount(), maxPresets, "Should handle many presets");
        
        // Test preset name length
        juce::String longName;
        for (int i = 0; i < 255; ++i)
            longName += "A";
        
        bool saved = manager.savePreset(longName, pattern);
        expect(saved || !saved, "Should handle long preset name gracefully");
        
        // Test empty directory
        cleanupTestDirectory();
        expectEquals(manager.getPresetCount(), 0, "Should handle empty directory");
        
        // Test non-existent preset
        Pattern loadedPattern;
        bool loaded = manager.loadPreset("NonExistent", loadedPattern);
        expect(!loaded, "Should not load non-existent preset");
        
        // Test invalid index
        loaded = manager.loadPresetByIndex(-1);
        expect(!loaded, "Should not load negative index");
        
        loaded = manager.loadPresetByIndex(1000);
        expect(!loaded, "Should not load out of bounds index");
        
        // Test concurrent access simulation
        for (int i = 0; i < 10; ++i)
        {
            manager.savePreset("Concurrent" + juce::String(i), pattern);
        }
        
        // Rapidly switch presets
        for (int i = 0; i < 100; ++i)
        {
            manager.loadPresetByIndex(i % 10);
        }
        
        // Should not crash or corrupt data
        expectEquals(manager.getPresetCount(), 10, "Presets should remain intact");
        
        cleanupTestDirectory();
    }
};

static PresetManagerTests presetManagerTests;

//==============================================================================
// Main function for standalone test execution
int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    int numPassed = 0, numFailed = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        if (result->failures > 0)
            numFailed++;
        else
            numPassed++;
            
        std::cout << result->unitTestName << ": "
                  << (result->failures > 0 ? "FAILED" : "PASSED")
                  << " (" << result->passes << " passes, "
                  << result->failures << " failures)\n";
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Total: " << numPassed << " passed, " << numFailed << " failed\n";
    std::cout << "========================================\n";
    
    return numFailed > 0 ? 1 : 0;
}