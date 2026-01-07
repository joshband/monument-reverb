// PHASE 3: Layer compositor with RGBA blending and alpha discipline
#pragma once
#include <JuceHeader.h>
#include <vector>

namespace monument::playground {

/**
 * @brief Composites multiple RGBA image layers with proper alpha blending
 *
 * Follows straight-alpha discipline (not premultiplied alpha) for compatibility
 * with standard image formats and PBR workflows.
 *
 * Supports multiple blend modes for photoreal rendering:
 * - Normal: Standard alpha blend
 * - Multiply: Shadows, ambient occlusion
 * - Screen: Highlights, bloom
 * - Additive: Glow effects
 */
class LayerCompositor
{
public:
    enum class BlendMode
    {
        Normal,     // Standard alpha over blend (straight alpha)
        Multiply,   // Multiply RGB, useful for shadows/AO
        Screen,     // Screen blend, useful for highlights/bloom
        Additive    // Additive blend, useful for glow
    };

    struct Layer
    {
        juce::Image image;
        BlendMode blendMode = BlendMode::Normal;
        float opacity = 1.0f;
        juce::String name;

        Layer() = default;
        Layer(juce::Image img, const juce::String& n, BlendMode mode = BlendMode::Normal, float op = 1.0f)
            : image(img), blendMode(mode), opacity(op), name(n) {}
    };

    LayerCompositor();
    ~LayerCompositor() = default;

    // === Layer Management ===

    /**
     * @brief Load an image from disk and add as a layer
     * @param file Path to image file (PNG, JPG, etc.)
     * @param name Layer name for debugging
     * @param mode Blend mode for this layer
     * @param opacity Layer opacity (0.0 - 1.0)
     * @return true if loaded successfully
     */
    bool loadImage(const juce::File& file,
                   const juce::String& name = "",
                   BlendMode mode = BlendMode::Normal,
                   float opacity = 1.0f);

    /**
     * @brief Add a pre-loaded image as a layer
     * @param image The JUCE image to add
     * @param name Layer name for debugging
     * @param mode Blend mode for this layer
     * @param opacity Layer opacity (0.0 - 1.0)
     */
    void addLayer(const juce::Image& image,
                  const juce::String& name = "",
                  BlendMode mode = BlendMode::Normal,
                  float opacity = 1.0f);

    /**
     * @brief Remove all layers
     */
    void clear();

    /**
     * @brief Get read-only access to layers
     */
    const std::vector<Layer>& getLayers() const { return layers; }

    /**
     * @brief Get number of layers
     */
    size_t getLayerCount() const { return layers.size(); }

    // === Compositing ===

    /**
     * @brief Composite all layers into a single RGBA image
     *
     * Uses straight-alpha blending (not premultiplied).
     * Layers are composited in order (first layer = bottom).
     *
     * @return Composited image with alpha channel preserved
     */
    juce::Image composite();

    /**
     * @brief Get the last composited result (cached)
     * @return Cached composite image, or empty image if none
     */
    const juce::Image& getLastComposite() const { return cachedComposite; }

    // === Debug Visualization ===

    /**
     * @brief Enable/disable debug mode
     *
     * In debug mode, composite() will overlay alpha channel visualization
     */
    void setDebugMode(bool enabled) { debugMode = enabled; }

    /**
     * @brief Check if debug mode is enabled
     */
    bool isDebugMode() const { return debugMode; }

    /**
     * @brief Generate alpha channel visualization for debugging
     *
     * White = fully opaque, Black = fully transparent
     *
     * @return Grayscale image showing alpha values
     */
    juce::Image getAlphaVisualization() const;

private:
    std::vector<Layer> layers;
    juce::Image cachedComposite;
    bool debugMode = false;

    // Blend two pixels using specified blend mode
    juce::Colour blendPixels(const juce::Colour& bottom,
                             const juce::Colour& top,
                             BlendMode mode,
                             float opacity) const;

    // Individual blend mode implementations (all use straight-alpha)
    juce::Colour normalBlend(const juce::Colour& bottom, const juce::Colour& top, float opacity) const;
    juce::Colour multiplyBlend(const juce::Colour& bottom, const juce::Colour& top, float opacity) const;
    juce::Colour screenBlend(const juce::Colour& bottom, const juce::Colour& top, float opacity) const;
    juce::Colour additiveBlend(const juce::Colour& bottom, const juce::Colour& top, float opacity) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerCompositor)
};

} // namespace monument::playground