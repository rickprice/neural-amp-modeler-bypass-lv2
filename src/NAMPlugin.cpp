#include "NAMPlugin.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

START_NAMESPACE_DISTRHO

NAMPlugin::NAMPlugin()
    : Plugin(kParameterCount, 0, 1), // parameters, programs, states
      fInputLevel(0.0f),
      fOutputLevel(0.0f),
      fEnabled(0.0f),  // Default: not bypassed
      fHardBypass(0.0f),
      currentModel(nullptr),
      sampleRate(getSampleRate()),
      prevDCInput(0.0f),
      prevDCOutput(0.0f),
      previousBypassState(false),
      bypassFadePosition(0.0f),
      delayBufferWritePos(0),
      warmupSamplesRemaining(0),
      fadeIncrement(0.0f),
      warmupSamplesTotal(0),
      targetInputLevel(1.0f),
      targetOutputLevel(1.0f),
      targetBypassGain(0.0f),
      inputLevel(1.0f),
      outputLevel(1.0f),
      maxBufferSize(512)
{
    currentModelPath.reserve(1024);

    // Pre-calculate fade coefficients
    fadeIncrement = 1.0f / ((FADE_TIME_MS / 1000.0f) * static_cast<float>(sampleRate));
    warmupSamplesTotal = static_cast<size_t>((WARMUP_TIME_MS / 1000.0) * sampleRate);

    // Initialize delay buffer
    updateDelayBufferSize();

    // Set default max buffer size
    NeuralAudio::NeuralModel::SetDefaultMaxAudioBufferSize(maxBufferSize);
}

NAMPlugin::~NAMPlugin()
{
}

void NAMPlugin::initParameter(uint32_t index, Parameter& parameter)
{
    parameter.hints = kParameterIsAutomatable;

    switch (index) {
    case kParameterInputLevel:
        parameter.name = "Input Level";
        parameter.symbol = "input_level";
        parameter.unit = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -20.0f;
        parameter.ranges.max = 20.0f;
        break;

    case kParameterOutputLevel:
        parameter.name = "Output Level";
        parameter.symbol = "output_level";
        parameter.unit = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -20.0f;
        parameter.ranges.max = 20.0f;
        break;

    case kParameterEnabled:
        parameter.name = "Bypass";
        parameter.symbol = "bypass";
        parameter.hints |= kParameterIsBoolean;
        parameter.ranges.def = 0.0f;  // Default: not bypassed (active)
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.designation = kParameterDesignationBypass;
        break;

    case kParameterHardBypass:
        parameter.name = "Hard Bypass";
        parameter.symbol = "hard_bypass";
        parameter.hints |= kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    }
}

void NAMPlugin::initState(uint32_t index, State& state)
{
    if (index == 0) {
        state.key = kStateKeyModelPath;
        state.label = "Model Path";
        state.description = "Path to the neural model file";
        state.hints = kStateIsFilenamePath;
        state.defaultValue = "";
    }
}

float NAMPlugin::getParameterValue(uint32_t index) const
{
    switch (index) {
    case kParameterInputLevel:
        return fInputLevel;
    case kParameterOutputLevel:
        return fOutputLevel;
    case kParameterEnabled:
        return fEnabled;
    case kParameterHardBypass:
        return fHardBypass;
    default:
        return 0.0f;
    }
}

void NAMPlugin::setParameterValue(uint32_t index, float value)
{
    switch (index) {
    case kParameterInputLevel:
        fInputLevel = value;
        break;
    case kParameterOutputLevel:
        fOutputLevel = value;
        break;
    case kParameterEnabled:
        fEnabled = value;
        break;
    case kParameterHardBypass:
        fHardBypass = value;
        break;
    }
}

String NAMPlugin::getState(const char* key) const
{
    if (std::strcmp(key, kStateKeyModelPath) == 0) {
        return String(currentModelPath.c_str());
    }
    return String();
}

void NAMPlugin::setState(const char* key, const char* value)
{
    if (std::strcmp(key, kStateKeyModelPath) == 0) {
        loadModel(value);
    }
}

void NAMPlugin::activate()
{
    // Reset processing state
    prevDCInput = 0.0f;
    prevDCOutput = 0.0f;
    previousBypassState = false;
    bypassFadePosition = 0.0f;
    warmupSamplesRemaining = 0;

    // Clear delay buffer
    std::fill(inputDelayBuffer.begin(), inputDelayBuffer.end(), 0.0f);
    delayBufferWritePos = 0;
}

void NAMPlugin::deactivate()
{
}

void NAMPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    const float* in = inputs[0];
    float* out = outputs[0];

    // Check bypass state (fEnabled is actually bypass: 1.0 = bypassed, 0.0 = active)
    const bool bypassed = fEnabled >= 0.5f;

    // If bypassed, just copy input to output
    if (bypassed) {
        std::copy(in, in + frames, out);
        return;
    }

    // ========== Calculate Target Gain Values ==========
    float modelInputAdjustmentDB = 0.0f;
    float modelOutputAdjustmentDB = 0.0f;

    if (currentModel != nullptr) {
        modelInputAdjustmentDB = currentModel->GetRecommendedInputDBAdjustment();
        modelOutputAdjustmentDB = currentModel->GetRecommendedOutputDBAdjustment();
    }

    targetInputLevel = std::pow(10.0f, (fInputLevel + modelInputAdjustmentDB) * 0.05f);
    targetOutputLevel = std::pow(10.0f, (fOutputLevel + modelOutputAdjustmentDB) * 0.05f);

    // ========== Apply Input Gain ==========
    const float smoothCoeff = SMOOTH_COEFF;
    float inGain = inputLevel;

    for (uint32_t i = 0; i < frames; i++) {
        inGain += smoothCoeff * (targetInputLevel - inGain);
        out[i] = in[i] * inGain;
    }
    inputLevel = inGain;

    // ========== Process Neural Model ==========
    if (currentModel != nullptr) {
        currentModel->Process(out, out, frames);
    }

    // ========== Apply Output Gain ==========
    float outGain = outputLevel;

    for (uint32_t i = 0; i < frames; i++) {
        outGain += smoothCoeff * (targetOutputLevel - outGain);
        out[i] *= outGain;
    }

    outputLevel = outGain;
}

void NAMPlugin::sampleRateChanged(double newSampleRate)
{
    sampleRate = newSampleRate;

    // Recalculate fade coefficients
    fadeIncrement = 1.0f / ((FADE_TIME_MS / 1000.0f) * static_cast<float>(sampleRate));
    warmupSamplesTotal = static_cast<size_t>((WARMUP_TIME_MS / 1000.0) * sampleRate);

    // Update delay buffer
    updateDelayBufferSize();
}

void NAMPlugin::updateDelayBufferSize()
{
    size_t fadeTimeSamples = static_cast<size_t>((FADE_TIME_MS / 1000.0) * sampleRate);
    size_t delayBufferSize = fadeTimeSamples + maxBufferSize;

    inputDelayBuffer.resize(delayBufferSize, 0.0f);
    delayBufferWritePos = 0;
}

void NAMPlugin::loadModel(const char* path)
{
    if (!path || std::strlen(path) == 0) {
        currentModel.reset();
        currentModelPath.clear();
        return;
    }

    try {
        auto newModel = NeuralAudio::NeuralModel::CreateFromFile(path);
        if (newModel != nullptr) {
            currentModel.reset(newModel);
            currentModelPath = path;

            // Reset processing state when model changes
            prevDCInput = 0.0f;
            prevDCOutput = 0.0f;
            std::fill(inputDelayBuffer.begin(), inputDelayBuffer.end(), 0.0f);
            delayBufferWritePos = 0;
        }
    } catch (const std::exception& e) {
        // Model loading failed
        currentModel.reset();
        currentModelPath.clear();
    }
}

Plugin* createPlugin()
{
    return new NAMPlugin();
}

END_NAMESPACE_DISTRHO
