#pragma once

#include "DistrhoPlugin.hpp"
#include <NeuralAudio/NeuralModel.h>
#include <string>
#include <vector>
#include <memory>

START_NAMESPACE_DISTRHO

// Parameter indices
enum Parameters {
    kParameterInputLevel = 0,
    kParameterOutputLevel,
    kParameterEnabled,
    kParameterHardBypass,
    kParameterCount
};

// State keys
static const char* kStateKeyModelPath = "modelPath";

class NAMPlugin : public Plugin {
public:
    NAMPlugin();
    ~NAMPlugin() override;

protected:
    // Plugin info
    const char* getLabel() const override {
        return "NeuralAmpModeler";
    }

    const char* getDescription() const override {
        return "Neural Amp Modeler - ML-based guitar amp simulation";
    }

    const char* getMaker() const override {
        return "Frederick Price";
    }

    const char* getHomePage() const override {
        return "https://github.com/rickprice/neural-amp-modeler-bypass-lv2";
    }

    const char* getLicense() const override {
        return "GPL-3.0";
    }

    uint32_t getVersion() const override {
        return d_version(0, 1, 9);
    }

    int64_t getUniqueId() const override {
        return d_cconst('N', 'A', 'M', 'B');
    }

    // Init
    void initParameter(uint32_t index, Parameter& parameter) override;
    void initState(uint32_t index, State& state) override;

    // Internal data
    float getParameterValue(uint32_t index) const override;
    void setParameterValue(uint32_t index, float value) override;
    String getState(const char* key) const override;
    void setState(const char* key, const char* value) override;

    // Process
    void activate() override;
    void deactivate() override;
    void run(const float** inputs, float** outputs, uint32_t frames) override;

    // Optional
    void sampleRateChanged(double newSampleRate) override;

private:
    // Parameters
    float fInputLevel;
    float fOutputLevel;
    float fEnabled;
    float fHardBypass;

    // Neural model
    std::unique_ptr<NeuralAudio::NeuralModel> currentModel;
    std::string currentModelPath;

    // Audio processing state
    double sampleRate;
    float prevDCInput;
    float prevDCOutput;

    // Bypass crossfade state
    bool previousBypassState;
    float bypassFadePosition;
    std::vector<float> inputDelayBuffer;
    size_t delayBufferWritePos;
    static constexpr size_t FADE_TIME_MS = 20;
    static constexpr size_t WARMUP_TIME_MS = 40;
    size_t warmupSamplesRemaining;

    // Pre-calculated coefficients
    float fadeIncrement;
    size_t warmupSamplesTotal;

    // Target values for gain smoothing
    float targetInputLevel;
    float targetOutputLevel;
    float targetBypassGain;

    // Smoothing coefficient
    static constexpr float SMOOTH_COEFF = 0.001f;

    // Current smoothed values
    float inputLevel;
    float outputLevel;
    int32_t maxBufferSize;

    // Private methods
    void updateDelayBufferSize();
    void loadModel(const char* path);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMPlugin)
};

END_NAMESPACE_DISTRHO
