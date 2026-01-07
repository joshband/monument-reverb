#include "WebViewKnobDemo.h"
#include <juce_core/juce_core.h>

namespace Monument
{

WebViewKnobDemo::WebViewKnobDemo()
{
    // Configure label
    label.setText("WebView Knob (CSS Blend Modes)", juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setFont(juce::FontOptions(14.0f));
    addAndMakeVisible(label);

    // Create WebBrowserComponent with default options
    webView = std::make_unique<juce::WebBrowserComponent>();

    // Load the embedded HTML file
    auto htmlFile = juce::File::getCurrentWorkingDirectory()
        .getChildFile("Assets")
        .getChildFile("webview")
        .getChildFile("knob_demo_embedded.html");

    // Try alternative path if not found
    if (!htmlFile.existsAsFile())
    {
        auto parentDir = juce::File::getCurrentWorkingDirectory().getParentDirectory();
        htmlFile = parentDir.getParentDirectory().getParentDirectory()
            .getChildFile("Assets")
            .getChildFile("webview")
            .getChildFile("knob_demo_embedded.html");
    }

    // Create simple test HTML to verify WebView works
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto tempHtmlFile = tempDir.getChildFile("monument_knob_test.html");

    juce::String testHtml =
        "<!DOCTYPE html><html><head><meta charset='utf-8'></head>"
        "<body style='background:white;display:flex;align-items:center;"
        "justify-content:center;height:100vh;margin:0;font-family:sans-serif;'>"
        "<div style='text-align:center;'>"
        "<div style='width:180px;height:180px;border-radius:50%;background:"
        "radial-gradient(circle at 40% 40%, #6495ed, #000080);"
        "box-shadow:inset 0 0 60px rgba(255,255,255,0.3), 0 10px 30px rgba(0,0,0,0.5);'>"
        "</div><p style='margin-top:20px;color:#666;'>CSS Gradient Knob</p></div></body></html>";

    tempHtmlFile.replaceWithText(testHtml);

    DBG("Created temp HTML at: " << tempHtmlFile.getFullPathName());
    webView->goToURL(tempHtmlFile.getFullPathName());
    DBG("Loading WebView from: " << tempHtmlFile.getFullPathName());

    addAndMakeVisible(*webView);
}

WebViewKnobDemo::~WebViewKnobDemo()
{
}

void WebViewKnobDemo::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
}

void WebViewKnobDemo::resized()
{
    auto bounds = getLocalBounds();

    // Reserve space for label at top
    auto labelHeight = 20;
    auto labelBounds = bounds.removeFromTop(labelHeight);
    label.setBounds(labelBounds);

    // WebView takes remaining space
    webView->setBounds(bounds);
}

void WebViewKnobDemo::setValue(double value)
{
    currentValue = juce::jlimit(0.0, 1.0, value);

    // Execute JavaScript to update the knob in the web page
    if (webView)
    {
        juce::String js = "if (typeof updateKnob === 'function') { updateKnob(" +
                         juce::String(currentValue, 2) + "); }";
        // Note: JUCE 8 WebBrowserComponent doesn't have executeScript() on all platforms
        // This is a simplified demo - full implementation would need platform-specific code
    }
}

} // namespace Monument
