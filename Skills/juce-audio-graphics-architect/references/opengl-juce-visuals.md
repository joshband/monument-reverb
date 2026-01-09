# OpenGL and Shader Visuals in JUCE

## OpenGL renderer skeleton
Attach an OpenGL context to a component and implement `OpenGLRenderer` callbacks.
Use `juce::OpenGLAppComponent` for a quick start if you want JUCE to manage the context and render loop.
See `references/juce-upstream/examples/OpenGLDemo.h` and `OpenGLAppDemo.h` for upstream patterns.

```cpp
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

private:
    void newOpenGLContextCreated() override
    {
        createShaders();
        createBuffers();
    }

    void renderOpenGL() override
    {
        juce::OpenGLHelpers::clear(juce::Colours::black);
        renderScene();
    }

    void openGLContextClosing() override
    {
        releaseResources();
    }

    void createShaders();
    void createBuffers();
    void renderScene();
    void releaseResources();

    juce::OpenGLContext openGLContext;
};
```

## Shader compilation pattern

```cpp
void OpenGLVisualizer::createShaders()
{
    shader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    shader->addVertexShader(vertexSource);
    shader->addFragmentShader(fragmentSource);
    shader->link();

    attributes.position = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "aPosition");
    uniforms.level = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader, "uLevel");
}
```

## Feeding audio data to the GPU
- Use atomics for small data (level, peak).
- For full spectrum, upload a 1D texture or a VBO and sample it in the shader.

```cpp
std::atomic<float> level { 0.0f };

void renderScene()
{
    const float currentLevel = level.load(std::memory_order_relaxed);
    if (uniforms.level != nullptr)
        uniforms.level->set(currentLevel);

    // Draw your geometry.
}
```

## Particle visualization pattern
- Update particle positions on the message thread (Timer) or a dedicated thread.
- Upload positions to a VBO each frame.

```cpp
struct Particle { float x, y, vx, vy, life; };
std::vector<Particle> particles;

void updateParticles(float dt, float drive)
{
    for (auto& p : particles)
    {
        p.vy += drive * 0.1f;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
    }
}
```

## Performance guidance
- Avoid allocating or compiling shaders in `renderOpenGL()`.
- Use `OpenGLContext::executeOnGLThread()` to update GL resources safely.
- Provide a CPU fallback path when OpenGL is not available.
