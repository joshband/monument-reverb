#pragma once

#include <JuceHeader.h>

class OpenGLVisualizer : public juce::Component, private juce::OpenGLRenderer
{
public:
    OpenGLVisualizer()
    {
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
        openGLContext.setContinuousRepainting(true);
    }

    ~OpenGLVisualizer() override
    {
        openGLContext.detach();
    }

    void setLevel(float newLevel)
    {
        level.store(juce::jlimit(0.0f, 1.0f, newLevel), std::memory_order_relaxed);
    }

private:
    void newOpenGLContextCreated() override
    {
        startTimeSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
        createShaders();
    }

    void renderOpenGL() override
    {
        juce::OpenGLHelpers::clear(juce::Colours::black);

        if (shader == nullptr)
            return;

        const float elapsed = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTimeSeconds);
        shader->use();

        if (timeUniform != nullptr)
            timeUniform->set(elapsed);

        if (levelUniform != nullptr)
            levelUniform->set(level.load(std::memory_order_relaxed));

        // TODO: Bind a VBO and draw a full-screen quad or particle buffer here.
    }

    void openGLContextClosing() override
    {
        releaseShaders();
    }

    void createShaders()
    {
        static const char* vertexSource = R"(
            #version 150
            in vec2 aPosition;
            out vec2 vUv;
            void main()
            {
                vUv = aPosition * 0.5 + 0.5;
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        )";

        static const char* fragmentSource = R"(
            #version 150
            in vec2 vUv;
            out vec4 fragColor;
            uniform float uTime;
            uniform float uLevel;
            void main()
            {
                float pulse = 0.4 + 0.6 * sin(uTime + vUv.x * 4.0) * uLevel;
                fragColor = vec4(0.1, 0.3, 0.6, 1.0) + vec4(pulse, pulse * 0.4, pulse * 0.2, 0.0);
            }
        )";

        shader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
        if (!shader->addVertexShader(vertexSource))
            return;
        if (!shader->addFragmentShader(fragmentSource))
            return;
        if (!shader->link())
            return;

        positionAttribute = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "aPosition");
        timeUniform = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader, "uTime");
        levelUniform = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader, "uLevel");
    }

    void releaseShaders()
    {
        positionAttribute.reset();
        timeUniform.reset();
        levelUniform.reset();
        shader.reset();
    }

    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> positionAttribute;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> timeUniform;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> levelUniform;

    std::atomic<float> level { 0.0f };
    double startTimeSeconds = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLVisualizer)
};
