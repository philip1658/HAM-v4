/*
  ==============================================================================

    AllTests.cpp
    Main test runner for all HAM unit tests
    Runs all registered JUCE unit tests

  ==============================================================================
*/

#include <JuceHeader.h>

//==============================================================================
int main(int argc, char* argv[])
{
    // Initialize JUCE for console app
    juce::ScopedJuceInitialiser_GUI scopedJuce;
    
    // Create test runner
    juce::UnitTestRunner runner;
    
    // Configure runner
    runner.setAssertOnFailure(false);
    runner.setPassesAreLogged(true);
    
    // Run all tests
    std::cout << "\n========================================\n";
    std::cout << "Running HAM Unit Tests\n";
    std::cout << "========================================\n\n";
    
    runner.runAllTests();
    
    // Calculate results
    int numPassed = 0, numFailed = 0;
    int totalPasses = 0, totalFailures = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        
        if (result->failures > 0)
        {
            numFailed++;
            std::cout << "❌ ";
        }
        else
        {
            numPassed++;
            std::cout << "✅ ";
        }
        
        std::cout << result->unitTestName << ": "
                  << result->passes << " passes";
        
        if (result->failures > 0)
            std::cout << ", " << result->failures << " failures";
            
        std::cout << "\n";
        
        totalPasses += result->passes;
        totalFailures += result->failures;
    }
    
    // Print summary
    std::cout << "\n========================================\n";
    std::cout << "Test Summary:\n";
    std::cout << "  Test Suites: " << numPassed << " passed, " << numFailed << " failed\n";
    std::cout << "  Test Cases:  " << totalPasses << " passed, " << totalFailures << " failed\n";
    std::cout << "  Total:       " << (numPassed + numFailed) << " test suites\n";
    std::cout << "========================================\n";
    
    // Return non-zero if any tests failed
    return numFailed > 0 ? 1 : 0;
}