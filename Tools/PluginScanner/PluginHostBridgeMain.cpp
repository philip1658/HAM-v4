// SPDX-License-Identifier: MIT
#include <JuceHeader.h>

// Very first MVP of an out-of-process plugin host bridge.
// Responsibilities:
// - Launch as separate process (arm64 or via Rosetta x86_64)
// - Load exactly one plugin instance, create editor window
// - Control via lightweight IPC (localhost socket): SHOW/HIDE window
// - Audio now enabled via AudioDeviceManager + AudioProcessorPlayer

// Note: For simplicity and JUCE ChildProcess API, we pass arguments instead of stdin JSON.

class BridgeApp : public juce::JUCEApplication
{
public:
    BridgeApp() = default;
    const juce::String getApplicationName() override { return "PluginHostBridge"; }
    const juce::String getApplicationVersion() override { return "0.1"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String& cmd) override
    {
        juce::ignoreUnused(cmd);
        // Args: --format=<VST3|AudioUnit> --identifier=<pathOrId> [--port=<tcpPort>]
        juce::String format;
        juce::String ident;
        int ipcPort = 0;
        auto args = juce::JUCEApplicationBase::getCommandLineParameters();
        auto tokens = juce::StringArray::fromTokens(args, true);
        for (auto t : tokens)
        {
            if (t.startsWith("--format=")) format = t.fromFirstOccurrenceOf("=", false, false).unquoted();
            else if (t.startsWith("--identifier=")) ident = t.fromFirstOccurrenceOf("=", false, false).unquoted();
            else if (t.startsWith("--port=")) ipcPort = t.fromFirstOccurrenceOf("=", false, false).getIntValue();
        }
        if (format.isEmpty() || ident.isEmpty()) { quit(); return; }

        m_formatManager.addDefaultFormats();

        std::unique_ptr<juce::PluginDescription> pd;
        for (int i = 0; i < m_formatManager.getNumFormats(); ++i)
        {
            auto* fmt = m_formatManager.getFormat(i);
            if (fmt == nullptr) continue;
            if (!fmt->getName().equalsIgnoreCase(format)) continue;

            juce::OwnedArray<juce::PluginDescription> list;
            fmt->findAllTypesForFile(list, ident);
            for (auto* t : list) {
                if (t != nullptr && (t->fileOrIdentifier == ident || t->name == ident)) { pd.reset(new juce::PluginDescription(*t)); break; }
            }
            if (pd) break;
        }

        if (!pd) { quit(); return; }

        juce::String error;
        auto instance = std::unique_ptr<juce::AudioPluginInstance>(m_formatManager.createPluginInstance(*pd, 48000.0, 512, error));
        if (!instance) { quit(); return; }

        // Enable audio processing: route device audio to the plugin via AudioProcessorPlayer
        m_deviceManager.initialiseWithDefaultDevices(0, 2);
        m_player.setProcessor(instance.get());
        m_deviceManager.addAudioCallback(&m_player);

        if (auto* ed = instance->hasEditor() ? instance->createEditor() : nullptr)
        {
            m_window = std::make_unique<juce::DocumentWindow>(pd->name, juce::Colours::black, juce::DocumentWindow::allButtons);
            m_window->setUsingNativeTitleBar(true);
            // Transfer ownership of the editor to the window to avoid double-free and ensure proper teardown order
            m_window->setContentOwned(ed, true);
            m_window->centreWithSize(900, 600);
            m_window->setResizable(true, true);
            m_window->setVisible(true);
        }
        m_instance = std::move(instance);

        // Start IPC server if requested
        if (ipcPort > 0)
        {
            m_ipcThread = std::make_unique<BridgeServerThread>(*this, ipcPort);
            m_ipcThread->startThread();
        }
    }

    void shutdown() override
    {
        // Stop audio first
        m_deviceManager.removeAudioCallback(&m_player);
        m_player.setProcessor(nullptr);
        // Destroy window first (this will also delete the editor it owns), then release the plugin instance
        m_window = nullptr;
        m_instance = nullptr;
        if (m_ipcThread)
        {
            m_ipcThread->stopThread(1000);
            m_ipcThread = nullptr;
        }
    }

    // Public window controls called by IPC
    void showWindow()
    {
        if (m_window != nullptr)
        {
            m_window->setVisible(true);
            m_window->toFront(true);
        }
    }

    void hideWindow()
    {
        if (m_window != nullptr)
            m_window->setVisible(false);
    }

private:
    // IPC server thread based on StreamingSocket (raw ASCII commands: SHOW/HIDE)
    class BridgeServerThread : public juce::Thread
    {
    public:
        BridgeServerThread(BridgeApp& appRef, int port)
            : juce::Thread("BridgeServerThread"), app(appRef), portNumber(port) {}

        void run() override
        {
            if (!listener.createListener(portNumber))
                return;
            while (!threadShouldExit())
            {
                std::unique_ptr<juce::StreamingSocket> client(listener.waitForNextConnection());
                if (client == nullptr)
                {
                    wait(50);
                    continue;
                }
                char buffer[64] {};
                const int n = client->read(buffer, (int) sizeof(buffer) - 1, 2000);
                if (n > 0)
                {
                    buffer[n] = 0;
                    juce::String cmd = juce::String(buffer).trim();
                    if (cmd.equalsIgnoreCase("SHOW"))      app.showWindow();
                    else if (cmd.equalsIgnoreCase("HIDE")) app.hideWindow();
                }
            }
        }

    private:
        BridgeApp& app;
        int portNumber { 0 };
        juce::StreamingSocket listener;
    };

    juce::AudioPluginFormatManager m_formatManager;
    std::unique_ptr<juce::AudioPluginInstance> m_instance;
    std::unique_ptr<juce::DocumentWindow> m_window;

    // Audio hosting in bridge
    juce::AudioDeviceManager m_deviceManager;
    juce::AudioProcessorPlayer m_player;

    std::unique_ptr<BridgeServerThread> m_ipcThread;
};

START_JUCE_APPLICATION(BridgeApp)


