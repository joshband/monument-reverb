#include <JuceHeader.h>

#include "plugin/PluginProcessor.h"
#include <chrono>
#include <csignal>
#include <cstring>
#include <execinfo.h>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

class MonumentUiPrototypeWindow : public juce::DocumentWindow
{
public:
    MonumentUiPrototypeWindow()
        : DocumentWindow("Monument UI Prototype",
                         juce::Colours::black,
                         juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        processor = std::make_unique<MonumentAudioProcessor>();
        auto* editor = processor->createEditor();
        if (editor != nullptr)
        {
            setContentOwned(editor, true);
            setResizable(true, false);
            centreWithSize(getWidth(), getHeight());
        }
        else
        {
            auto* fallback = new juce::Label("editorUnavailable", "Editor unavailable");
            fallback->setJustificationType(juce::Justification::centred);
            setContentOwned(fallback, true);
            centreWithSize(480, 240);
        }
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    std::unique_ptr<MonumentAudioProcessor> processor;
};

class MonumentUiPrototypeApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override
    {
        return "Monument UI Prototype";
    }

    const juce::String getApplicationVersion() override
    {
        return "0.1.0";
    }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MonumentUiPrototypeWindow>();
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

private:
    std::unique_ptr<MonumentUiPrototypeWindow> mainWindow;
};

namespace
{
std::string sanitizeAsciiArg(const char* arg)
{
    if (arg == nullptr)
        return {};
    std::string out;
    out.reserve(std::strlen(arg));
    const unsigned char* cursor = reinterpret_cast<const unsigned char*>(arg);
    while (*cursor != 0)
    {
        const unsigned char c = *cursor++;
        out.push_back(c < 128 ? static_cast<char>(c) : '?');
    }
    return out;
}

class AssertionBacktraceLogger final : public juce::Logger
{
public:
    void logMessage(const juce::String& message) override
    {
        juce::Logger::outputDebugString(message);

        if (message.contains("JUCE Assertion failure in juce_String.cpp"))
            dumpBacktrace();
    }

private:
    static void dumpBacktrace()
    {
        void* stack[128];
        const int frames = backtrace(stack, static_cast<int>(std::size(stack)));
        backtrace_symbols_fd(stack, frames, STDERR_FILENO);
    }
};
}

juce::JUCEApplicationBase* juce_CreateApplication()
{
    return new MonumentUiPrototypeApplication();
}

int main(int argc, char* argv[])
{
    juce::JUCEApplicationBase::createInstance = &juce_CreateApplication;
    AssertionBacktraceLogger backtraceLogger;
    juce::Logger::setCurrentLogger(&backtraceLogger);

    if (const char* stopEnv = std::getenv("MONUMENT_DEBUG_STOP"))
    {
        if (stopEnv[0] != '\0' && stopEnv[0] != '0')
        {
            if (const char* pidFile = std::getenv("MONUMENT_DEBUG_PID_FILE"))
            {
                std::ofstream out(pidFile, std::ios::trunc);
                if (out)
                    out << static_cast<long>(getpid()) << "\n";
            }
            std::raise(SIGSTOP);
        }
    }

    if (const char* sleepEnv = std::getenv("MONUMENT_DEBUG_SLEEP_MS"))
    {
        const int sleepMs = std::max(0, std::atoi(sleepEnv));
        if (sleepMs > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }

    std::vector<std::string> sanitizedArgs;
    std::vector<const char*> argPtrs;
    sanitizedArgs.reserve(static_cast<size_t>(argc));
    argPtrs.reserve(static_cast<size_t>(argc) + 1);

    for (int i = 0; i < argc; ++i)
    {
        sanitizedArgs.emplace_back(sanitizeAsciiArg(argv[i]));
        argPtrs.push_back(sanitizedArgs.back().c_str());
    }
    argPtrs.push_back(nullptr);

    const int result = juce::JUCEApplicationBase::main(argc, argPtrs.data());
    juce::Logger::setCurrentLogger(nullptr);
    return result;
}
