// main.cpp
//
// QA harness test runner for Monument Reverb

#include "monument_adapter.h"
#include "scenario_engine/scenario_executor.h"
#include "scenario_engine/scenario_loader.h"
#include "scenario_engine/test_suite_loader.h"
#include "scenario_engine/test_suite_executor.h"
#include "scenario_engine/invariant_evaluator.h"
#include "runners/in_process_runner.h"
#include <juce_events/juce_events.h>
#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <filesystem>

namespace {

struct CommandLineOptions
{
    std::string inputPath;
    bool discoverMode = false;
    std::string discoverDir;
};

// Factory function for creating Monument adapter
std::unique_ptr<qa::DspUnderTest> createMonumentDut()
{
    return std::make_unique<monument::qa::MonumentAdapter>();
}

// QARunnerFactory that creates an InProcessRunner from a DutFactory
qa::scenario::QARunnerFactory makeInProcessRunnerFactory()
{
    return [](qa::DutFactory dutFactory) -> std::unique_ptr<qa::QARunner> {
        return std::make_unique<qa::InProcessRunner>(dutFactory);
    };
}

int runScenario(const std::string& scenarioPath, const CommandLineOptions& /* options */)
{
    std::cout << "Running scenario: " << scenarioPath << "\n";

    // Load scenario
    auto loadResult = qa::scenario::loadScenarioFile(scenarioPath);
    if (!loadResult.ok)
    {
        std::cerr << "ERROR: Failed to load scenario\n";
        for (const auto& error : loadResult.errors)
            std::cerr << "  - " << error << "\n";
        return 1;
    }

    // Create execution config
    qa::scenario::ExecutionConfig config;
    config.sampleRate = 48000;
    config.blockSize = 512;
    config.numChannels = 2;
    config.outputDir = "qa_output";

    // Create scenario executor
    qa::scenario::ScenarioExecutor executor(
        makeInProcessRunnerFactory(),
        createMonumentDut,
        config
    );

    // Execute scenario
    qa::scenario::ScenarioResult result = executor.execute(loadResult.scenario);

    // Evaluate invariants
    qa::scenario::InvariantEvaluator evaluator;
    evaluator.evaluateInto(loadResult.scenario, result);

    // Report results
    std::cout << "\n=== Scenario Results ===\n";
    std::cout << "Dry path: " << result.dryPath << "\n";
    std::cout << "Wet path: " << result.wetPath << "\n";
    std::cout << "Status: ";
    switch (result.status)
    {
        case qa::scenario::ScenarioResult::Status::PASS:
            std::cout << "PASS\n";
            break;
        case qa::scenario::ScenarioResult::Status::WARN:
            std::cout << "WARN\n";
            break;
        case qa::scenario::ScenarioResult::Status::FAIL:
            std::cout << "FAIL\n";
            break;
        case qa::scenario::ScenarioResult::Status::SKIP:
            std::cout << "SKIP (" << result.skipReason << ")\n";
            break;
        case qa::scenario::ScenarioResult::Status::ERROR:
            std::cout << "ERROR (" << result.errorMessage << ")\n";
            break;
    }

    if (!result.hardFailures.empty())
    {
        std::cout << "\nHard Failures:\n";
        for (const auto& failure : result.hardFailures)
            std::cout << "  - " << failure << "\n";
    }

    if (!result.softWarnings.empty())
    {
        std::cout << "\nSoft Warnings:\n";
        for (const auto& warning : result.softWarnings)
            std::cout << "  - " << warning << "\n";
    }

    std::cout << "\nInvariant Results:\n";
    for (const auto& invResult : result.invariantResults)
    {
        std::cout << "  " << invResult.metric << ": ";
        if (invResult.passed)
            std::cout << "PASS (value=" << invResult.measuredValue << ")\n";
        else
            std::cout << "FAIL (value=" << invResult.measuredValue << ")\n";
    }

    return (result.status == qa::scenario::ScenarioResult::Status::PASS ||
            result.status == qa::scenario::ScenarioResult::Status::WARN) ? 0 : 1;
}

int runTestSuite(const std::string& suitePath, const CommandLineOptions& /* options */)
{
    std::cout << "Running test suite: " << suitePath << "\n";

    // Determine scenario directory (assume sibling to suite file)
    std::filesystem::path scenarioDir = std::filesystem::path(suitePath).parent_path();

    // Load and resolve test suite
    auto resolvedSuite = qa::scenario::loadAndResolveTestSuite(suitePath, scenarioDir);
    if (!resolvedSuite.ok)
    {
        std::cerr << "ERROR: Failed to load test suite\n";
        for (const auto& error : resolvedSuite.errors)
            std::cerr << "  - " << error << "\n";
        return 1;
    }

    // Create execution config
    qa::scenario::ExecutionConfig config;
    config.sampleRate = 48000;
    config.blockSize = 512;
    config.numChannels = 2;
    config.outputDir = "qa_output";

    // Create scenario executor
    qa::scenario::ScenarioExecutor scenarioExecutor(
        makeInProcessRunnerFactory(),
        createMonumentDut,
        config
    );

    // Create suite executor
    qa::scenario::TestSuiteExecutor suiteExecutor(scenarioExecutor);

    // Execute suite
    qa::scenario::TestSuiteResult result = suiteExecutor.execute(
        resolvedSuite.suite,
        resolvedSuite.scenarios,
        nullptr  // No baseline config for now
    );

    // Report results
    std::cout << "\n=== Test Suite Results ===\n";
    std::cout << "Total: " << result.totalScenarios << "\n";
    std::cout << "Passed: " << result.passCount << "\n";
    std::cout << "Warned: " << result.warnCount << "\n";
    std::cout << "Failed: " << result.failCount << "\n";
    std::cout << "Skipped: " << result.skipCount << "\n";
    std::cout << "Errors: " << result.errorCount << "\n";

    if (result.stoppedEarly)
        std::cout << "\n(Stopped early due to failure)\n";

    return result.passed ? 0 : 1;
}

int runDiscoverSuite(const std::string& directory, const CommandLineOptions& /* options */)
{
    std::cout << "Auto-discovering scenarios in: " << directory << "\n";

    auto resolvedSuite = qa::scenario::discoverSuite(
        std::filesystem::path(directory),
        "",  // auto-generate suite ID from dir name
        ""   // auto-generate suite name
    );

    if (!resolvedSuite.ok)
    {
        std::cerr << "ERROR: Suite discovery failed\n";
        for (const auto& error : resolvedSuite.errors)
            std::cerr << "  - " << error << "\n";
        return 1;
    }

    std::cout << "Discovered " << resolvedSuite.scenarios.size() << " scenarios\n";

    // Create execution config
    qa::scenario::ExecutionConfig config;
    config.sampleRate = 48000;
    config.blockSize = 512;
    config.numChannels = 2;
    config.outputDir = "qa_output";

    qa::scenario::ScenarioExecutor scenarioExecutor(
        makeInProcessRunnerFactory(),
        createMonumentDut,
        config
    );

    qa::scenario::TestSuiteExecutor suiteExecutor(scenarioExecutor);

    qa::scenario::TestSuiteResult result = suiteExecutor.execute(
        resolvedSuite.suite,
        resolvedSuite.scenarios,
        nullptr  // No baseline config for now
    );

    // Report results
    std::cout << "\n=== Auto-Discovered Suite Results ===\n";
    std::cout << "Total: " << result.totalScenarios << "\n";
    std::cout << "Passed: " << result.passCount << "\n";
    std::cout << "Warned: " << result.warnCount << "\n";
    std::cout << "Failed: " << result.failCount << "\n";
    std::cout << "Skipped: " << result.skipCount << "\n";
    std::cout << "Errors: " << result.errorCount << "\n";

    return result.passed ? 0 : 1;
}

void printUsage(const char* programName)
{
    std::cout << "Usage:\n";
    std::cout << "  " << programName << " [options] <scenario.json>    Run single scenario\n";
    std::cout << "  " << programName << " [options] <suite.json>       Run test suite\n";
    std::cout << "  " << programName << " [options] --discover <dir>   Auto-discover scenarios in directory\n";
    std::cout << "  " << programName << " [options]                    Run smoke test\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --discover <dir>   Auto-discover all scenarios in directory\n";
    std::cout << "  --help, -h         Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " scenarios/monument/smoke_test.json\n";
    std::cout << "  " << programName << " --discover scenarios/monument/\n";
}

} // namespace

int main(int argc, char** argv)
{
    // Initialize JUCE MessageManager — required for plugin timers
    // (e.g., parameter smoothing timers triggered during automation)
    juce::ScopedJuceInitialiser_GUI juceInit;

    try
    {
        CommandLineOptions options;

        // Parse command-line arguments
        int i = 1;
        while (i < argc)
        {
            std::string arg(argv[i]);

            if (arg == "--help" || arg == "-h")
            {
                printUsage(argv[0]);
                return 0;
            }
            else if (arg == "--discover")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "ERROR: --discover requires a directory path\n";
                    return 1;
                }
                options.discoverMode = true;
                options.discoverDir = argv[i + 1];
                i += 2;
            }
            else if (arg[0] == '-')
            {
                std::cerr << "ERROR: Unknown option: " << arg << "\n";
                printUsage(argv[0]);
                return 1;
            }
            else
            {
                // Non-option argument is the input path
                if (!options.inputPath.empty())
                {
                    std::cerr << "ERROR: Multiple input files specified\n";
                    return 1;
                }
                options.inputPath = arg;
                ++i;
            }
        }

        // Determine what to run
        if (options.discoverMode)
        {
            return runDiscoverSuite(options.discoverDir, options);
        }
        else if (!options.inputPath.empty())
        {
            // Detect if suite or scenario based on filename
            if (options.inputPath.find("suite") != std::string::npos)
                return runTestSuite(options.inputPath, options);
            else
                return runScenario(options.inputPath, options);
        }
        else
        {
            // Default: run smoke test
            const std::string smokeTest = "scenarios/monument/smoke_test.json";
            std::cout << "No input file specified, running default smoke test\n";
            return runScenario(smokeTest, options);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
